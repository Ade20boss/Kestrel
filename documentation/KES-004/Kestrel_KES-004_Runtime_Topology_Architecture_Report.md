# Kestrel KES-004 Runtime Topology Architecture Report

**Project:** Kestrel  
**Ticket:** KES-004 - Runtime Topology  
**Status:** Architecture contract complete enough to move into the implementation ticket  
**Documentation checkpoint:** 6 October 2026  

## 1. Purpose of this document

This document consolidates the complete KES-004 topology design discussion into one source of truth. It records what the topology layer means, why the model changed, the edge cases used to stress-test it, what has been frozen, what has been superseded, what remains deliberately deferred, and what must happen next before implementation begins.

The central rule of KES-004 is:

> Kestrel topology describes what compute and memory resources exist and how those resources are structurally related. It does not decide what is legal, what is fast, or what should run where.

That gives the runtime four separate concerns:

```text
Topology     = what exists and how it is connected
Capabilities = what operations are legal on a target/backend
Performance  = what those operations cost
Planner      = what Kestrel should choose
Execution    = actually execute the work
```

KES-004 owns the first line only.

## 2. Why the original topology model had to change

The original KES-004 Slice 1 assumed a flat device model:

```text
Device
Backend Instance
Memory Space
Relationship
```

The important original shape was effectively:

```text
Backend Instance -> Device
Device <-> Memory Space
```

and the old `kestrel_device_t` was intended to represent a physical CPU, GPU, accelerator, or unknown device.

Research into CUDA, NVIDIA MIG/NVML, Intel Level Zero, OpenCL-style partitioning, Linux SR-IOV, Vulkan device groups, hwloc/CPU topology, and AMD GPU partitioning showed that one object called `Device` was being forced to mean several different things at once.

The failures were concrete:

- One physical GPU can expose multiple independently schedulable execution targets, as with MIG.
- One physical GPU can expose a hierarchy of execution targets, as with Level Zero root devices and subdevices.
- A schedulable execution target can exist without its physical parent being visible, as in a virtual machine receiving an SR-IOV virtual function.
- One execution target can be backed by more than one physical device, as with a multi-device execution construct such as a Vulkan device group.
- Multiple backend APIs can expose the same execution target. A normal NVIDIA GPU can be visible through both CUDA and Vulkan.
- Multiple execution targets can share one memory space, as with compute instances sharing a MIG GPU Instance memory partition.

Therefore the flat `Device` abstraction was superseded.

## 3. Current topology vocabulary

The amended model separates four core concepts:

```text
Physical Device
        ^ BACKED_BY
Execution Target
        |
        +----> Backend Instance

Execution Target ---- SUBTARGET_OF ----> Execution Target

Execution Target <---- topology association ----> Memory Space
```

### 3.1 Physical Device

**Definition:** A root physical compute resource that Kestrel can identify independently of the execution targets derived from it.

A Physical Device is defined by execution-relevant hardware identity, not by packaging boundaries such as a board, removable card, socket connector, die, chiplet, or core.

Examples:

- one CPU package;
- one physical NVIDIA GPU;
- one physical AMD GPU;
- another root accelerator resource if the platform exposes it as an independently identifiable whole.

Non-examples:

- one CPU core;
- one MIG partition;
- one Level Zero subdevice;
- one SR-IOV virtual function;
- one AMD execution partition;
- one GPU die merely because it is physically separate silicon inside a larger physical GPU resource.

The practical question used to find this granularity is:

> What whole underlying hardware compute resource are the execution targets derived from?

A useful supporting question is:

> What whole resource is being partitioned?

This avoids turning Kestrel into a complete hardware inventory system.

### 3.2 Execution Target

**Definition:** A compute target Kestrel may independently choose as somewhere to execute work.

This is the scheduler-facing object.

An Execution Target may represent:

- the host CPU execution target;
- a whole GPU;
- a MIG target;
- a Level Zero subdevice;
- an SR-IOV virtualized target;
- a future aggregate/multi-device target.

The critical distinction is:

```text
Physical Device = underlying hardware root
Execution Target = schedulable compute target
```

