
# KES-004 — Runtime Topology

**Status:** ACTIVE  
**Depends on:** KES-001 Arena lifecycle/failure contract, KES-002 heterogeneous-runtime architecture, KES-003 dtype/numeric semantics  
**Blocks:** KES-005 allocator/storage and essentially every backend/planner ticket after it

## Goal

Implement Kestrel’s canonical runtime-topology layer.

KES-004 must allow Kestrel to discover heterogeneous compute resources from multiple sources, reconcile independent observations of the same entities, preserve real hardware/execution relationships, and produce one immutable canonical topology snapshot that later subsystems can use.

The core question is:

> **What compute resources exist, what execution targets can Kestrel schedule onto, what memory spaces exist, and how are those things structurally related?**

Topology answers **what exists and how it is connected**.

It does **not** decide:

```
is this operation legal?      → Capability
how expensive is it?          → Performance
which target should I choose? → Planner
run the work                  → Execution
```

---

# 1. Frozen conceptual model

The runtime topology is built around four primary concepts:

```
PhysicalDevice
      ↑
   BACKED_BY
      |
ExecutionTarget
      ↑
 SUBTARGET_OF
      |
BackendInstance

ExecutionTarget ↔ MemorySpace
```

## PhysicalDevice

Represents a root, non-partition, execution-relevant physical compute resource identifiable as a whole.

Examples:

```
CPU package
NVIDIA GPU
AMD GPU
other root accelerator
```

It does **not** mean:

```
board
package
die
PCI function
```

by definition.

Packaging boundaries do not define the topology.

For example:

```
multi-GPU board
    ├── PhysicalDevice P1
    └── PhysicalDevice P2
```

rather than necessarily one physical device representing the board.

Likewise, an integrated CPU/GPU package may contain separately identifiable CPU and GPU physical compute resources.

---

# 2. ExecutionTarget

Represents something Kestrel may independently choose as a compute destination.

Examples:

```
whole host CPU
whole GPU
MIG target
Level Zero subdevice
nested Level Zero subdevice
SR-IOV-visible execution target
possible aggregate multi-GPU target
```

The important distinction is:

> `PhysicalDevice` describes physical compute roots.  
> `ExecutionTarget` describes schedulable compute choices.

Those are deliberately not the same thing.

---

# 3. BackendInstance

Represents one Kestrel backend implementation attached to one execution target.

Frozen cardinality:

```
BackendInstance → exactly one ExecutionTarget
```

An execution target may have several backend instances.

Example:

```
ExecutionTarget E1
    ↑
    ├── CUDA BackendInstance B1
    └── Vulkan BackendInstance B2
```

That means CUDA and Vulkan can be two ways to execute on the **same canonical execution target**.

Backend instance identity must therefore not be confused with device identity.

---

# 4. MemorySpace

Represents a memory domain where bytes may reside.

It is **not**:

```
an allocation
an allocator
a memory provider
a pointer
a CUDA/Vulkan native handle
```

Current broad kinds:

```
KESTREL_MEMORY_KIND_INVALID = 0
KESTREL_MEMORY_KIND_SYSTEM
KESTREL_MEMORY_KIND_DEVICE_LOCAL
KESTREL_MEMORY_KIND_UNKNOWN
```

The identity of distinct memory spaces comes from their canonical `memory_space_id`, not by inventing dozens of overly specific memory-kind enum values.

For example, two independent MIG memory partitions may both be:

```
kind = DEVICE_LOCAL
```

while still being distinct:

```
M1 != M2
```

---

# 5. Strong IDs

KES-004 uses distinct ID wrapper types:

```
kestrel_physical_device_id_t
kestrel_execution_target_id_t
kestrel_backend_instance_id_t
kestrel_memory_space_id_t
```

Each currently wraps:

```
uint32_t value;
```

Rules:

```
0 = invalid / unassigned
valid canonical IDs begin at 1
IDs are snapshot-local
```

They are separate semantic types even though they currently have the same underlying representation.

---

# 6. Shared compute class

Both PhysicalDevice and ExecutionTarget use:

```
INVALID = 0
CPU
GPU
ACCELERATOR
UNKNOWN
```

This intentionally says only the broad compute class.

It does not encode detailed vendor capability.

---

# 7. Backend families

Current families:

