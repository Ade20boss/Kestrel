What we built in KES-003 is basically the **numerical type system foundation** of Kestrel.

At the highest level:

> A dtype tells Kestrel what kind of values a tensor contains and how those values should behave.

Without dtype information, Kestrel would just have raw bytes.

For example, suppose memory contains:

```
00000000 00000000 10000000 00111111
```

Those 4 bytes by themselves mean nothing.

They could be interpreted as:

```
FP32
INT32
UINT32
part of two FP16 values
four INT8 values
```

The **dtype gives the bytes meaning**.

---

# 1. Why dtypes exist in the first place

Imagine a tensor:

```
[1, 2, 3, 4]
```

You might think:

> “Those are just numbers.”

But a computer has to know:

```
Are these integers?
Are they floats?
How many bytes does each number occupy?
Can they be negative?
How much precision do they have?
What happens if I combine them with another type?
What type should the result become?
```

Those questions matter because hardware does not execute a generic operation called:

```
add two numbers
```

It executes things more like:

```
add two 32-bit integers
add two 64-bit integers
add two FP32 values
add packed FP16 values
```

The representation affects:

- memory usage,
- precision,
- range,
- arithmetic behavior,
- SIMD support,
- GPU support,
- kernel selection,
- transfer cost,
- planner decisions.

So dtype is not cosmetic metadata.

It influences both **correctness** and **performance**.

---

# 2. The dtype set we chose

Kestrel currently supports:

```
BOOL

FP16
BF16
FP32
FP64

INT8
UINT8
INT16
UINT16
INT32
UINT32
INT64
UINT64
```

plus:

```
INVALID
COUNT
```

The actual enum in `kestrel_core.h` is the identity system for these types.

We deliberately put dtype identity in `kestrel_core.h` because nearly every part of Kestrel will eventually need to talk about dtypes.

Examples:

```
tensor
backend capability
kernel
planner
cast
storage sizing
operator validation
```

But the behavior does **not** live in `kestrel_core.h`.

That distinction matters:

```
kestrel_core.h
→ what dtype exists

dtype.h / dtype.c
→ what those dtypes mean and how they behave
```

---

# 3. Why `INVALID = 0`

We deliberately made:

```
KESTREL_DTYPE_INVALID = 0
```

This matches the same safe-zero philosophy we used in the arena.

If some structure is zero-initialized:

```
dtype = 0
```

then it naturally means:

```
no valid dtype yet
```

instead of accidentally meaning:

```
FP32
```

That is a small decision, but it improves safety throughout the runtime.

---

# 4. Why `COUNT` exists

`COUNT` is not a dtype.

It is a terminal sentinel.

Conceptually:

```
INVALID = 0
FP16
BF16
...
BOOL
COUNT
```

It becomes useful later for things such as:

```
for each dtype
capability tables
lookup tables
tests
backend matrices
```

Your test suite even uses that contiguous range to iterate through all real dtype combinations in the promotion symmetry test.

And importantly, `COUNT` itself is rejected as a real dtype.

---

# 5. Dtype validity

We added:

```
kestrel_dtype_is_valid()
```

because later code should not have to reinvent:

> “Is this actually a real Kestrel dtype?”

Valid means one of the supported real dtypes.

Invalid means:

```
INVALID
COUNT
garbage enum value
```

Your tests explicitly verify both positive garbage like `999` and negative garbage like `-1` are rejected.

Why this matters:

Suppose later the planner receives:

```
dtype = 93842
```

We want a single authoritative place that says:

```
No. That is not a dtype.
```

rather than allowing undefined assumptions deeper in the runtime.

---

# 6. Dtype size

Then we built:

```
kestrel_dtype_size()
```

This answers:

> How many bytes does one element of this dtype occupy?

Examples:

```
INT8   → 1 byte
FP16   → 2 bytes
FP32   → 4 bytes
FP64   → 8 bytes
INT64  → 8 bytes
BOOL   → 1 byte
```

Your tests cover every supported dtype plus invalid sentinels.

This becomes fundamental when tensors arrive.

Imagine:

```
shape = [4, 8]
dtype = FP32
```

Logical element count:

```
4 × 8 = 32 elements
```

Each FP32 value:

```
4 bytes
```

So a tightly packed tensor needs:

```
32 × 4 = 128 bytes
```

Without dtype size, Kestrel can't even determine how much memory a tensor's elements require.

---

# 7. Why we did not create a separate integer-width table

You caught this yourself.