They are intentionally not the same concept even when they happen to be one-to-one on a simple machine.

### 3.3 Backend Instance

**Definition:** A concrete Kestrel backend instance capable of executing on exactly one Execution Target.

Current backend families:

```text
SCALAR
AVX2
VULKAN
CUDA
```

Example:

```text
Execution Target E1
├── Scalar backend instance B1
└── AVX2 backend instance B2
```

or:

```text
Execution Target E2
├── CUDA backend instance B3
└── Vulkan backend instance B4
```

The invariant is:

> Every Backend Instance refers to exactly one Execution Target.

The old `backend_instance.device_id` design is superseded by `backend_instance.execution_target_id`.

### 3.4 Memory Space

**Definition:** A memory domain in which bytes relevant to Kestrel storage can reside.

A Memory Space is not an allocation, allocator, provider, pointer, backend object, or physical device.

Current broad memory kinds:

```text
SYSTEM
DEVICE_LOCAL
UNKNOWN
```

Examples:

- system RAM -> `SYSTEM`;
- normal discrete GPU local memory -> `DEVICE_LOCAL`;
- a MIG GPU Instance memory partition -> `DEVICE_LOCAL`.

Two memory spaces may have the same kind while still being completely distinct domains. The kind is classification; the memory-space ID is identity.

A memory space exists in topology only if it is relevant to runtime execution/storage decisions. Kestrel does not create topology objects for every cache, register file, BAR region, or hardware memory structure merely because it physically exists.

## 4. Physical Device granularity decisions

### 4.1 Packaging is not identity

A board can contain multiple GPUs, so one board does not imply one Physical Device.

Conversely, a single physical GPU resource can contain multiple dies/chiplets without each die becoming a Kestrel Physical Device.

Therefore:

> Physical Device granularity is determined by the root compute resource exposed by the system/vendor model, not by packaging or silicon boundaries.

### 4.2 CPU

The current CPU physical root is the CPU package.

A simple host may therefore look like:

```text
Physical CPU P1
        |
        +---- Execution Target E1
              ├── Scalar B1
              └── AVX2 B2
```

A dual-socket host may have two Physical Devices. Kestrel may initially expose one host-wide Execution Target backed by both packages, or later expose package/NUMA-aware targets. The topology model allows either without changing the Physical Device definition.

### 4.3 Integrated CPU/GPU systems

A shared SoC/package does not require CPU and GPU compute engines to collapse into one Physical Device merely because they share packaging.

Kestrel may model independently identifiable CPU and GPU compute resources separately while representing shared memory through Memory Space relationships.

### 4.4 GPU partitioning

A MIG partition, Level Zero subdevice, or similar execution partition is an Execution Target, not another Physical Device.

This preserves the distinction between root hardware and derived scheduling surfaces.

## 5. Topology relationships

Three different structural relationships are required. They must remain typed and separate.

### 5.1 BACKED_BY: Execution Target -> Physical Device

Meaning:

> Which known physical compute resource or resources underlie this execution target?

The relationship is represented separately rather than embedding a single `physical_device_id` inside an Execution Target.

This is required because the real cardinality is not one-to-one:

```text
Normal GPU:
E1 -> P1

MIG:
E1 -> P1
E2 -> P1
E3 -> P1

Virtualized guest with hidden physical parent:
E1 -> no known Physical Device

Aggregate target:
E1 -> P1
E1 -> P2
```

Therefore the model permits zero or more known physical backing relationships per Execution Target.

Zero means Kestrel cannot prove the physical parent from the current environment. It does not mean the target has no underlying hardware.

Duplicate `(execution_target, physical_device)` edges are invalid.

### 5.2 SUBTARGET_OF: Execution Target -> Execution Target

Meaning:

> This target was exposed as a direct execution subdivision of another execution target.

This exists specifically to preserve systems such as Intel Level Zero where execution targets may form nested hierarchies:

```text
E1
├── E2
└── E3
    ├── E4
    └── E5
```

Stored direct relations:

```text
E2 SUBTARGET_OF E1
E3 SUBTARGET_OF E1
E4 SUBTARGET_OF E3
E5 SUBTARGET_OF E3
```

