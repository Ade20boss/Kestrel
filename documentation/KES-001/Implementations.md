# Texyn Labs — Kestrel Engineering

# KES-001 — Arena Implementation Contract

**Project:** Kestrel  
**Subsystem:** Memory Management  
**Component:** Host Arena Allocator  
**Architecture:** ADR-001  
**Ticket:** KES-001  
**Status:** Merged  
**Purpose:** Define the exact implementation-facing contract of the host arena.

---

# 1. Purpose of This Document

ADR-001 explains **why** the arena is designed the way it is.

This document defines **what the implementation must actually do**.

It answers:

- what every arena field means;
    
- which states are valid;
    
- which functions are legal in each state;
    
- what every function may modify;
    
- what every function must preserve on failure;
    
- which operations constitute programming misuse;
    
- what debug and release builds do;
    
- what every arena test is actually proving.
    

The source code may evolve internally.

The contract must not change accidentally.

---

# 2. Core Arena Representation

The host arena fundamentally contains:

```
arena_t
├── data
├── capacity
└── offset
```

Conceptually:

```
data
 │
 ▼
┌────────────────────────────────────────────────┐
│ allocated / used           │ available         │
└────────────────────────────────────────────────┘
                             ▲
                           offset

<---------------- capacity --------------------->
```

---

# 3. Field Semantics

## `data`

`data` identifies the beginning of the arena's owned host-memory allocation.

When the arena is LIVE:

```
data != NULL
```

When the arena is ZERO:

```
data == NULL
```

`data` is the ownership anchor for the arena.

Losing or overwriting this value while the arena is LIVE would lose the only reference Kestrel has to the underlying allocation.

---

## `capacity`

`capacity` is the total usable byte extent of the arena.

It does **not** mean:

- bytes currently allocated;
    
- number of objects;
    
- number of tensors;
    
- number of elements.
    

It describes the total storage extent owned by the arena.

Invariant while LIVE:

```
capacity > 0
```

---

## `offset`

`offset` represents the current allocation frontier measured in bytes from `data`.

Conceptually:

```
address of next allocation candidate
=
data + aligned(offset)
```

Invariant while LIVE:

```
offset <= capacity
```

`offset` may move:

```
forward
```

during successful allocation, and:

```
backward
```

during rollback/reset.

It must never exceed `capacity`.

---

# 4. Arena Mark Representation

Kestrel defines an arena mark as an offset value.

Conceptually:

```
arena_mark_t = size_t
```

A mark represents:

> the allocation frontier at a particular point during the arena's current lifetime.

A mark does not own memory.

A mark is not a pointer.

A mark does not preserve allocations after reset.

---

# 5. Global State Model

Every arena must always be interpreted as belonging to one of three states.

## ZERO

```
data     == NULL
capacity == 0
offset   == 0
```

ZERO is valid.

It means:

> no live arena allocation currently exists.

---

## LIVE

```
data     != NULL
capacity > 0
offset   <= capacity
```

LIVE is valid.

It means:

> the arena owns a valid backing allocation.

---

## INVALID

Anything else.

Examples:

```
data == NULL
capacity > 0
```

```
data != NULL
capacity == 0
```

```
offset > capacity
```

INVALID is never a legitimate persistent state.

---

# 6. Global Mutation Rule

No arena operation may accidentally turn:

```
ZERO → INVALID
```

or:

```
LIVE → INVALID
```

A public operation must either:

```
complete a valid state transition
```

or:

```
leave the previous valid state unchanged
```

This is one of the most important implementation rules in KES-001.

---

# 7. State Transition Table

|Operation|Input State|Result|
|---|---|---|
|`arena_init` success|ZERO|LIVE|
|`arena_init` recoverable failure|ZERO|ZERO|
|`arena_init` misuse|LIVE|LIVE unchanged|
|`arena_init` misuse|INVALID|INVALID unchanged|
|`arena_alloc` success|LIVE|LIVE with larger offset|
|`arena_alloc` recoverable failure|LIVE|LIVE unchanged|
|`arena_alloc` misuse|ZERO/INVALID|unchanged|
|`arena_get_mark`|LIVE|LIVE unchanged|
|`arena_pop_to_mark` valid rollback|LIVE|LIVE with same/smaller offset|
|`arena_pop_to_mark` stale/future mark|LIVE|LIVE unchanged|
|`arena_reset`|LIVE|LIVE with offset = 0|
|`arena_destroy`|LIVE|ZERO|
|`arena_destroy`|ZERO|ZERO|
|`arena_destroy(NULL)`|—|no-op|
|`arena_destroy`|INVALID|INVALID unchanged|

