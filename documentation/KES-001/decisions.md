# Texyn Labs — Kestrel Engineering

# Arena Architecture Documentation

**Project:** Kestrel  
**Subsystem:** Memory Management  
**Component:** Host Arena Allocator  
**Architecture Record:** ADR-001  
**Status:** Accepted  
**Implementation Status:** KES-001 Merged

---

# Part I — Concept Notes

## CONCEPT-001 — Why Kestrel Uses an Arena

### Definition

An **arena allocator** reserves a relatively large block of memory and then serves smaller allocations from that block by moving an offset forward.

Instead of repeatedly asking the operating system or C allocator for memory:

```
malloc
malloc
malloc
free
malloc
free
...
```

Kestrel does roughly:

```
Reserve large region

┌─────────────────────────────────────────────┐
│                                             │
└─────────────────────────────────────────────┘
^

offset = 0
```

An allocation advances the offset:

```
┌──────────────┬──────────────────────────────┐
│ allocation A │                              │
└──────────────┴──────────────────────────────┘
               ^
             offset
```

Another allocation:

```
┌──────────────┬───────────┬──────────────────┐
│ allocation A │ alloc. B  │                  │
└──────────────┴───────────┴──────────────────┘
                           ^
                         offset
```

Individual allocations are not normally freed.

Instead, the arena can discard many allocations at once by moving its offset backward.

---

## Why Kestrel Wants This

Machine-learning workloads produce many allocations whose lifetimes are naturally grouped.

Examples include:

- tensor payloads;
    
- temporary operation buffers;
    
- graph nodes;
    
- gradients;
    
- scratch memory;
    
- intermediate activations.
    

An arena gives Kestrel:

- cheap allocation;
    
- predictable allocation behavior;
    
- good locality;
    
- bulk lifetime management;
    
- very little allocator metadata;
    
- explicit control over alignment;
    
- a useful foundation for performance-sensitive systems work.
    

The arena is therefore deliberately simple.

It is **not** intended to become a general-purpose replacement for `malloc`.

---

# CONCEPT-002 — Bump Allocation

The Kestrel arena is a **bump allocator**.

Its essential state is:

```
base pointer
capacity
offset
```

Conceptually:

```
data
 ↓
┌─────────────────────────────────────────────┐
│ used memory             │ free memory       │
└─────────────────────────────────────────────┘
                          ^
                        offset

<--------------- capacity ------------------->
```

To allocate `N` bytes:

1. align the current offset;
    
2. verify that the allocation fits;
    
3. return the corresponding address;
    
4. advance the offset.
    

There is no search for free blocks.

There is no free-list.

There is no coalescing.

That is why bump allocation is extremely inexpensive.

---

# CONCEPT-003 — Arena State Is More Than a Pointer

One of the most important lessons from KES-001 was that the arena is not simply:

```
data != NULL → initialized
```

The complete state matters.

Kestrel defines three lifecycle states.

## ZERO

```
data     == NULL
capacity == 0
offset   == 0
```

This is the only valid uninitialized state.

---

## LIVE

```
data     != NULL
capacity > 0
offset   <= capacity
```

The arena owns a valid allocation and can service requests.

---

## INVALID

Anything else.

Examples:

```
data == NULL
capacity == 1024
offset == 0
```

or:

```
data != NULL
capacity == 0
```

or:

```
offset > capacity
```

These states violate the arena contract.

---

## Why Explicit States Matter

Without a state model, functions tend to accumulate local assumptions.

For example:

```
arena_init assumes data == NULL
arena_alloc assumes capacity > 0
arena_destroy assumes pointer validity
arena_reset assumes previous initialization
```

Those assumptions may disagree.

By defining the legal states globally, every function reasons from the same invariant.

That converts vague expectations into a state machine.

---

# CONCEPT-004 — Invariants

An **invariant** is a condition that must remain true whenever an object is in a valid state.

For Kestrel's arena:

```
ZERO:
data == NULL
capacity == 0
offset == 0
```

and:

```
LIVE:
data != NULL
capacity > 0
offset <= capacity
```

An operation may temporarily compute values internally, but it must either:

```
valid state → valid state
```

or fail without corrupting the original valid state.

This is much stronger than:

> “The function seems to work.”

---

# CONCEPT-005 — Transactional Failure

A major design requirement is that failed operations must not leave partially modified state.

Consider arena initialization.

Bad sequence:

```
capacity = requested_size
data = allocate(...)
```

If allocation fails after `capacity` changes:

```
data == NULL
capacity > 0
```

The arena is now INVALID.