```
INVALID = 0
SCALAR
AVX2
VULKAN
CUDA
```

Future backends can extend this later.

---

# 8. Canonical public records

The finalized topology must expose records conceptually equivalent to:

## PhysicalDevice

```
id
compute_class
name[128]
vendor[64]
```

## ExecutionTarget

```
id
compute_class
name[128]
```

Do **not** embed:

```
physical_device_id
parent_target_id
memory IDs
backend native handles
```

inside it.

Those relationships are separate because their real cardinalities are not necessarily one-to-one.

## BackendInstance

```
id
family
execution_target_id
```

## MemorySpace

```
id
kind
name[128]
capacity_known
capacity_bytes
```

Do not attach allocator/provider objects here yet.

---

# 9. Structural relationships

KES-004 needs three distinct canonical relationship types.

## 9.1 BACKED_BY

```
ExecutionTarget → PhysicalDevice
```

Stored separately.

Why?

Because reality supports:

```
many ExecutionTargets → one PhysicalDevice
```

such as MIG:

```
E1 → P1
E2 → P1
E3 → P1
```

and potentially:

```
one ExecutionTarget → many PhysicalDevices
```

such as an aggregate device-group target:

```
E3 → P1
E3 → P2
```

Therefore a single:

```
ExecutionTarget.physical_device_id
```

would be wrong.

Required invariant:

```
duplicate (execution_target_id, physical_device_id) pairs forbidden
```

An execution target may have zero known physical backings if the parent hardware is hidden or cannot be resolved.

---

# 10. Vulkan / multi-device aggregate case

The model must be capable of representing:

```
PhysicalDevice P1
PhysicalDevice P2

ExecutionTarget E1 → P1
ExecutionTarget E2 → P2

ExecutionTarget E3 → P1
ExecutionTarget E3 → P2
```

where:

```
E1 = individually usable GPU 1
E2 = individually usable GPU 2
E3 = aggregate/device-group execution target
```

E3 is a distinct execution choice, not merely an alias for E1 + E2.

KES-004 only needs to represent this topology correctly.

Actual multi-GPU execution semantics belong later.

---

# 11. SUBTARGET_OF

Separate relationship:

```
ExecutionTarget → ExecutionTarget
```

Needed for partition/hierarchy systems such as Level Zero.

Example:

```
E1 root
└── E2 subtarget
    └── E3 nested subtarget
```

Frozen hierarchy invariants:

```
no target may parent itself
at most one direct parent
hierarchy must be acyclic
store direct-parent relation only
```

Important:

> Sharing a PhysicalDevice does **not** imply SUBTARGET_OF.

For example:

```
MIG E1 → P1
MIG E2 → P1
```

does not automatically mean:

```
E2 SUBTARGET_OF E1
```

---

# 12. ExecutionTarget ↔ MemorySpace

There must be a separate execution-memory association relation.

It means approximately:

> This memory space is topologically associated with this execution target.

It must **not** mean:

```
allocation is guaranteed legal
access is guaranteed legal
access is local
access is fast
zero-copy exists
```

Those belong to capability/performance layers.

The earlier generic:

```
LOCAL
SHARED
REMOTE
```

relationship concept is **superseded** because it mixed topology, legality, and performance.

---

# 13. MIG memory case

The model must be capable of representing cases such as:

```
PhysicalDevice P1

ExecutionTarget E1
ExecutionTarget E2
ExecutionTarget E3

MemorySpace M1
MemorySpace M2

E1 ↔ M1
E2 ↔ M1
E3 ↔ M2
```

because separate compute instances can potentially share one GPU-instance memory partition.

Therefore:

```
ExecutionTarget count != MemorySpace count
```

is completely valid.

---

# 14. Discovery is not canonical topology

This distinction is fundamental.

Discovery produces **temporary observations**.

It does **not** immediately create final canonical objects.

Conceptually:

```
Discovery
    ↓
temporary observations + evidence
    ↓
reconciliation
    ↓
canonical objects
    ↓
validation
    ↓
immutable RuntimeTopology
```

Observation categories:

```
physical observations    (POBS)
execution observations   (EOBS)
memory observations      (MOBS)
backend observations
```

After reconciliation:

```
POBS → PhysicalDevice ID
EOBS → ExecutionTarget ID
MOBS → MemorySpace ID
```

