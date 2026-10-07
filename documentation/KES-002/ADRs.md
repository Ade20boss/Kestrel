
# Texyn Labs — Kestrel Architecture Decision Records

## ADR-002 — Device, Backend, and Backend Instance

**Status:** Accepted  
**Context:** Kestrel must execute across CPUs, integrated GPUs, discrete GPUs, and future accelerators without confusing hardware with execution APIs.

### Decision

Kestrel separates:

```
DEVICE
Physical compute hardware.

BACKEND FAMILY
An implementation/API used to execute operations.

BACKEND INSTANCE
One backend family attached to one particular device.
```

Examples:

```
Devices:
CPU
Intel Iris Xe
NVIDIA T4

Backend families:
Scalar
AVX2
Vulkan
CUDA

Backend instances:
Scalar + CPU
AVX2 + CPU
Vulkan + Iris Xe
Vulkan + T4
CUDA + T4
```

### Why

One physical device can support several execution mechanisms.

For example:

```
NVIDIA GPU
├── CUDA
└── Vulkan
```

Calling CUDA and Vulkan separate devices would duplicate the same hardware and make hardware discovery, memory topology, and planning incorrect.

### Consequence

The planner selects a **backend instance**, not simply `"CUDA"` or `"GPU"`.

---

# ADR-003 — Memory Spaces and Physical Accessibility

**Status:** Accepted

### Problem

Kestrel needs to distinguish:

> where computation occurs

from:

> where data physically lives.

### Decision

Introduce a **memory space** abstraction.

Examples:

```
SYSTEM_SHARED_RAM
NVIDIA_LOCAL_VRAM
```

A memory space describes things such as:

```
identity
classification
capacity
which physical devices can access it
```

Example:

```
SYSTEM RAM
↑          ↑
CPU      Iris Xe
```

while:

```
T4 VRAM
   ↑
NVIDIA T4
```

### Important distinction

```
Device can access memory space
```

does **not** necessarily mean:

```
every backend on that device can consume every allocation in that space.
```

Storage representation/API compatibility is a separate issue.

### Rejected

We rejected making separate memory spaces merely because CUDA and Vulkan use different allocation APIs.

They may operate on the same underlying physical VRAM while managing separate pools.

---

# ADR-004 — Allocator and Storage Architecture

**Status:** Accepted

### Decision

The hierarchy is:

```
MEMORY SPACE
     ↑
ALLOCATOR / POOL
     ↓
STORAGE
```

An allocator manages a region/pool inside exactly one memory space.

Examples:

```
SYSTEM RAM
├── host arena
└── Vulkan host-visible pool

T4 VRAM
├── CUDA pool
└── Vulkan pool
```

### Allocator responsibility

Allocator understands:

```
bytes
alignment
allocation mechanics
lifetime mechanics
provider-specific native memory details
```

Allocator does **not** understand:

```
tensor shape
dtype semantics
tensor strides
AVX2 padding
matmul
graph operations
planner policy
```

The request should effectively be:

```
allocate N bytes with alignment X
```

not:

```
allocate an FP32 AVX2 tensor
```

---

## Storage

`storage_t` represents one physical allocation.

Conceptually:

```
storage
├── byte_size
├── memory_space
├── allocator/provider
└── opaque allocation token
```

The allocation token is deliberately opaque.

For example:

```
Host allocator
token → host allocation information

CUDA allocator
token → CUDA allocation information

Vulkan allocator
token → Vulkan allocation information
```

Only the provider that created the token interprets it.

### Why

We explicitly reject a core object like:

```
storage
├── host_ptr
├── cuda_ptr
├── VkBuffer
├── VkDeviceMemory
└── ...
```

because every new backend would require modifying the core storage representation.

### Arena relationship

KES-001 `arena_t` remains one concrete host allocation mechanism.

GPU allocators are **not required to imitate arena mark/pop semantics**.

---

# ADR-005 — Tensor Storage and View Model

**Status:** Accepted

### Decision

A tensor is a **logical view over storage**.

Conceptually:

```
tensor
├── ndim
├── shape[]
├── stride_bytes[]
├── offset_bytes
├── dtype
└── storage
```

### Tensor does not contain

```
device
backend
host pointer
CUDA pointer
Vulkan resource
allocator internals
planner state
```

### Why storage exists

This was the fundamental reason we introduced `storage_t`.

Tensor should describe:

> what bytes mean.

Storage/provider layers describe:

> where the bytes are and how the system accesses them.

---

## Views

Views normally share the same storage.

Examples:

```
slice
transpose
reverse
broadcast
```

So:

```
Tensor A ─┐
Tensor B ─┼──→ Storage S
Tensor C ─┘
```