Current hierarchy invariants:

- a target cannot parent itself;
- one target has at most one direct execution parent;
- the hierarchy must be acyclic;
- only direct parent edges are stored; transitive ancestry is derived.

A shared physical parent does not imply a SUBTARGET_OF relationship.

For example, two MIG targets both backed by one physical GPU are not automatically parent/child execution targets.

### 5.3 Execution Target <-> Memory Space association

Meaning:

> This execution target is topologically associated with this memory domain.

This relationship deliberately does not claim that every operation is legal, that access is local, or that access is fast.

Those are capability/performance questions.

Examples:

```text
Host CPU:
E_CPU ---- M_SYSTEM
```

```text
Discrete GPU:
E_GPU ---- M_GPU
```

Two MIG compute instances sharing a GPU Instance memory partition:

```text
E1 ---\
       +---- M1
E2 ---/
```

Integrated CPU/GPU sharing system memory:

```text
E_CPU ---\
          +---- M_SYSTEM
E_GPU ---/
```

The previous `LOCAL / SHARED / REMOTE` relationship enum is not part of the current core topology contract. Those labels mixed topology with access/performance semantics and are therefore superseded for KES-004.

## 6. Why allocators and storage are not part of KES-004

The distinction is:

```text
Execution Target = where computation can run
Memory Space     = where bytes can live
Allocator/Provider = how Kestrel asks for bytes there
Allocation/Storage = one specific chunk of bytes
```

Every allocatable memory space needs some allocation path eventually, but that does not imply exactly one allocator per memory space.

A memory domain can potentially be reachable through multiple allocation/provider mechanisms. Therefore KES-004 does not embed an allocator/provider into Physical Device, Execution Target, or Memory Space.

That work is deliberately deferred to KES-005.

The existing Kestrel storage abstraction remains compatible with this direction: storage owns byte size, memory-space identity, provider/allocator reference, and an opaque provider-specific token; tensors refer to storage without understanding native backend handles.

## 7. Stress tests that shaped the model

### 7.1 Ordinary CPU with Scalar and AVX2

```text
Physical Device P1 = CPU package
Execution Target E1 = host CPU compute target

B1 Scalar -> E1
B2 AVX2   -> E1

E1 ---- M1 system RAM
```

Result: model passes without special cases.

### 7.2 Normal NVIDIA GPU exposed by CUDA and Vulkan

```text
Physical GPU P1
      ^
      | BACKED_BY
Execution Target E1
├── CUDA B1
└── Vulkan B2

E1 ---- M1 GPU-local memory
```

CUDA and Vulkan are not separate devices here. They are two backend instances targeting one reconciled execution target.

### 7.3 NVIDIA MIG

One physical GPU can expose multiple CUDA execution targets:

```text
Physical GPU P1
^      ^      ^
|      |      |
E1     E2     E3
```

MIG also proves target-to-memory is not one-to-one. Two compute instances may share one GPU Instance memory partition:

```text
E1 ---\
       +---- M1
E2 ---/

E3 -------- M2
```

This case was one of the strongest reasons to keep Execution Target, Physical Device, and Memory Space separate.

### 7.4 Intel Level Zero root and nested subdevices

Level Zero can expose root devices and subdevices, including nested subdivision.

The model represents this through SUBTARGET_OF rather than by flattening everything into one list with no ancestry.

```text
E1
├── E2
└── E3
    └── E4
```

This is the primary reason an explicit execution hierarchy exists.

### 7.5 SR-IOV host and guest

On the host, virtual functions can be related back to their physical function/root hardware.

Inside a guest, the physical parent may not be visible at all.

Therefore this invariant is invalid:

```text
Every Execution Target must have a Physical Device.
```

The correct model is:

> An Execution Target may have zero or more known physical backing relationships.

### 7.6 Multi-device / Vulkan device-group execution target

The architecture permits a future execution construct backed by more than one physical device:

```text
P1 ---\
       +---- E_GROUP
P2 ---/
```

This does not mean P1 and P2 became one physical GPU. It means Kestrel has one schedulable execution construct that can involve both physical devices.