Temporary observation IDs and raw API handles die after topology construction.

---

# 15. Discovery-source classes

Conceptually, discovery can come from:

```
SYSTEM / HARDWARE
    Linux PCI/sysfs
    CPU/system discovery

MANAGEMENT
    NVML

EXECUTION
    Scalar
    AVX2
    CUDA
    Vulkan
```

Only execution discovery creates `BackendInstance`s.

This distinction is important because NVML can tell us things about physical relationships without itself being a backend used to execute a Kestrel kernel.

Exact public discovery-function names are **not yet frozen**.

---

# 16. Identity evidence

KES-004 needs typed identity values.

Current frozen representation:

```
typedef struct
{
    kestrel_identity_kind_t identity_kind;
    uint8_t length;
    uint8_t bytes[KESTREL_MAX_IDENTITY_BYTES];
} kestrel_identity_value_t;
```

with:

```
KESTREL_MAX_IDENTITY_BYTES = 64
```

Current identity kinds:

```
INVALID = 0

PCI_BDF
LINUX_DRM_DEVICE

NVIDIA_GPU_UUID
CUDA_DEVICE_UUID
VULKAN_DEVICE_UUID
NVIDIA_MIG_UUID
LEVEL_ZERO_DEVICE_UUID
```

---

# 17. Evidence has three semantic categories

This must remain explicit in the builder/discovery model.

## Identity claim

Answers:

> Which exact entity is this observation?

Used for reconciliation.

Examples:

```
CUDA device UUID
Vulkan device UUID
NVIDIA physical GPU UUID
MIG UUID
```

---

## Physical locator

Answers:

> Which physical hardware does this execution observation appear associated with?

Used to establish `BACKED_BY`.

It must **not** blindly participate in execution-target merging.

Typical examples:

```
PCI BDF
Linux-local DRM location
```

MIG demonstrates why:

```
EOBS1
    MIG UUID = A
    PCI BDF  = X

EOBS2
    MIG UUID = B
    PCI BDF  = X
```

These are:

```
two distinct execution targets
```

even though both have:

```
physical locator = X
```

---

## Explicit relationship evidence

Answers:

> What relationship did the discovery API directly tell us exists?

Examples:

```
NVML:
MIG observation BACKED_BY physical GPU observation

Level Zero:
child observation SUBTARGET_OF parent observation
```

Explicit relationship evidence should be preferred over weak inference from coincidental similarity.

---

# 18. PCI BDF rule

This one is important enough to be explicit.

PCI BDF may be useful for:

```
physical-device reconciliation
physical-device correlation
physical locator evidence
```

But:

> PCI BDF must not generally be treated as ExecutionTarget identity.

Otherwise multiple MIG targets sharing the same physical PCI function could be incorrectly merged.

---

# 19. Reconciliation

There are conceptually separate reconciliation domains:

```
physical observations
    ↓
PhysicalDevice

execution observations
    ↓
ExecutionTarget

memory observations
    ↓
MemorySpace
```

The most important cross-backend reconciliation happens at the `ExecutionTarget` level.

Example:

```
CUDA observes target X
Vulkan observes target X
```

If documented compatible identity evidence proves they represent the same target:

```
CUDA EOBS + Vulkan EOBS
        ↓
ExecutionTarget E1
```

Then:

```
CUDA BackendInstance → E1
Vulkan BackendInstance → E1
```

---

# 20. Identity comparison rules

Do **not** implement:

```
raw bytes equal
    ⇒ MATCH
```

Identity namespaces matter.

Comparison is permitted only when Kestrel has an explicit/documented compatibility rule.

Example:

```
CUDA_DEVICE_UUID ↔ VULKAN_DEVICE_UUID
```

may be comparable for ordinary NVIDIA devices where the APIs document that relationship.

But:

```
CUDA UUID ↔ arbitrary unrelated UUID namespace
```

must not automatically match just because 16 bytes happen to be equal.

---

# 21. Reconciliation outcomes

The resolver conceptually needs four outcomes:

```
MATCH
DISTINCT
INDETERMINATE
CONFLICT
```

Meaning:

## MATCH

Evidence establishes the observations represent the same canonical entity.

## DISTINCT

Evidence establishes they are different.