No payload copy is required merely because a new view exists.

### Lifetime

For arena-backed storage:

```
tensor destruction != storage free
```

The arena owns the allocation lifetime.

If `pop`, `reset`, or `destroy` invalidates the allocation, every tensor/view referring to it becomes invalid.

Ordinary arena-backed views therefore do not require reference counting.

---

# ADR-006 — Tensor Strides, Broadcasting and Contiguity

**Status:** Accepted

## Stride representation

Strides are stored in **bytes**.

For FP32:

```
[a b c]
[d e f]
```

has:

```
shape   = [2,3]
strides = [12,4]
```

Meaning:

```
move one row    → 12 bytes
move one column → 4 bytes
```

### Why bytes

Storage is byte-based and dtype-independent.

Kestrel itself normally calculates strides; regular users are not expected to manually calculate byte offsets.

---

## Offset

A tensor view stores `offset_bytes`, describing where it starts inside the backing storage.

This replaces the old approach of modifying a raw `float *data` pointer.

---

## Negative strides

Supported.

Example:

```
[a b c d e]
```

with FP32 reversed as:

```
offset = 16
stride = -4
shape  = [5]
```

gives:

```
e d c b a
```

without copying.

Because negative strides can walk backwards, validation must check both the lowest and highest reachable storage addresses.

---

## Zero strides

Supported for broadcasting.

Suppose storage contains:

```
[10 20 30]
```

A logical view:

```
[10 20 30]
[10 20 30]
```

can use:

```
shape   = [2,3]
strides = [0,4]
```

Moving to another row advances zero bytes.

### Broadcasting

Broadcasting lets Kestrel logically repeat data without physically copying it.

This is important for operations such as:

```
matrix + bias
batch + vector
channel bias
normalization parameters
```

### Consequence

Broadcasted tensors may contain aliasing: multiple logical elements can point to the same physical bytes.

Write semantics must account for that.

---

## Kestrel contiguity

Contiguity is defined by **Kestrel**, not by individual backends.

Simple meaning:

> The meaningful tensor values are packed directly beside each other in normal row-major order with no gaps.

Example:

```
a b c d e f
```

is contiguous.

```
a b c _
d e f _
```

is not contiguous because padding introduces gaps.

A non-contiguous tensor is not inherently bad or slow.

Backend-specific layouts may deliberately be non-contiguous.

Dimensions of size `1` do not affect the contiguity decision because indexing never advances through them.

---

## Backend layout preferences

Backends may advertise layouts they prefer.

Example:

```
AVX2
prefers:
    aligned memory
    SIMD-friendly row spacing
```

But that preference does not alter Kestrel's tensor semantics.

This permanently rejects universal AVX2 row padding.

---

# ADR-007 — Dtype Promotion and Accumulation

**Status:** Accepted

### Core dtype set

```
FP16
BF16
FP32
FP64

INT8
UINT8
INT32

BOOL
```

Sizes:

```
FP16   2 bytes
BF16   2 bytes
FP32   4 bytes
FP64   8 bytes

INT8   1 byte
UINT8  1 byte
INT32  4 bytes

BOOL   1 byte
```

---

## Global numeric semantics

Kestrel defines promotion and accumulation rules.

Backends do not.

Therefore the meaning of:

```
matmul(A, B)
```

must not change depending on whether AVX2, Vulkan, or CUDA executes it.

A backend either:

```
supports the requested Kestrel semantics
```

or:

```
reports unsupported
```

---

## Promotion direction

For ordinary floating-point elementwise operations:

```
FP16 + FP16 → FP16
BF16 + BF16 → BF16
FP32 + FP32 → FP32
FP64 + FP64 → FP64

FP16 + BF16 → FP32
FP16 + FP32 → FP32
BF16 + FP32 → FP32
anything involving FP64 → FP64
```

Integer direction:

```
INT8  + INT8  → INT8
UINT8 + UINT8 → UINT8

INT8  + INT32 → INT32
UINT8 + INT32 → INT32
INT8  + UINT8 → INT32
```

BOOL remains logically separate from ordinary numeric arithmetic.

---

## Accumulation

For accumulation-heavy operations:

```
FP16  → FP32 accumulation
BF16  → FP32 accumulation
FP32  → FP32 accumulation
FP64  → FP64 accumulation

INT8   → INT32 accumulation
UINT8  → INT32 accumulation
INT32  → INT32 accumulation
```

Examples:

```
FP16 matmul
→ FP32 accumulation

INT8 dot product
→ INT32 accumulation
```

### Output policy

When accumulation widens the dtype, the default result preserves the accumulation dtype.