This keeps the Backend Instance invariant simple: a backend instance still targets one Execution Target, even when that Execution Target is aggregate.

This capability is architecturally accommodated, not necessarily implemented in the first Kestrel release.

## 8. Discovery is separate from finalized topology

The finalized topology is not the raw output of CUDA, Vulkan, NVML, Linux sysfs, or any other discovery source.

Discovery produces temporary observations and evidence. Reconciliation turns those observations into canonical Kestrel entities.

```text
Discovery sources
      |
      +-- Physical observations
      +-- Execution observations
      +-- Memory observations
      +-- typed identity claims
      +-- physical locators
      +-- relationship evidence
              |
              v
        reconciliation
              |
              v
       canonical topology
```

### 8.1 Observation IDs are temporary

Discovery uses strongly typed temporary IDs such as:

```text
physical_observation_id
execution_observation_id
memory_observation_id
```

These IDs only exist during builder construction.

Example:

```text
POBS1 = physical GPU observation
EOBS1 = MIG execution observation
```

If discovery reports:

```text
EOBS1 BACKED_BY POBS1
```

and reconciliation later resolves:

```text
EOBS1 -> E1
POBS1 -> P1
```

then the finalized relation becomes:

```text
E1 BACKED_BY P1
```

The same translation applies to SUBTARGET_OF and execution-memory relationships.

## 9. Reconciliation semantics

### 9.1 What reconciliation means

Reconciliation answers only:

> Do these independently discovered observations describe the same canonical entity?

It does not create parent relationships.

That gives three separate operations:

```text
Reconciliation:
Are these observations the same entity?

Physical backing association:
What physical device backs this execution target?

Execution hierarchy association:
Is this execution target a subtarget of another execution target?
```

### 9.2 Identity evidence is typed

Kestrel must never treat every UUID-shaped byte sequence as globally comparable.

Identity evidence has a kind/namespace. Current identity vocabulary includes concepts such as:

```text
PCI_BDF
LINUX_DRM_DEVICE
NVIDIA_GPU_UUID
CUDA_DEVICE_UUID
VULKAN_DEVICE_UUID
NVIDIA_MIG_UUID
LEVEL_ZERO_DEVICE_UUID
```

The exact identity payload encoding is discussed later in this document.

Names, vendor strings, model strings, backend enumeration positions, CUDA ordinals, Vulkan enumeration order, and NVML indexes are not identity.

### 9.3 Legal comparisons are explicit

Two identity claims may only participate in reconciliation if Kestrel has an explicit rule stating that those identity namespaces are comparable in that context.

Example:

```text
CUDA_DEVICE_UUID <-> VULKAN_DEVICE_UUID
```

may be comparable on a platform/vendor combination where the API/vendor contract explicitly defines that correlation.

By contrast:

```text
LEVEL_ZERO_DEVICE_UUID <-> VULKAN_DEVICE_UUID
```

must be treated as indeterminate unless Kestrel has a documented comparison rule.

Even identical raw bytes do not permit an undocumented merge.

### 9.4 Reconciliation outcomes

The resolver has four outcomes:

```text
MATCH
DISTINCT
INDETERMINATE
CONFLICT
```

**MATCH** - sufficient comparable evidence proves the observations describe the same entity.

**DISTINCT** - comparable identity evidence proves they are different entities.

**INDETERMINATE** - Kestrel cannot prove either result. The observations remain separate; Kestrel never interprets this as "probably the same".

**CONFLICT** - authoritative evidence disagrees. The builder must not silently choose one interpretation. Finalization fails or the conflict must otherwise be explicitly resolved before a valid topology can be produced.

## 10. Identity claims, physical locators, and relationship evidence are different

MIG exposed an important distinction.

A PCI BDF can be useful for identifying/correlating a physical PCI function, but it must not be blindly used as an Execution Target identity. Multiple MIG targets can be derived from one underlying PCI function.

Therefore discovery evidence has three semantic categories:

### 10.1 Identity claim

Meaning:

> This evidence identifies this observation as an entity within a particular identity namespace.