## INDETERMINATE

Kestrel does not have enough legitimate evidence to decide.

Rule:

> INDETERMINATE means keep them separate.

Never:

```
"probably same"
```

## CONFLICT

Evidence is mutually inconsistent.

Finalization must fail.

---

# 22. Physical reconciliation remains

Although CUDA/Vulkan cross-backend matching now primarily occurs at the ExecutionTarget level, PhysicalDevice reconciliation still exists.

For example:

```
Linux/sysfs POBS
    PCI BDF X

NVML POBS
    GPU UUID Y
    PCI BDF X
```

may reconcile into:

```
PhysicalDevice P1
```

Otherwise Kestrel could accidentally create duplicate physical roots.

---

# 23. Canonical retained identity metadata

Chosen strategy:

> Keep final core objects clean, but retain selected normalized identity metadata separately.

So the final topology has metadata conceptually keyed by:

```
PhysicalDevice ID
ExecutionTarget ID
MemorySpace ID
```

rather than embedding UUID/BDF fields inside every core object.

Raw handles such as:

```
VkPhysicalDevice
CUDA ordinal
nvmlDevice_t
temporary observation ID
```

must not survive canonical finalization.

Selected normalized identity metadata may survive.

---

# 24. Memory identity seam

KES-004 must leave room for multiple discovery sources to reconcile the same memory domain.

Therefore memory identity metadata exists as a seam.

However:

> Do not invent fake memory identity kinds just to fill the enum.

Only introduce real identity kinds when backed by actual discovery semantics.

---

# 25. Ownership model

Both builder and final topology are true opaque owner types:

```
typedef struct kestrel_topology_builder kestrel_topology_builder_t;
typedef struct kestrel_runtime_topology kestrel_runtime_topology_t;
```

The public header must not expose:

```
struct fields
impl pointers
ownership internals
```

The previous idea of a public wrapper struct containing a private impl pointer is **superseded**.

---

# 26. Lifecycle

Builder:

```
NULL
 ↓ create
LIVE builder
 ↓ destroy
NULL
```

Runtime topology:

```
NULL
 ↓ successful finalize
LIVE immutable topology
 ↓ destroy
NULL
```

Public lifecycle currently frozen as:

```
kestrel_status_t
kestrel_topology_builder_create(
    kestrel_topology_builder_t **builder
);

void
kestrel_topology_builder_destroy(
    kestrel_topology_builder_t **builder
);

void
kestrel_runtime_topology_destroy(
    kestrel_runtime_topology_t **topology
);
```

---

# 27. Builder-create contract

Preconditions:

```
builder != NULL
*builder == NULL
```

Success:

```
NULL → LIVE
return SUCCESS
```

Failure:

```
caller-visible state unchanged
```

Relevant statuses:

```
SUCCESS
INVALID_ARGUMENT
INVALID_STATE
OUT_OF_MEMORY
```

`kestrel_status_t` is being used where multiple meaningful failures need to be distinguished.

Do **not** retrofit every old Kestrel API merely because topology uses statuses.

---

# 28. Destroy contract

These must all be safe:

```
destroy(NULL)
destroy(&NULL)
destroy(&LIVE)
destroy twice
```

After destruction:

```
caller pointer == NULL
```

Destroy is:

```
void
infallible
```

---

# 29. Builder semantics

The builder is mutable.

It eventually owns:

```
observations
identity claims
physical locators
relationship evidence
backend observations
temporary maps
canonical assembly state
```

Implementation should use typed dynamic arrays rather than one giant untyped bag.

The builder is a construction workspace.

It is **not** the topology users query during runtime.

---

# 30. Final topology semantics

`kestrel_runtime_topology_t` is:

```
canonical
immutable
self-contained
snapshot-based
```

After finalization, callers query it but do not mutate it.

---

# 31. Transactional finalize

Future finalize contract within KES-004:

```
builder LIVE
topology NULL
```

On success:

```
builder → NULL
topology → LIVE immutable snapshot
```

On failure:

```
builder remains LIVE and valid
topology remains NULL
```

This follows the KES-001 transactional-failure philosophy.

Important implementation rule:

> All operations that can fail — allocation, reconciliation, validation, canonicalization — happen before ownership transfer.

Once ownership transfer begins, that final transfer phase should not fail.