Instead, compute everything into temporary variables first:

```
validate
calculate
allocate
```

and only after everything succeeds perform the state transition.

Conceptually:

```
OLD STATE
    │
    │ prepare privately
    │
    ├── validation fails ───→ OLD STATE
    │
    ├── allocation fails ───→ OLD STATE
    │
    ▼
commit
    │
    ▼
NEW STATE
```

The point where state actually changes is the **commit point**.

This idea appears throughout systems engineering:

- transactions;
    
- filesystem updates;
    
- database commits;
    
- resource acquisition;
    
- compiler transformations;
    
- runtime state transitions.
    

---

# CONCEPT-006 — Alignment

Kestrel requires the arena base and arena allocations to respect alignment constraints.

The current fundamental arena alignment is:

```
64 bytes
```

Why?

Modern processors access memory most efficiently when data begins at suitable boundaries.

Later SIMD implementations also have useful alignment requirements.

A 64-byte baseline provides:

- cache-line alignment on common modern CPUs;
    
- ample alignment for AVX2;
    
- headroom for wider SIMD such as AVX-512;
    
- predictable tensor storage alignment.
    

However:

> Arena alignment does not automatically guarantee that every row inside a tensor is SIMD-aligned.

For example:

```
tensor base = 64-byte aligned
row 0       = aligned
row 1       = base + 20 bytes
```

Row 1 is not necessarily suitably aligned.

Therefore:

```
arena alignment
```

and:

```
tensor physical layout / row pitch
```

are separate architectural problems.

This distinction later helped expose the problem with universally padding tensors for AVX2.

---

# CONCEPT-007 — Alignment Requirements

Kestrel permits an allocation alignment only when it is valid.

An alignment must:

```
be non-zero
be a power of two
not exceed KESTREL_ALIGNMENT
```

Power-of-two alignment matters because common alignment arithmetic relies on binary boundaries.

For alignment `A`:

```
A = 1, 2, 4, 8, 16, 32, 64 ...
```

not:

```
3, 6, 10, 24 ...
```

---

# CONCEPT-008 — Integer Overflow Is a Memory-Safety Problem

Alignment arithmetic often looks harmless:

```
value + alignment - 1
```

But if `value` is close to `SIZE_MAX`, the addition can wrap around.

Example:

```
SIZE_MAX - 2
+ 63
---------
wraps
```

The resulting number may suddenly become small.

A bounds check performed afterward could then incorrectly succeed.

Therefore Kestrel checks that the addition itself is legal before performing it.

General principle:

> Validate before performing potentially overflowing arithmetic.

This rule applies far beyond the arena:

- tensor element counts;
    
- tensor byte sizes;
    
- stride calculations;
    
- buffer offsets;
    
- network packet sizes;
    
- file offsets.
    

---

# CONCEPT-009 — Marks

A mark represents a previous arena offset.

Conceptually:

```
offset = 0

allocate A
allocate B

mark = current offset

allocate C
allocate D
```

Memory:

```
┌─────┬─────┬─────┬─────┬──────────────┐
│  A  │  B  │  C  │  D  │              │
└─────┴─────┴─────┴─────┴──────────────┘
            ^
           mark
```

Popping to the mark performs:

```
offset = mark
```

Conceptually discarding C and D together.

---

## Why Marks Are Offsets

Kestrel uses:

```
arena_mark_t = size_t
```

The mark does not need to be a pointer.

The arena already knows its base address.

Therefore the essential information is simply:

```
how far from the arena base was the allocation frontier?
```

---

# CONCEPT-010 — Valid and Stale Marks

A mark is valid when:

```
mark <= current_offset
```

`mark == current_offset` is valid.

It simply means:

```
pop to where I already am
```

which is a no-op.

But:

```
mark > current_offset
```

must never advance the allocator.

Example:

```
mark = 500

later:
reset arena

offset = 0
```

The old mark now represents memory from a previous allocation lifetime.

If Kestrel allowed:

```
offset = 500
```

the stale mark would move the allocator forward into memory that has no valid current allocation history.

Therefore stale/future marks are programming violations.

---

# CONCEPT-011 — Reset Ends Allocation Lifetimes

`arena_reset` performs:

```
offset → 0
```

while preserving:

```
data
capacity
```

It does not free the underlying arena.

Conceptually:

```
before:

┌─────┬─────┬─────┬────────────────────┐
│  A  │  B  │  C  │                    │
└─────┴─────┴─────┴────────────────────┘
                  ^
                offset


after reset:

┌────────────────────────────────────────┐
│ reusable memory                        │
└────────────────────────────────────────┘
^
offset
```