The implementation must never use a failed operation to "repair" an INVALID arena unless some future API explicitly defines such behavior.

---

# 8. `arena_init` Contract

## Responsibility

Create a LIVE arena from an exact ZERO arena.

---

## Preconditions

The arena pointer must be valid.

The arena object itself must be in exact ZERO state:

```
data     == NULL
capacity == 0
offset   == 0
```

The requested capacity must be valid.

Capacity calculations and alignment rounding must not overflow.

---

## Successful Postcondition

After successful initialization:

```
data != NULL
capacity > 0
offset == 0
```

The base allocation satisfies Kestrel's required arena alignment.

The arena is LIVE.

---

## Failure Postcondition

If initialization cannot be completed:

```
data     == previous data
capacity == previous capacity
offset   == previous offset
```

For legitimate initialization failure from ZERO, this means the arena remains exact ZERO.

---

## Commit Rule

Initialization must conceptually follow:

```
validate
    ↓
calculate aligned capacity
    ↓
allocate temporary backing storage
    ↓
confirm success
    ↓
COMMIT
    ↓
write arena fields
```

The arena fields must not be gradually mutated while failure remains possible.

---

## Duplicate Initialization

Calling `arena_init` on LIVE is misuse.

Why?

Because replacing:

```
arena.data
```

before destroying the previous allocation could lose the original allocation permanently.

Debug:

```
assert
```

Release:

```
return failure
do not mutate arena
```

---

## INVALID-State Initialization

Calling `arena_init` on INVALID is also misuse.

The function must not guess which fields are trustworthy.

Debug:

```
assert
```

Release:

```
return failure
do not mutate arena
```

---

# 9. `arena_alloc` Contract

## Responsibility

Reserve a contiguous region within an already-LIVE arena.

---

## Preconditions

Arena must be LIVE.

Requested allocation size must be non-zero.

Requested alignment must be:

```
non-zero
power of two
<= KESTREL_ALIGNMENT
```

All arithmetic required to align the offset and compute the resulting allocation extent must be overflow-safe.

---

## Conceptual Allocation Procedure

Given:

```
current_offset
requested_size
requested_alignment
```

the allocator conceptually determines:

```
aligned_offset
```

then verifies:

```
aligned_offset + requested_size <= capacity
```

using overflow-safe arithmetic.

Only after every check passes may the allocator commit:

```
offset = allocation_end
```

---

## Successful Postcondition

Return value identifies a valid region beginning at:

```
data + aligned_offset
```

The returned address satisfies the requested alignment.

The new arena state remains LIVE.

Offset advances to the end of the allocation.

---

## Failure Postcondition

For:

- out-of-memory;
    
- invalid alignment;
    
- zero-sized request;
    
- arithmetic overflow;
    
- other recoverable allocation rejection;
    

the entire arena state remains unchanged:

```
data     unchanged
capacity unchanged
offset   unchanged
```

---

## Exact-Fit Allocation

This is legal:

```
allocation_end == capacity
```

Afterward:

```
offset == capacity
```

The arena remains LIVE.

Further non-zero allocations must fail unless the frontier is rolled back/reset.

---

# 10. Allocation Alignment Contract

Every returned allocation must begin at an address satisfying the requested alignment.

Supported alignment must never exceed:

```
KESTREL_ALIGNMENT
```

currently:

```
64 bytes
```

An aligned arena base alone does not prove every returned allocation is correctly aligned.

Each allocation must calculate its own aligned start.

---

# 11. Overflow Contract

No allocation calculation may rely on integer wraparound.

Potentially dangerous expressions include conceptual forms such as:

```
offset + alignment - 1
```

and:

```
aligned_offset + size
```

Before addition, the implementation must establish that the result is representable by `size_t`.

General pattern:

```
if addition would overflow:
    fail
else:
    perform addition
```

Arithmetic safety occurs **before** bounds checking.

A wrapped value must never be allowed to appear valid merely because it became numerically small.

---

# 12. `arena_get_mark` Contract