Used for reconciliation.

Examples: CUDA device UUID, Vulkan device UUID, NVIDIA physical GPU UUID, MIG UUID.

### 10.2 Physical locator

Meaning:

> This execution observation appears to be associated with physical hardware at this host-local location.

Used to help establish BACKED_BY relationships, not to merge execution targets.

PCI BDF and Linux-local DRM information can serve this role where appropriate.

Example:

```text
MIG EOBS1
    execution identity = UUID A
    physical locator   = PCI 0000:01:00.0

MIG EOBS2
    execution identity = UUID B
    physical locator   = PCI 0000:01:00.0
```

The UUIDs show two distinct execution targets. The shared physical locator may help associate both targets with the same Physical Device.

### 10.3 Explicit relationship evidence

Meaning:

> The discovery API explicitly reported a structural relationship between two observations.

Examples:

```text
NVML MIG observation BACKED_BY physical GPU observation
Level Zero child observation SUBTARGET_OF parent observation
```

Explicit relationship evidence is preferred over guessing relationships from weak similarity.

## 11. Memory observation and reconciliation seam

Memory spaces may potentially be observed by more than one source.

Therefore the discovery model allows memory observations to carry typed identity claims even though Kestrel v1 does not need to invent a large memory-identity vocabulary immediately.

Conceptually:

```text
MemoryIdentityClaim
    memory_observation_id
    identity_kind
    identity_value
```

This prevents the architecture from assuming that every API-visible memory observation is automatically a distinct final Memory Space.

The seam exists now; exact vendor-specific memory identity kinds can be added only when real discovery requirements justify them.

## 12. What survives finalization

The builder may temporarily own:

```text
raw discovery observations
source-local handles
identity claims
physical locators
relationship evidence
temporary reconciliation state
canonical objects under construction
normalized metadata under construction
```

After successful finalization:

```text
DIES:
raw API handles
temporary observation IDs
temporary reconciliation state
duplicate/raw evidence that is no longer needed

SURVIVES:
canonical Physical Devices
canonical Execution Targets
Backend Instances
Memory Spaces
normalized relationships
selected normalized identity metadata
```

The key interpretation of ownership transfer is:

> Everything that forms finalized runtime state becomes owned by RuntimeTopology. Temporary construction state does not have to be transferred; it is destroyed with the builder.

## 13. Retained identity metadata

The core topology objects stay deliberately small. Backend/vendor identity data is not embedded directly into every object.

The finalized topology may retain normalized identity metadata in separate tables keyed by final Kestrel IDs:

```text
PhysicalIdentityMetadata[]
ExecutionIdentityMetadata[]
MemoryIdentityMetadata[]
```

Example:

```text
P1 -> NVIDIA_GPU_UUID(...)
P1 -> PCI_BDF(0000:01:00.0)

E1 -> CUDA_DEVICE_UUID(...)
E1 -> VULKAN_DEVICE_UUID(...)
```

This gives Kestrel useful post-finalization diagnostic/re-identification information without turning the core objects into CUDA/Vulkan/NVML-specific structures.

Only normalized identities worth retaining survive. Raw source handles and duplicate discovery evidence do not.

## 14. Candidate identity-value representation

The current implementation candidate is a fixed-size generic identity payload:

```text
IdentityValue
    kind
    length
    bytes[64]
```

The bytes are interpreted according to `kind`.

Rationale:

- supports UUIDs, PCI identity encodings, Linux-local identities, and future vendor-specific values;
- avoids a heap allocation for every small identity claim;
- avoids changing a giant central union every time a new identity namespace is added;
- keeps identity semantics in the `kind` and comparator rules rather than in raw byte shape.

**Status:** Current implementation proposal, not yet separately user-frozen as an immutable ABI decision. It should be confirmed in the implementation ticket before code is committed.

## 15. Finalized topology data model

### 15.1 Strongly typed final IDs

```text
kestrel_physical_device_id_t
kestrel_execution_target_id_t
kestrel_backend_instance_id_t
kestrel_memory_space_id_t
```

Rules:

- IDs wrap `uint32_t` conceptually;
- zero is invalid;
- valid runtime IDs begin at one;
- different ID types are not interchangeable;
- IDs are only meaningful within one topology snapshot.

### 15.2 Shared compute class

```text
INVALID
CPU
GPU
ACCELERATOR
UNKNOWN
```

One common compute-class enum is preferred over duplicate physical-device and execution-target class enums.

### 15.3 Backend family

```text
INVALID
SCALAR
AVX2
VULKAN
CUDA
```

Compile-time backend support and runtime backend availability remain separate concepts.

### 15.4 Memory kind

```text
INVALID
SYSTEM
DEVICE_LOCAL
UNKNOWN
```

A MIG memory partition is `DEVICE_LOCAL`; MIG describes how the resource is partitioned, not a fundamentally new memory kind.

### 15.5 Physical Device

```text
PhysicalDevice
    id
    compute_class
    name
    vendor
```

No embedded backend IDs, execution-target IDs, memory-space IDs, allocator/provider, UUID, PCI BDF, or parent pointer.

### 15.6 Execution Target

```text
ExecutionTarget
    id
    compute_class
    name
```

No embedded physical-device ID, execution-parent ID, backend list, or memory-space ID. Those are graph relationships.

### 15.7 Backend Instance

```text
BackendInstance
    id
    family
    execution_target_id
```

### 15.8 Memory Space

```text
MemorySpace
    id
    kind
    name
    capacity_known
    capacity_bytes
```

Invariant:

```text
capacity_known == false => capacity_bytes == 0
```

### 15.9 Physical-execution relation

```text
PhysicalExecutionRelation
    execution_target_id
    physical_device_id
```

Represents BACKED_BY.

### 15.10 Execution hierarchy relation

```text
ExecutionHierarchyRelation
    child_target_id
    parent_target_id
```

Represents SUBTARGET_OF.

### 15.11 Execution-memory relation

```text
ExecutionMemoryRelation
    execution_target_id
    memory_space_id
```

Represents structural memory-domain association only.

## 16. RuntimeTopology ownership and opacity

The finalized topology should be an immutable snapshot.

The preferred public API direction is an opaque `kestrel_runtime_topology_t` rather than exposing mutable internal arrays in the public struct.

Reason:

> Immutability should be enforced by the API shape, not merely requested by convention.

Internally the topology may own arrays for:

```text
PhysicalDevice[]
ExecutionTarget[]
BackendInstance[]
MemorySpace[]
PhysicalExecutionRelation[]
ExecutionHierarchyRelation[]
ExecutionMemoryRelation[]
PhysicalIdentityMetadata[]
ExecutionIdentityMetadata[]
MemoryIdentityMetadata[]
```

Callers use read-only query functions rather than writable array ownership.

## 17. Builder model

Discovery and reconciliation feed a mutable topology builder.

```text
Discovery sources
        |
        v
kestrel_topology_builder_t   (mutable)
        |
        | reconcile / associate / validate / normalize
        v
kestrel_runtime_topology_t   (immutable snapshot)
```

The builder owns dynamic typed arrays because the final number of unique entities is not known before cross-source reconciliation.

Typed arrays are preferred over introducing a generic vector library at this stage.

### 17.1 Builder responsibilities

Conceptually the builder accepts:

```text
physical observations
execution observations
memory observations

physical identity claims
execution identity claims
memory identity claims
execution physical locators

BACKED_BY evidence
SUBTARGET_OF evidence
execution-memory evidence

backend-instance observations
```

Each observation registration returns a strongly typed temporary observation ID.

### 17.2 Canonicalization maps

During finalization the resolver effectively creates mappings:

```text
POBS -> PhysicalDevice ID
EOBS -> ExecutionTarget ID
MOBS -> MemorySpace ID
```

Multiple observations may map to one canonical final entity.

Example:

```text
CUDA EOBS1  ---\
                +---- E1
Vulkan EOBS2 ---/
```

Relationship evidence is then rewritten using canonical IDs.

## 18. Finalization contract

Builder finalization must be transactional.

Success:

```text
builder -> exact ZERO state
topology -> LIVE immutable snapshot
```