Every previous allocation must now be considered dead.

A raw pointer may still contain an address, but its logical lifetime has ended.

That distinction becomes important later when tensors reference arena-backed storage.

---

# CONCEPT-012 — Reset vs Pop-to-Zero

These operations may produce the same numeric result:

```
offset = 0
```

but their meaning is slightly different.

`arena_reset()` explicitly communicates:

> Discard all current allocations.

`arena_pop_to_mark(arena, 0)` means:

> Return to a legitimate previously captured mark whose value happens to be zero.

A valid mark of zero is allowed.

Therefore:

```
0
```

cannot be treated as an error sentinel.

---

# CONCEPT-013 — Debug Assertions vs Release Safety

Kestrel uses assertions to loudly expose programmer mistakes during development.

Example misuse:

```
initialize already-live arena
```

In debug:

```
assert
abort
show file + line
```

That's useful.

But assertions disappear under `NDEBUG`.

Therefore this is dangerous:

```
assert(valid);
perform dangerous operation anyway;
```

because release builds would lose the only protection.

Kestrel's rule is:

> Assertions diagnose programming violations. They do not provide release-mode safety.

So functions must still behave defensively after detecting invalid state.

Example:

```
debug:
    assertion fires

release:
    safe failure
    state remains unchanged
```

This distinction became one of the central design rules of KES-001.

---

# CONCEPT-014 — Programming Error vs Recoverable Runtime Failure

Not every failure has the same meaning.

Example:

```
arena allocation does not fit
```

is a legitimate runtime failure.

The caller may recover.

But:

```
arena has impossible internal state
```

indicates a programming error.

Similarly:

```
attempting to initialize an already-live arena
```

means the lifecycle contract has been violated.

Kestrel therefore distinguishes:

```
RECOVERABLE FAILURE
```

from:

```
PROGRAMMING CONTRACT VIOLATION
```

Programming violations should be loud in debug builds while remaining safe in release builds.

---

# CONCEPT-015 — Idempotent Destruction

An operation is **idempotent** when repeating it leaves the system in the same state as performing it once.

For arena destruction:

```
destroy(arena)
destroy(arena)
```

must be safe.

After the first successful destruction:

```
data     = NULL
capacity = 0
offset   = 0
```

The second call sees ZERO and does nothing.

This property simplifies cleanup paths because callers do not need elaborate bookkeeping merely to avoid a second destroy.

---

# Part II — ADR-001

# ADR-001 — Explicit Arena Lifecycle and Transactional Failure Contract

## Status

**Accepted**

Implemented and validated by **KES-001**.

---

# Context

Kestrel depends on arena allocation as a foundational memory primitive.

The original allocator was simple enough to allocate memory but did not completely define:

- what constitutes initialized state;
    
- what constitutes destroyed state;
    
- whether duplicate initialization is legal;
    
- what happens after failed initialization;
    
- whether invalid alignment may alter allocator state;
    
- whether stale marks may move the allocator;
    
- what reset means for allocation lifetime;
    
- what release builds do when assertions disappear;
    
- whether destruction is idempotent.
    

These are not cosmetic details.

Tensor allocation, graph construction, scratch memory and future runtime memory systems will rely on arena behavior.

Therefore the arena required a precise lifecycle and failure contract before higher-level systems could safely build on it.

---

# Decision

Kestrel's host arena is governed by an explicit three-state lifecycle.

## ZERO

```
data     == NULL
capacity == 0
offset   == 0
```

## LIVE

```
data     != NULL
capacity > 0
offset   <= capacity
```

## INVALID

Anything else.

Only ZERO and LIVE are legal persistent states.

---

# Initialization Contract

`arena_init` accepts only an arena in exact ZERO state.

Legal transition:

```
ZERO
 ↓
arena_init succeeds
 ↓
LIVE
```

Failure:

```
ZERO
 ↓
arena_init fails
 ↓
ZERO
```

Initialization failure must be transactional.

No arena field may be partially modified.

Calling `arena_init` on LIVE or INVALID state is a programming contract violation.

Debug behavior:

```
assert loudly
```

Release behavior:

```
return failure
preserve existing state
```

This prevents accidental reinitialization from leaking an existing arena allocation.

---

# Allocation Contract

Allocation is valid only on a LIVE arena.

The allocator must verify:

- arena state;
    
- non-zero request size;
    
- requested alignment;
    
- alignment is a power of two;
    
- alignment does not exceed the arena guarantee;
    
- alignment arithmetic does not overflow;
    
- allocation size arithmetic does not overflow;
    