## Responsibility

Capture the arena's current allocation frontier.

---

## Preconditions

Arena must be LIVE.

---

## Successful Result

Return:

```
current offset
```

The arena is not modified.

---

## Important Property

A legitimate mark may equal:

```
0
```

Therefore:

```
0
```

must never be interpreted universally as:

```
invalid mark
```

---

## Invalid Usage

Calling `arena_get_mark` on an invalid arena state is programmer misuse.

Debug:

```
assert
```

Release:

```
return defensive value
```

The current defensive value may be zero, but that value is not an error sentinel.

---

# 13. `arena_pop_to_mark` Contract

## Responsibility

Rollback the allocation frontier to a previously valid point.

---

## Preconditions

Arena must be LIVE.

Mark must satisfy:

```
mark <= current offset
```

---

## Valid Cases

### Earlier Mark

```
mark < offset
```

Result:

```
offset = mark
```

Allocations created after the mark become logically dead.

---

### Current Mark

```
mark == offset
```

Result:

```
no-op
```

This is valid.

---

## Invalid / Stale Mark

```
mark > offset
```

must never produce:

```
offset = mark
```

because that would move the allocator forward.

Debug:

```
assert
```

Release:

```
leave arena unchanged
```

---

# 14. Mark Lifetime

Marks are meaningful only relative to the allocation history from which they were captured.

Example:

```
offset = 500
mark = 500

arena_reset()

offset = 0
```

The old mark must not now be used to resurrect:

```
offset = 500
```

Reset logically ended that allocation history.

The numeric value still exists.

Its previous lifetime meaning does not.

---

# 15. `arena_reset` Contract

## Responsibility

Discard every current arena allocation while retaining backing storage.

---

## Preconditions

Arena must be LIVE.

---

## Postcondition

```
data     unchanged
capacity unchanged
offset   = 0
```

Arena remains LIVE.

---

## Lifetime Effect

Every allocation created before reset becomes logically invalid.

Example:

```
ptr = arena_alloc(...)
arena_reset(...)
```

The numeric address in `ptr` may still point into allocated RAM.

But Kestrel must treat the allocation as dead.

A future allocation may reuse the same bytes immediately.

---

# 16. `arena_destroy` Contract

## Responsibility

End arena ownership of backing host memory.

---

## `NULL`

```
arena_destroy(NULL)
```

is a safe no-op.

---

## ZERO

Destroying ZERO is a safe no-op.

State remains:

```
NULL
0
0
```

---

## LIVE

Destroy underlying backing allocation.

Then restore exact ZERO:

```
data     = NULL
capacity = 0
offset   = 0
```

---

## INVALID

INVALID destruction is programming misuse.

The implementation cannot safely infer which pieces of the corrupted state are trustworthy.

Debug:

```
assert
```

Release:

```
safe no-op
```

No speculative free should occur.

---

# 17. Idempotent Cleanup

This sequence is valid:

```
arena_destroy(&arena)
arena_destroy(&arena)
```

First call:

```
LIVE → ZERO
```

Second call:

```
ZERO → ZERO
```

This property simplifies cleanup/error paths.

---

# 18. Reinitialization After Destruction

This sequence is valid:

```
ZERO
 ↓
arena_init
 ↓
LIVE
 ↓
arena_destroy
 ↓
ZERO
 ↓
arena_init
 ↓
LIVE
```

Destruction fully restores the initialization precondition.

---

# 19. Debug vs Release Contract

Assertions exist to expose programming mistakes.

They are not the implementation's only safety mechanism.

## Debug

Misuse should fail loudly where appropriate:

```
assert
diagnostic
abort
```

Examples:

- initializing LIVE;
    
- operating on impossible arena state;
    
- stale/future pop mark.
    

---

## Release

Because assertions may disappear:

```
NDEBUG
```

the same misuse must not turn into:

- memory corruption;
    
- allocator advancement;
    
- accidental free;
    
- memory leak caused by state replacement;
    
- undefined state mutation.
    

Release behavior must remain conservative.

General rule:

```
detect misuse
    ↓
assert in debug
    ↓
safe return/no-op in release
```

---

# 20. Mutation Boundaries

The following operations may modify arena fields.

## `arena_init`

May modify:

```
data
capacity
offset
```

but only at successful commit.

---