Therefore:

```
FP16 matmul
→ FP32 output by default
```

not:

```
FP16 matmul
→ FP16 automatically
```

A developer may explicitly downcast afterwards.

### Why

Otherwise Kestrel would spend effort preserving precision during accumulation only to silently discard it at output.

---

# ADR-008 — Backend Capability Model

**Status:** Accepted

A backend instance exposes its capabilities to the runtime.

Conceptually this includes:

```
device
supported operations
supported dtype combinations
supported layouts/stride patterns
compatible storage/provider representations
execution properties
hard requirements
performance preferences
```

### Requirement vs preference

A backend must distinguish them.

For example:

```
AVX2 requirement:
CPU-accessible storage.

AVX2 preference:
SIMD-friendly alignment.
```

The first affects legality.

The second affects performance.

---

## Storage compatibility

Two tests exist:

```
DEVICE ↔ MEMORY SPACE
Can the hardware physically reach those bytes?
```

and:

```
BACKEND INSTANCE ↔ STORAGE
Can this execution API consume this specific resource representation?
```

These are not equivalent.

Example:

```
RTX GPU can access RTX VRAM
```

does not automatically mean:

```
CUDA can consume every Vulkan-created allocation in that VRAM.
```

If incompatible, the planner may need transfer, import, materialization, or another conversion path.

---

# ADR-009 — Planner, Performance Model and Expert Control

**Status:** Accepted

## Planner objective

The planner does not merely find a backend that works.

It should choose:

> the most efficient complete execution plan Kestrel currently knows how to produce, subject to correctness constraints.

Correctness comes first.

Performance optimization occurs only among legal plans.

---

## Total plan cost

Planning must eventually account for more than kernel execution time.

Conceptually:

```
TOTAL COST =

compute
+ data movement
+ allocation
+ repacking/layout conversion
+ synchronization
+ launch/dispatch overhead
+ relevant memory pressure
```

Example:

A GPU kernel could be faster than AVX2 by itself.

But for a tiny operation:

```
CPU → GPU transfer
+ GPU launch
+ GPU execution
+ synchronization
+ GPU → CPU transfer
```

may cost more than simply using AVX2.

Therefore:

> Kestrel optimizes the complete execution plan, not isolated kernel speed.

---

## Performance measurements

Kestrel should measure the actual machine rather than assuming:

```
GPU > CPU
CUDA > Vulkan
AVX2 > Scalar
dGPU > iGPU
```

Measurements may include:

```
operation latency
throughput
transfer bandwidth
allocation overhead
dispatch overhead
synchronization cost
layout conversion cost
```

Performance depends on:

```
device
backend
operation
dtype
problem size
layout
memory location
```

---

## Performance profiles

Kestrel can build persistent machine/backend-specific profiles.

Example:

```
Machine: Daniel's laptop

AVX2 + CPU
FP32 matmul:
32x32   ...
64x64   ...
128x128 ...
512x512 ...

Vulkan + Iris Xe
FP32 matmul:
...
```

On Lightning:

```
CUDA + T4
```

receives a different profile.

The profile can later be invalidated when relevant hardware, drivers, kernels, or Kestrel versions change.

---

## Advanced-user visibility

This is now a frozen principle:

> Anything important enough for the planner to inspect should also be available for advanced-user introspection.

There must not be one secret planner model and a separate user model.

Conceptually:

```
              RUNTIME MODEL
              /           \
             /             \
        PLANNER       ADVANCED USER
```

An advanced user should eventually be able to inspect:

```
devices
memory spaces
backend instances
allocators
backend capabilities
storage compatibility
performance measurements
planner estimates
planner choices
```

---

## Control levels

Default:

```
AUTOMATIC
Kestrel chooses.
```

Advanced:

```
CONSTRAINED
CPU only.
No CUDA.
Prefer Iris Xe.
Keep this tensor in system RAM.
```

Expert:

```
FORCED
Use AVX2 + CPU.
```

A forced choice must still satisfy Kestrel semantics.

Impossible requests are rejected rather than silently altered.

---

# KES-002 ADR Status

With ADR-002 through ADR-009 accepted, the architecture chain is now:

```
KES-001
Arena contract
    ↓
ADR-002
Device/backend model
    ↓
ADR-003
Memory topology
    ↓
ADR-004
Allocator/storage
    ↓
ADR-005
Tensor/view model
    ↓
ADR-006
Layout semantics
    ↓
ADR-007
Dtype semantics
    ↓
ADR-008
Backend capabilities
    ↓
ADR-009
Planner/performance model
```

There is now enough architecture to derive the C implementation instead of inventing architecture while coding.