Originally we talked about:

```
integer width = 8 / 16 / 32 / 64 bits
```

Then you asked:

> If we already have `dtype_size()`, why duplicate width?

Exactly.

For integer dtypes:

```
width_bits = dtype_size * 8
```

So:

```
INT32
→ 4 bytes
→ 32 bits
```

We avoided creating two sources of truth.

That's an important systems design instinct:

> If something is cheap and unambiguous to derive, don't duplicate it unless there's a strong reason.

---

# 8. Dtype classification

Then we built classification helpers.

Conceptually:

```
is_float()
is_integer()
is_bool()
is_signed_integer()
is_unsigned_integer()
```

Your tests exhaustively check each category.

Why are these useful?

Because later code should be able to ask semantic questions like:

```
Does this backend support floating-point inputs?

Is this operation valid only for integers?

Does this dtype participate in numeric promotion?

Can this type be treated as signed integer arithmetic?

Is this tensor a boolean mask?
```

instead of repeatedly listing enum values everywhere.

---

# 9. Why `is_signed_integer()` instead of `is_signed()`

This was one of the subtle design discussions.

At first, something like:

```
is_signed(FP32)
```

sounds reasonable because FP32 can represent negative values.

But then the term becomes ambiguous.

In low-level numeric systems, “signed vs unsigned” is primarily an integer classification.

So we made the API explicit:

```
is_signed_integer
is_unsigned_integer
```

Then:

```
FP32
```

is neither.

It is simply:

```
floating point
```

That prevents semantic confusion later.

---

# 10. Why the classification helpers are `static inline`

These helpers are tiny.

Something like:

```
is this one of these enum values?
```

So they are good candidates for header-level `static inline`.

The idea is:

```
static
→ private copy per translation unit

inline
→ compiler can directly substitute the tiny operation when useful
```

But the bigger architectural reason is not raw speed.

It is that these are:

```
small
stable
side-effect free
behaviorally simple
```

Whereas things like promotion are more substantial and belong in `dtype.c`.

---

# 11. Promotion

This is the biggest thing KES-003 added.

Promotion answers:

> If two operands have different dtypes, what common dtype should Kestrel use?

Example:

```
FP16 + FP32
```

The hardware/kernel can't just execute “two random types.”

We need a common arithmetic representation.

So:

```
FP16 + FP32
→ FP32
```

Your test suite now covers the full float promotion matrix.

---

# 12. Promotion is not the same as overflow prevention

This was an important conceptual correction we made during the design.

Example:

```
FP16 + FP16
→ FP16
```

That does **not** mean the result is guaranteed to fit.

If both values are huge enough:

```
60000 + 60000
```

the result can exceed FP16's finite range.

So:

```
promotion
```

answers:

```
what type should the operation use?
```

not:

```
can every possible result fit?
```

That distinction is crucial.

---

# 13. Float promotion

We froze:

```
FP16 + FP16 → FP16
BF16 + BF16 → BF16

FP16 + BF16 → FP32

FP16/BF16 + FP32 → FP32

anything involving FP64 → FP64
```

Why is:

```
FP16 + BF16 → FP32
```

special?

Because FP16 and BF16 are both 16-bit floats, but they make different tradeoffs.

Very roughly:

```
FP16
→ more precision
→ less range

BF16
→ more range
→ less precision
```

So neither one fully dominates the other.

FP32 gives us a sensible common representation.

---

# 14. Integer promotion

Integer promotion was more interesting because of signedness.

Consider:

```
INT8
```

range roughly:

```
-128 ... 127
```

and:

```
UINT8
```

range:

```
0 ... 255
```

Neither INT8 nor UINT8 can represent the complete domain of the other.

So:

```
INT8 + UINT8
→ INT16
```

because INT16 can represent both input domains.

Your tests explicitly cover equal-width mixed signedness cases like this.

---

# 15. Wider signed + narrower unsigned

Example:

```
INT16 + UINT8
```

INT16 can already represent:

```
-32768 ... 32767
```

which includes the entire UINT8 domain:

```
0 ... 255
```

So:

```
INT16 + UINT8
→ INT16
```

Your tests cover this pattern through INT16/INT32/INT64.

---

# 16. Unsigned wider than signed

Example:

```
INT8 + UINT16
```

INT16 is not enough, because UINT16 can reach:

```
65535
```

So we need:

```
INT32
```

Thus:

```
INT8 + UINT16
→ INT32
```

Likewise:

```
INT16 + UINT32
→ INT64
```

Your tests cover these transitions.

---

# 17. The UINT64 problem

Now:

```
INT64 + UINT64
```

What wider signed integer do we have?

None.

We stop at INT64.

So no supported integer dtype can exactly represent both domains.

Therefore:

```
INT64 + UINT64
→ INVALID implicit promotion
```

The user must explicitly choose what loss/conversion semantics they want.

Your tests cover all the “unsigned 64-bit has no larger signed type” cases.

This is a deliberate Kestrel philosophy:

> Do not silently lose information when the runtime cannot find a safe implicit common type.

---

# 18. Integer + float promotion

Then we had to answer something harder:

```
INT32 + FP32
```

Why not simply return FP32?

Because FP32 cannot exactly represent every possible INT32 value.

So under Kestrel's current policy:

```
INT32 + FP32
→ FP64
```

because FP64 can represent the full INT32 domain exactly.

Likewise:

```
INT16 + FP16
→ FP32
```

because FP16 cannot represent every 16-bit integer exactly.

Your tests hit the 8-, 16-, 32-, and 64-bit mixed width classes.

---

# 19. Why INT64 + FP64 is invalid

This one surprises people initially.

FP64 is huge, but:

> Huge range is not the same as exact integer precision.

FP64 cannot exactly distinguish every possible 64-bit integer.

So:

```
INT64 + FP64
```

does not have a supported floating type that preserves the complete integer domain exactly.

Therefore:

```
→ INVALID implicit promotion
```

If the developer wants to convert INT64 to FP64 anyway, they must do it explicitly.

That makes potential information loss visible.

---

# 20. Why BOOL does not participate in numeric promotion

We deliberately said:

```
BOOL + INT32
→ invalid
```

and even:

```
BOOL + BOOL
→ invalid numeric promotion
```

BOOL is for:

```
logical operations
comparisons
masks
conditions
```

not ordinary arithmetic.

That prevents strange implicit behavior like:

```
true + FP32
```

quietly turning into numeric arithmetic.

Your test suite explicitly traps BOOL numeric promotion.

---

# 21. Why promotion must be symmetric

We want:

```
promote(A, B)
```

to equal:

```
promote(B, A)
```

because operand ordering should not change the common dtype.

For example:

```
INT16 + UINT32
```

and:

```
UINT32 + INT16
```

must resolve identically.

Your test suite checks this across all 169 real dtype pairs.

That is one of the strongest invariants in the module.

---

# 22. Why you normalized operands internally

Your implementation uses a swap phase.

Conceptually:

```
float + integer
→ force float into first

signed + unsigned
→ force signed into first
```

This is good because instead of writing:

```
if A signed and B unsigned...
else if A unsigned and B signed...
```

everywhere, you normalize once.

Then later logic can assume:

```
first = signed
second = unsigned
```

or:

```
first = float
second = integer
```

This reduces branching and makes the algorithm easier to reason about.

---

# 23. Accumulation

Then we separated another concept:

```
accumulation
```

from promotion.

Accumulation answers:

> If we're repeatedly adding intermediate results, what dtype should hold the running total?

Example:

```
sum(FP16)
```

There may be thousands of additions.

Keeping the running sum in FP16 could lose precision very quickly.

So:

```
FP16
→ accumulate in FP32
```

Your final tests cover every accumulation mapping.

---

# 24. Why FP16/BF16 accumulate in FP32

Suppose you're doing:

```
a0*b0
+ a1*b1
+ a2*b2
+ ...
```

in a neural network dot product.

FP16 and BF16 are compact storage/compute formats, but they're relatively low precision.

So Kestrel says:

```
FP16 → FP32 accumulator
BF16 → FP32 accumulator
```

This improves numerical stability.

FP32 already accumulates into FP32.

FP64 stays FP64.

---

# 25. Integer accumulation

Our current policy is:

```
INT8  → INT32
INT16 → INT64
INT32 → INT64
INT64 → INT64

UINT8  → UINT32
UINT16 → UINT64
UINT32 → UINT64
UINT64 → UINT64
```

Why widen?

Because repeated additions need more headroom.

Example:

```
INT8 values
```

individually are tiny.

But summing thousands of them can produce a value much larger than INT8 can hold.

So we accumulate into a wider type.

---

# 26. Promotion and accumulation can both occur

Imagine mixed-type matrix multiplication:

```
FP16 × FP32
```

First:

```
promotion
FP16 + FP32
→ FP32
```