---

# 32. Finalization responsibilities

Finalization eventually needs to:

```
validate observations/evidence
reconcile physical observations
reconcile execution observations
reconcile memory observations where possible
canonicalize IDs
rewrite temporary evidence using canonical IDs
deduplicate canonical relationships
validate hierarchy
detect cycles
validate relationship references
validate backend-instance targets
construct immutable arrays
retain selected normalized identity metadata
discard temporary observations/raw handles
```

---

# 33. Query API

KES-004 ends with an immutable topology query surface.

Exact names are **not yet frozen**, but the final API must allow later systems to obtain/read things equivalent to:

```
PhysicalDevices
ExecutionTargets
BackendInstances
MemorySpaces

BACKED_BY relations
SUBTARGET_OF relations
ExecutionTarget↔MemorySpace relations

retained normalized identities
```

The API must not expose mutable internal arrays.

Prefer stable snapshot-local IDs over raw pointers between entities.

---

# 34. KES-004 implementation slices

This is the working implementation sequence.

## Slice 1 — Finalized Topology Types + Lifecycle

**Current slice.**

Files:

```
include/kestrel/topology.h
src/topology.c
tests/test_topology.c
```

Implement:

```
opaque owner types
strong IDs
enums
canonical public record structs
relationship structs
identity value
identity metadata structs
builder create/destroy
runtime topology destroy
representation/lifecycle tests
```

Current private structs may still be:

```
reserved placeholders
```

No real storage yet.

### Slice 1 acceptance

Lifecycle tests:

```
create NULL→LIVE
create(NULL) → INVALID_ARGUMENT
create into LIVE → INVALID_STATE
existing LIVE pointer unchanged
destroy LIVE→NULL
destroy(NULL) safe
destroy(&NULL) safe
destroy twice safe
runtime topology ZERO-state destroy safe
```

Representation tests:

```
INVALID enums == 0
ID zero representable
constants == 128 / 64 / 64

represent:
CPU PhysicalDevice
CPU ExecutionTarget
Scalar + AVX2 backend instances on same target
SYSTEM memory
BACKED_BY
SUBTARGET_OF
Execution↔Memory
multi-physical backing
typed identity value
```

No graph validation yet.

---

# 35. Slice 2 — Builder Typed Dynamic Arrays

Replace placeholder builder internals with typed storage.

Builder needs typed arrays for the categories KES-004 construction requires.

At minimum the design will need room for:

```
physical observations
execution observations
memory observations
backend observations

identity claims
physical locators
explicit relationship evidence
```

Each dynamic array needs correct:

```
count
capacity
element storage
growth
cleanup
OOM behavior
```

Allocation failures must preserve valid builder state.

Do not expose these arrays publicly.

---

# 36. Slice 3 — Observation Registration

Implement builder operations that allow discovery layers to register temporary observations.

Need internal/temporary IDs distinct from final canonical IDs.

Support registering observations for:

```
physical device
execution target
memory space
backend instance
```

Registration must not prematurely canonicalize objects.

Raw source-local handles/ordinals may exist during construction but remain temporary.

---

# 37. Slice 4 — Identity / Locator / Relationship Evidence

Implement builder mechanisms for attaching:

```
identity claims
physical locators
explicit structural relationship evidence
```

Keep these semantics distinct.

Do not create one generic "attribute" mechanism that destroys meaning.

Evidence must know which observation it belongs to and what semantic role it plays.

---

# 38. Slice 5 — Reconciliation

Implement the actual canonical-entity resolver.

Responsibilities:

```
compare compatible identity namespaces
MATCH
DISTINCT
INDETERMINATE
CONFLICT

union/associate MATCH observations
keep INDETERMINATE separate
fail on genuine CONFLICT
```

There should be separate reconciliation reasoning for:

```
PhysicalDevice
ExecutionTarget
MemorySpace
```

as appropriate.

This is where CUDA/Vulkan execution-target reconciliation eventually lives.

---

# 39. Slice 6 — Canonicalization + Validation

Build canonical objects from reconciled observations.

Assign:

```
PhysicalDevice IDs
ExecutionTarget IDs
MemorySpace IDs
BackendInstance IDs
```

starting from 1.

Rewrite temporary relationship evidence into canonical relations.

Validate:

```
IDs/references
duplicate BACKED_BY
duplicate execution-memory relation
self-parent
multiple direct parents
SUBTARGET_OF cycles
backend target existence
other topology invariants
```

This is also where canonical metadata gets assembled.

---

# 40. Slice 7 — Transactional Finalize

Implement builder → immutable runtime topology transition.

Conceptual API shape is still to be frozen, but semantic contract is frozen:

```
success:
builder consumed
topology produced

failure:
builder preserved
topology unchanged/null
```

The final topology owns its canonical arrays.

Builder temporary state is destroyed only on successful transfer.

---

# 41. Slice 8 — Immutable Query API

Expose read-only access to canonical topology.

Users must be able to inspect:

```
physical devices
execution targets
backend instances
memory spaces
relationships
retained normalized identity metadata
```

without gaining mutation access.

Once this works and tests pass, KES-004 is complete.

---

# 42. Important scenarios KES-004 must represent

The architecture should survive all of these without redesign:

### Normal NVIDIA GPU

```
P1
↑
E1
↑
├── CUDA
└── Vulkan
```

### MIG

```
P1
↑   ↑   ↑
E1  E2  E3
```

possibly:

```
E1 ↔ M1
E2 ↔ M1
E3 ↔ M2
```

### Level Zero hierarchy

```
E1
└── E2
    └── E3
```

with:

```
SUBTARGET_OF
```

### SR-IOV / hidden parent

Possible guest-visible:

```
ExecutionTarget E1
BACKED_BY = unknown
```

Kestrel must tolerate no known physical parent.

### Vulkan device group

```
P1 → E1
P2 → E2

P1 ─┐
    ├→ E3
P2 ─┘
```

### Dual-socket host CPU

Potentially:

```
P1 CPU socket
P2 CPU socket

host ExecutionTarget E1
    BACKED_BY P1
    BACKED_BY P2
```

if the initial host backend schedules the machine as one target.

---

# 43. Explicitly out of scope for KES-004

Do **not** let this ticket absorb:

```
allocators
storage allocation
memory providers
tensor descriptors
tensor strides/views
dtype semantics
backend kernel implementations
AVX2 kernels
Vulkan execution
CUDA execution
capability legality
transfer legality
performance profiles
planner decisions
scheduling
autodiff
neural-network ops
training
```

Those belong later.

Especially:

```
MemorySpace != allocator
Topology != capability
Topology != performance
Topology != planner
```

---

# 44. Superseded ideas

Do not return to these unless we discover new evidence requiring an amendment.

### Superseded

```
flat Device model
```

### Superseded

```
BackendInstance == Device
```

### Superseded

```
ExecutionTarget has one physical_device_id
```

### Superseded

```
shared physical backing implies target hierarchy
```

### Superseded

```
PCI BDF is generic ExecutionTarget identity
```

### Superseded

```
LOCAL / SHARED / REMOTE topology relation
```

### Superseded

```
public topology wrapper containing private impl pointer
```

True opaque owner types are now used.

---

# 45. KES-004 Definition of Done

KES-004 is complete when Kestrel can do this:

```
independent discovery sources
        ↓
temporary observations
        ↓
typed identity / locator / relationship evidence
        ↓
reconciliation
        ↓
canonical PhysicalDevices
canonical ExecutionTargets
canonical MemorySpaces
canonical BackendInstances
        ↓
validated BACKED_BY
validated SUBTARGET_OF
validated Execution↔Memory
        ↓
transactional finalize
        ↓
immutable RuntimeTopology
        ↓
read-only queries
```

while satisfying:

```
no accidental execution-target merging
no fake one-to-one physical assumptions
no hierarchy inferred from shared backing
no raw backend handles in final topology
no mutation after finalize
no partial externally visible state after failure
```

---

## Where you are right now

We are only here:

```
KES-004
│
├── Slice 1
│   ├── topology.h                 ✓ APPROVED
│   ├── topology.c lifecycle       ✓ APPROVED
│   └── test_topology.c            ← YOU ARE HERE 😭
│
├── Slice 2 typed arrays
├── Slice 3 observations
├── Slice 4 evidence
├── Slice 5 reconciliation
├── Slice 6 canonicalization/validation
├── Slice 7 finalize
└── Slice 8 queries
```