Failure:

```text
builder -> remains LIVE, valid, and destroyable
topology -> remains exact ZERO state
```

No half-transfer is permitted.

The intended pattern is:

1. perform all operations that can fail;
2. reconcile observations;
3. resolve physical associations;
4. canonicalize relationships;
5. normalize/deduplicate metadata;
6. validate the graph and all references;
7. only then transfer finalized ownership into RuntimeTopology;
8. reset the builder to ZERO.

Validation includes at least:

- all referenced IDs/observations exist;
- reconciliation conflicts are rejected;
- SUBTARGET_OF has no self-parent;
- one child has at most one direct parent;
- execution hierarchy is acyclic;
- duplicate final relationships are removed/rejected according to contract;
- every Backend Instance points to an existing Execution Target;
- memory capacity invariant holds;
- finalized retained identities are internally consistent.

Discovery order must not determine semantic truth. There is no `first backend wins` rule.

## 19. Lifecycle

### 19.1 RuntimeTopology

```text
ZERO
  |
  | successful finalize
  v
LIVE immutable snapshot
  |
  | destroy
  v
ZERO
```

Destroy should be safe on ZERO and restore exact ZERO state.

### 19.2 TopologyBuilder

```text
ZERO
  |
  | init
  v
LIVE mutable builder
  |
  | successful finalize
  v
ZERO
```

On failed finalization, the builder remains valid and destroyable.

## 20. Superseded assumptions and controlled amendment record

The following earlier KES-004 assumptions are now superseded.

### Superseded: one flat Device concept

Old:

```text
Device = physical CPU/GPU/accelerator and scheduler-facing device
```

New:

```text
Physical Device = root physical compute resource
Execution Target = schedulable compute target
```

### Superseded: Backend Instance -> Device

Old:

```text
backend_instance.device_id
```

New:

```text
backend_instance.execution_target_id
```

### Superseded: Execution Target embeds one physical parent

Rejected design:

```text
ExecutionTarget {
    physical_device_id;
}
```

Reason: hidden physical parents, one physical device with many targets, and aggregate targets backed by multiple physical devices.

Replacement: separate BACKED_BY relations.

### Superseded: Device <-> Memory Space

Old model attached memory to a generic device.

New model associates Memory Space with Execution Target, because scheduling/storage decisions care about the target that can use the memory domain.

### Superseded: generic LOCAL / SHARED / REMOTE memory relationship enum

Reason: these labels blur structural topology, legal accessibility, and performance cost.

Replacement: a structural execution-memory association in KES-004; detailed access legality goes to capabilities and cost goes to performance profiles.

### Superseded: raw name/vendor/capacity matching

Names, models, vendors, and capacities are diagnostic properties, never sufficient identity for reconciliation.

### Superseded: backend API index as identity

CUDA ordinals, Vulkan enumeration order, NVML indexes, and similar source enumeration positions are not stable canonical identity.

### Superseded: blindly compare UUID-shaped bytes

Identity namespaces are typed. Raw byte equality does not establish legal comparability.

## 21. Frozen/current invariants summary

The current architecture relies on these invariants:

- zero is invalid for every strongly typed runtime ID and observation ID;
- valid runtime IDs begin at one;
- `UNKNOWN` is a valid known state distinct from `INVALID`;
- Physical Device and Execution Target are separate concepts;
- Backend Instance points to exactly one Execution Target;
- an Execution Target may have zero, one, or multiple known physical backing relations;
- physical backing and execution hierarchy are different relationships;
- SUBTARGET_OF stores only direct ancestry;
- SUBTARGET_OF is acyclic;
- a child has at most one direct execution parent;
- one physical device may back many execution targets;
- one execution target may be backed by multiple physical devices;
- multiple Backend Instances may target the same Execution Target;
- one Backend Family may have multiple instances across different targets;
- multiple Execution Targets may associate with one Memory Space;
- multiple Memory Spaces may have the same memory kind;
- `capacity_known == false` implies `capacity_bytes == 0`;
- topology does not allocate memory;
- topology does not decide capability legality;
- topology does not contain performance costs;
- topology does not choose placements;
- discovery observations are not canonical entities;
- identity claims are typed;
- only explicitly compatible identity namespaces may be compared;
- `INDETERMINATE` never causes a merge;
- `CONFLICT` is not silently resolved;
- raw source handles and temporary observation IDs die after finalization;
- selected normalized identity metadata may survive in separate finalized tables;
- RuntimeTopology is immutable after successful finalization;
- finalization is transactional.