- the requested allocation fits inside remaining capacity.
    

Only after every check succeeds may the arena advance its offset.

Therefore failed allocation preserves:

```
data
capacity
offset
```

exactly.

---

# Alignment Contract

Arena base alignment:

```
KESTREL_ALIGNMENT = 64 bytes
```

Requested allocation alignment must be:

```
> 0
power of two
<= KESTREL_ALIGNMENT
```

Arena alignment guarantees allocation start alignment.

It does not define tensor row pitch or physical tensor layout.

Those are separate layers.

---

# Mark Contract

A mark represents an arena offset.

```
arena_mark_t = size_t
```

A mark is valid when:

```
mark <= current offset
```

`mark == current offset` is legal and produces a no-op.

A mark greater than the current offset is stale or invalid.

Such a mark must never move the allocator forward.

Debug:

```
assert
```

Release:

```
ignore safely
leave state unchanged
```

---

# `arena_get_mark` Contract

On a valid LIVE arena:

```
return current offset
```

Invalid arena usage is a programming violation.

A defensive release value may be `0`, but:

> `0` is not an error code.

Zero is a legitimate mark.

---

# Reset Contract

`arena_reset` is valid only on a LIVE arena.

It performs:

```
offset = 0
```

while preserving:

```
data
capacity
```

Reset logically ends the lifetime of every previous allocation from the arena.

The underlying storage remains owned by the arena and is immediately reusable.

---

# Destruction Contract

`arena_destroy(NULL)`:

```
safe no-op
```

Destroying ZERO:

```
safe no-op
```

Destroying LIVE:

```
free storage
 ↓
restore exact ZERO state
```

Destroying INVALID:

```
programming violation
```

Debug:

```
assert
```

Release:

```
safe no-op
```

After successful destruction, the arena may be initialized again.

---

# Why No `initialized` Boolean Exists

An alternative considered was:

```
bool initialized;
```

This was rejected.

The existing fields already encode lifecycle state.

Adding:

```
initialized
```

would create duplicate truth.

For example:

```
initialized = false
data != NULL
capacity > 0
```

Which source is authoritative?

Every additional lifecycle flag creates another invariant that every function must maintain.

Therefore Kestrel uses the arena's actual state as the lifecycle representation.

---

# Why Duplicate Initialization Is Rejected

Allowing:

```
arena_init(&arena, ...)
arena_init(&arena, ...)
```

without destruction would risk losing the first allocation.

Conceptually:

```
arena.data → allocation A

second init

arena.data → allocation B

allocation A lost
```

That is a memory leak.

Therefore initialization requires exact ZERO.

---

# Why Allocation Failure Must Preserve All Fields

A failed operation should not leave the caller asking:

> Which parts of the allocator changed before it failed?

Transactional failure gives the much stronger rule:

```
success → complete state transition

failure → original state
```

This dramatically simplifies reasoning and testing.

---

# Why Marks Cannot Move Forward

Arena marks represent previous allocation frontiers.

Their purpose is rollback.

Allowing:

```
mark > offset
```

would make a rollback operation capable of creating allocation state.

That violates the meaning of a mark.

Therefore:

```
pop_to_mark
```

may only preserve or decrease the offset.

Never increase it.

---

# Why Release Builds Still Check Invariants

Debug assertions are development diagnostics.

They are compiled out in release configurations.

Therefore:

```
assert(condition)
```

cannot be the only thing standing between bad input and memory corruption.

Every public/internal arena operation must still perform sufficient runtime validation to avoid unsafe state transitions after assertions disappear.

---

# Why 64-Byte Alignment Was Chosen

Kestrel's future compute stack includes SIMD and performance-sensitive tensor kernels.

A 64-byte aligned arena:

- aligns naturally with common cache-line boundaries;
    
- comfortably satisfies AVX2 requirements;
    
- leaves room for wider SIMD;
    
- gives downstream allocations a predictable alignment ceiling.
    

This is a storage-allocation guarantee.

It deliberately does not encode SIMD-specific tensor layouts.

---

# Invariants

The following are frozen architectural invariants.

### INV-A1

Every valid arena is exactly ZERO or LIVE.

### INV-A2

ZERO is represented only by:

```
NULL, 0, 0
```

### INV-A3

LIVE requires:

```
data != NULL
capacity > 0
offset <= capacity
```

### INV-A4

`arena_init` accepts only exact ZERO.

### INV-A5

Failed initialization does not mutate state.

### INV-A6

Failed allocation does not mutate state.

### INV-A7

An arena mark can never cause the offset to increase.