## `arena_alloc`

May modify only:

```
offset
```

and only on success.

---

## `arena_get_mark`

Must modify:

```
nothing
```

---

## `arena_pop_to_mark`

May modify only:

```
offset
```

and only when the mark is valid.

---

## `arena_reset`

May modify only:

```
offset
```

---

## `arena_destroy`

On LIVE, modifies:

```
data
capacity
offset
```

to restore exact ZERO.

---

# 21. State-Preservation Matrix

|Failure / Misuse|`data`|`capacity`|`offset`|
|---|---|---|---|
|failed init|unchanged|unchanged|unchanged|
|duplicate init|unchanged|unchanged|unchanged|
|invalid-state init|unchanged|unchanged|unchanged|
|OOM allocation|unchanged|unchanged|unchanged|
|invalid alignment|unchanged|unchanged|unchanged|
|zero-size allocation|unchanged|unchanged|unchanged|
|overflow rejection|unchanged|unchanged|unchanged|
|stale/future pop|unchanged|unchanged|unchanged|
|invalid destroy|unchanged|unchanged|unchanged|

This table is deliberately strict.

Failure means:

> no partial allocator mutation.

---

# 22. Ownership Contract

While LIVE:

```
arena owns data
```

The caller must not independently free the arena backing allocation.

Individual pointers returned by `arena_alloc` do not own their memory.

Their lifetime is subordinate to:

```
arena lifetime
```

and:

```
current arena allocation epoch
```

A returned allocation ends logically when:

- the arena rolls back past it;
    
- the arena resets;
    
- the arena is destroyed.
    

---

# 23. What `arena_alloc` Does Not Provide

The allocator does not track:

- object type;
    
- destructor;
    
- tensor metadata;
    
- individual allocation size after allocation;
    
- ownership references;
    
- alias count;
    
- device residency;
    
- backend identity.
    

Those belong to higher layers.

The arena answers only:

> Can this aligned byte region be reserved from this allocation domain?

---

# 24. Test-to-Contract Map

Tests are not just examples.

Each test exists to prove a specific architectural property.

---

## Initialization Test

Proves:

```
ZERO → LIVE
```

and verifies valid backing allocation.

Supports:

```
INV-A1
INV-A2
INV-A3
INV-A4
```

---

## Capacity Alignment Test

Proves the arena backing extent follows the required alignment policy.

Supports the 64-byte host-arena contract.

---

## Base / Allocation Alignment Test

Proves returned allocation addresses satisfy requested alignments.

Ensures alignment is enforced per allocation rather than merely assumed from the arena base.

---

## Non-Overlap Test

Allocate multiple regions and verify their reserved extents do not overlap.

Proves monotonic frontier advancement.

---

## Exact-Fit Test

Allocate exactly to:

```
offset == capacity
```

Proves capacity boundary semantics use:

```
<=
```

rather than incorrectly rejecting a legal exact fit.

---

## OOM Test

Request allocation beyond available capacity.

Expected:

```
failure
```

and:

```
data unchanged
capacity unchanged
offset unchanged
```

Proves transactional allocation failure.

---

## Invalid Alignment Test

Use unsupported/non-power-of-two alignment.

Expected:

```
failure
```

with all state unchanged.

---

## Zero-Size Test

Request zero bytes.

Expected rejection with unchanged state.

This prevents ambiguous zero-length allocation semantics from entering the arena contract.

---

## Overflow Tests

Exercise values near:

```
SIZE_MAX
```

Expected:

```
safe rejection
```

before arithmetic wraps.

Proves arithmetic validation precedes bounds reasoning.

---

## Mark Capture Test

Proves:

```
arena_get_mark() == current offset
```

without modifying arena state.

---

## Pop / Reuse Test

Allocate:

```
A
B
```

capture mark, allocate temporary memory, pop, then allocate again.

Proves rolled-back storage can be reused.

---

## `mark == offset` Test

Proves current-position rollback is legal and is a no-op.

Protects against incorrectly defining all non-decreasing comparisons as failure.

---

## Future/Stale Mark Test

Attempt:

```
mark > current offset
```

Expected:

```
debug assertion
```

or release-safe no-op.

Proves marks cannot move allocator state forward.

---

## Reset Test

Expected transition:

```
LIVE(offset > 0)
    ↓
LIVE(offset = 0)
```