## 22. Deliberately deferred work

The following are not KES-004 responsibilities and should not reopen the topology architecture unless new evidence genuinely requires it.

### KES-005: allocator/provider/storage mechanics

Still to decide:

- exact memory-provider abstraction;
- whether provider binding is backend-instance plus memory-space, or another representation;
- CUDA/Vulkan allocation paths;
- host/pinned/device-local allocation providers;
- storage creation and provider-specific opaque tokens.

### Capability layer

Still to decide:

- which operations/dtypes/layouts are legal on each backend target;
- detailed cross-memory accessibility;
- backend feature flags.

### Performance layer

Still to decide:

- bandwidth/latency/copy cost;
- kernel performance;
- topology-aware placement costs;
- benchmark-derived profiles.

### Planner

Still to decide:

- target selection;
- aggregate-target policy;
- migration/copy decisions;
- heterogeneous partitioning.

### Hotplug / topology generations

The immutable-snapshot model can support publishing a new topology snapshot later, but exact generation/version semantics are not part of the current KES-004 implementation contract.

## 23. Remaining item to confirm before implementation

Most of the semantic architecture is settled. The primary small contract detail still worth explicitly confirming in the implementation ticket is:

```text
IdentityValue
    kind
    length
    bytes[64]
```

The existence of typed identity claims and separate retained metadata is settled; the exact fixed payload size/ABI representation should be confirmed before it becomes code.

Memory identity kinds are intentionally not exhaustively defined yet. The seam exists, and concrete kinds should be added when real discovery implementations require them.

## 24. Current KES-004 status

At this documentation checkpoint:

```text
Hardware/runtime research and stress testing      DONE
Physical Device semantics                        DONE
Execution Target semantics                       DONE
Backend Instance semantics                       DONE
Memory Space semantics                           DONE
BACKED_BY semantics                              DONE
SUBTARGET_OF semantics                           DONE
Execution-memory association semantics           DONE
Identity/reconciliation semantics                DONE
Discovery observation model                      DONE
Retained metadata strategy                       DONE
Builder lifecycle/finalization contract          DONE
Consolidated architecture documentation          THIS DOCUMENT

NEXT:
KES-004 implementation ticket
    -> exact files
    -> exact public/internal types
    -> exact function signatures/responsibilities
    -> failure behavior
    -> implementation order
    -> tests
    -> Definition of Done

THEN:
write C
```

No KES-005 work should begin until KES-004 is implemented and its topology snapshot/builder contract is validated by tests.

## 25. One-page mental model

If the rest of this report is forgotten, retain this:

```text
                    FINAL TOPOLOGY

Physical Device
    root physical compute resource
          ^
          | BACKED_BY
          |
Execution Target -------------------- Memory Space
    schedulable target                where bytes can live
          |
          +---- Backend Instance
          |       concrete API/runtime path
          |
          +---- SUBTARGET_OF ----> Execution Target
                   direct execution hierarchy
```

Discovery is a separate construction world:

```text
CUDA / Vulkan / NVML / Linux / other sources
                    |
                    v
              observations
              identity claims
              physical locators
              relationship evidence
                    |
                    v
              reconciliation
                    |
                    v
          canonical final entities
                    |
                    v
         immutable RuntimeTopology
```

The most important distinctions are:

```text
same entity?                    -> reconciliation
execution target vs hardware?   -> BACKED_BY
child execution target?         -> SUBTARGET_OF
where bytes may reside?         -> Memory Space
how bytes are allocated?        -> KES-005
what is legal?                  -> capability layer
what is fast?                   -> performance layer
what should run where?          -> planner
```

That separation is the foundation KES-004 is intended to preserve.