Then:

```
accumulation
FP32
→ FP32
```

For another example:

```
INT8 × UINT8
```

First:

```
promotion
INT8 + UINT8
→ INT16
```

Then:

```
accumulation
INT16
→ INT64
```

Two separate decisions.

---

# 27. Why accumulation does not guarantee no overflow

Widening helps.

It does not make overflow mathematically impossible.

Example:

```
INT8
→ INT32 accumulator
```

You can still add enough values to overflow INT32.

So we explicitly separated:

```
promotion
accumulation
overflow
```

This was probably the most important conceptual cleanup in the whole ticket.

---

# 28. Overflow policy

KES-003 doesn't implement arithmetic kernels yet.

But we froze the semantics.

For floating point:

```
overflow
→ +INF / -INF
```

and invalid operations can produce:

```
NaN
```

in IEEE-style behavior.

For integers:

```
fixed-width modular wraparound
```

Conceptually:

```
INT8:
127 + 1 → -128

UINT8:
255 + 1 → 0
```

Critically, later C implementations must not simply rely on C signed overflow, because C signed overflow can be undefined behavior.

So this becomes a backend implementation constraint later.

---

# 29. Why dtype semantics are global instead of backend-specific

This is extremely important for Kestrel.

Suppose:

```
AVX2 says:
INT8 + UINT8 → INT16
```

but CUDA says:

```
INT8 + UINT8 → INT32
```

Then Kestrel would behave differently depending on where the planner executes the operation.

That's unacceptable.

So:

> Backend determines **how** an operation executes.

But Kestrel core determines:

```
what the operation means
```

Dtype promotion and accumulation are semantic rules.

Scalar, AVX2, Vulkan and CUDA must all obey the same ones.

---

# 30. Why this matters for the future planner

Eventually the planner may ask:

```
Can AVX2 execute FP16 efficiently?

Does Vulkan backend instance support BF16?

Can CUDA execute INT8 matmul?

Would converting FP64 → FP32 be required?

Would that conversion violate semantics?
```

The planner needs a stable dtype system before it can answer any of those questions.

So KES-003 isn't an isolated utility.

It becomes part of the foundation for:

```
tensor validation
backend capability
kernel dispatch
conversion planning
memory sizing
cost modelling
execution legality
```

---

# 31. The whole KES-003 architecture in one picture

Think of it like this:

```
               KESTREL DTYPE CORE

                    dtype ID
                       │
          ┌────────────┼────────────┐
          │            │            │
        size       category      validity
          │            │            │
          └───────┬────┴─────┬─────┘
                  │          │
              promotion   accumulation
                  │          │
                  └────┬─────┘
                       │
                 numeric semantics
                       │
        ┌──────────────┼───────────────┐
        │              │               │
      tensor         backend         planner
```

---

# 32. Why KES-003 had to come before tensors

Later our tensor will conceptually look like:

```
tensor_t
├ shape
├ stride_bytes
├ offset_bytes
├ dtype
└ storage
```

Notice:

```
dtype
```

is one of the tensor's fundamental properties.

Before building tensor validation, we need to know:

```
what dtypes exist
how large they are
which are valid
how they interact numerically
```

Otherwise tensor code would start inventing dtype assumptions itself.

That would create technical debt immediately.

---

# 33. What you actually built, in plain English

Forget all the C for a second.

You built a system that can answer:

```
"What kind of number is this?"

"Is this number type valid?"

"How much memory does one value consume?"

"Is it float, integer or boolean?"

"If it's integer, is it signed or unsigned?"

"If two different number types interact,
what common type should they use?"

"If I repeatedly combine values,
what type should hold the running result?"
```

That is KES-003.

And the test suite now proves the major rules rather than just checking a few examples, including exhaustive symmetry plus explicit semantic boundaries. Pasted text.txtTXT

---

## The mental model I want you to carry into KES-004

For KES-003:

> **Dtype describes the numerical meaning of values.**

KES-004 is going to move into a completely different dimension:

> **Runtime topology describes the machine available to execute work.**

So we're transitioning from:

```
WHAT IS THE DATA?
```

to:

```
WHAT COMPUTE HARDWARE EXISTS?
WHAT BACKENDS CAN USE IT?
WHAT MEMORY IS ATTACHED/ACCESSIBLE?
```

That's why doing this review now matters. KES-003 gives Kestrel a numerical language; KES-004 will start giving Kestrel a language for understanding the machine.