while preserving:

```
data
capacity
```

---

## Reset Reuse Test

Allocate after reset and verify allocation begins again from the reusable start of the arena.

Proves reset is bulk lifetime termination rather than backing-storage destruction.

---

## Destroy Test

Proves:

```
LIVE → ZERO
```

and exact ZERO restoration:

```
NULL
0
0
```

---

## Double Destroy Test

Proves destruction is idempotent:

```
LIVE → ZERO → ZERO
```

---

## Destroy `NULL` Test

Proves cleanup code can safely call:

```
arena_destroy(NULL)
```

without special caller-side branching.

---

## Reinitialization Test

Proves:

```
init
destroy
init
```

is legal because destruction restores exact ZERO.

---

## Failed Initialization Preservation Test

Force initialization failure.

Expected:

```
ZERO → ZERO
```

with all fields unchanged.

Proves initialization has a real commit point.

---

## Failed Allocation Preservation Tests

For each failure mode, capture:

```
old_data
old_capacity
old_offset
```

and verify all three remain identical afterward.

This is stronger than testing only the returned error value.

---

## Release-Only INVALID Arena Attack

Construct deliberately malformed arena state while assertions are disabled.

Verify operation fails safely.

Proves release correctness does not depend on assertions.

---

## Release-Only LIVE Reinitialization Attack

Attempt to initialize an already-LIVE arena in release configuration.

Expected:

```
failure
existing allocation preserved
```

This specifically protects against the leak hazard caused by overwriting `data`.

---

## Release-Only Stale Mark Attack

Attempt to pop using:

```
mark > offset
```

Expected:

```
offset unchanged
```

Proves stale marks cannot manufacture allocation state when assertions disappear.

---

# 25. Acceptance Criteria Proven by KES-001

KES-001 is considered complete because the implementation establishes all of the following:

```
[✓] exact ZERO state exists
[✓] explicit LIVE state exists
[✓] INVALID state is detectable
[✓] initialization requires ZERO
[✓] failed init is transactional
[✓] duplicate init cannot overwrite ownership
[✓] allocation validates lifecycle
[✓] allocation validates alignment
[✓] allocation protects arithmetic from overflow
[✓] failed allocation preserves complete state
[✓] exact-fit allocation is legal
[✓] marks represent offsets
[✓] current mark is a valid no-op
[✓] stale marks cannot advance allocator
[✓] reset preserves storage
[✓] reset terminates prior allocation lifetimes
[✓] destroy restores exact ZERO
[✓] destroy is idempotent
[✓] reinitialization after destroy works
[✓] debug misuse is loud
[✓] release misuse remains safe
```

---

# 26. What Higher Layers May Assume

Code using the arena may rely on:

```
valid allocations begin correctly aligned
```

```
successful allocations never overlap
```

```
failed allocations do not alter allocator state
```

```
marks never move the arena forward
```

```
reset retains backing storage
```

```
destroy returns exact ZERO
```

```
release builds retain safety checks
```

Higher layers must **not** assume:

```
arena pointers survive reset
```

```
individual arena allocations can be freed
```

```
host arena semantics automatically represent GPU memory
```

```
64-byte arena alignment automatically produces SIMD-compatible tensor row layout
```

---

# 27. Boundary With KES-002

KES-001 defines:

```
how one host-memory allocation domain behaves
```

KES-002 must answer:

```
what an allocation domain actually means
in a heterogeneous runtime
```

This is the architectural bridge:

```
KES-001

HOST RAM
   ↓
arena_t
   ↓
aligned byte allocation


              becomes


KES-002

MEMORY SPACE
   ↓
ALLOCATION DOMAIN
   ↓
ALLOCATOR / ARENA
   ↓
STORAGE OBJECT
   ↓
TENSOR VIEW
```

KES-001 therefore remains valid.

KES-002 generalizes the architectural layer above it.

---

# Final Implementation Contract

The Kestrel host arena guarantees:

> Given a correctly initialized LIVE arena, allocation behaves as an aligned, overflow-safe, monotonic bump allocator whose failed operations preserve complete allocator state, whose rollback operations can never move the frontier forward, whose reset operation terminates allocation lifetimes without freeing backing storage, and whose destruction operation safely restores exact ZERO state.

That is the contract higher layers are allowed to build upon.