### INV-A8

Reset preserves storage while invalidating previous allocation lifetimes.

### INV-A9

Successful destruction returns the arena to exact ZERO.

### INV-A10

Destroying ZERO is safe.

### INV-A11

Assertions are diagnostic and are never relied upon for release safety.

### INV-A12

Arena alignment and tensor physical layout are separate concerns.

---

# Consequences

## Benefits

The design provides:

- deterministic lifecycle behavior;
    
- leak-resistant initialization;
    
- transactional failures;
    
- predictable alignment;
    
- cheap allocation;
    
- cheap bulk deallocation;
    
- safe cleanup;
    
- clean testing;
    
- reliable behavior when assertions disappear;
    
- a strong base for tensors and runtime storage.
    

---

## Costs

The arena deliberately does not support:

- freeing arbitrary individual allocations;
    
- reallocating one object;
    
- fragmentation recovery;
    
- independent general-purpose heap behavior 

- independent lifetimes inside one arena;
    
- automatic thread safety.
    

Callers must therefore choose arena lifetimes deliberately.

---

# Testing Strategy

KES-001 validates architecture rather than merely checking happy-path outputs.

Required tests include:

- initialization;
    
- capacity alignment;
    
- allocation alignment;
    
- non-overlapping allocations;
    
- exact-fit allocation;
    
- out-of-memory failure;
    
- invalid alignment;
    
- zero-sized allocation rejection;
    
- `SIZE_MAX` overflow cases;
    
- marks;
    
- pop/reuse behavior;
    
- `mark == offset`;
    
- stale/future mark rejection;
    
- reset;
    
- reset storage reuse;
    
- destroy;
    
- double destroy;
    
- destroy `NULL`;
    
- reinitialization after destruction;
    
- failed initialization preserving ZERO;
    
- failed allocation preserving every arena field;
    
- release-only invalid-state attacks;
    
- release-only duplicate initialization attempts.
    

The important testing principle is:

> Test invariants and forbidden state transitions, not only successful return values.

---

# Architectural Relationship to Future Kestrel

The arena is the first layer of Kestrel's memory architecture.

Conceptually:

```
HOST MEMORY
     │
     ▼
HOST ARENA
     │
     ▼
STORAGE ALLOCATIONS
     │
     ▼
TENSORS / GRAPH / SCRATCH
```

However, KES-001 does **not** establish that every future Kestrel memory system must be represented by the existing `arena_t`.

As Kestrel evolves toward heterogeneous execution, other memory domains may require different native mechanisms:

```
host RAM
integrated/shared memory
CUDA device memory
Vulkan buffers/device memory
future accelerator memory
```

The architectural lesson carried forward from KES-001 is therefore broader than the concrete C implementation:

> Every Kestrel allocation domain must have explicit lifecycle, ownership, alignment, failure and lifetime semantics.

The host arena is the first implementation of that philosophy.

---

# Engineering Lessons From KES-001

KES-001 introduced several systems-engineering ideas that should recur throughout Kestrel.

## 1. State should be explicit

Do not let legal states emerge accidentally from implementation details.

Define them.

---

## 2. Invalid states matter

Designing only the happy path leaves the dangerous behavior unspecified.

Ask:

```
What happens if this is called twice?
What happens after failure?
What happens with stale state?
What happens in release mode?
```

---

## 3. Failure is part of the API

A function's contract includes both:

```
what happens when it succeeds
```

and:

```
what remains true when it fails
```

---

## 4. Prefer one source of truth

Do not introduce duplicated lifecycle metadata unless there is a strong reason.

---

## 5. Debug diagnostics and runtime correctness are different

Assertions help engineers find bugs.

Checks keep software safe.

Kestrel needs both.

---

## 6. Memory lifetime is logical, not merely physical

A pointer can still numerically point into valid RAM even though the object's allowed lifetime has ended.

Arena reset demonstrates this clearly.

---

## 7. Architecture should expose boundaries

The arena guarantees allocation alignment.

Tensor layout determines internal organization.

SIMD kernels determine execution requirements.

Those concerns interact, but they should not be collapsed into one abstraction.

---

# Final Architecture Summary

The Kestrel host arena is:

> A 64-byte-aligned bump allocator with explicit ZERO/LIVE lifecycle states, transactional initialization and allocation failure, monotonic rollback marks, bulk reset semantics, idempotent destruction and release-safe invariant enforcement.

Its purpose is not merely to allocate memory quickly.

Its purpose is to provide Kestrel with a **predictable memory-lifetime primitive whose behavior can be reasoned about formally by every system built above it.**