# Texyn Labs — Kestrel Architecture Decision Record

## KES-004 — Slice 2: Discovery Observation Storage and Registration

**Project:** Kestrel  
**Area:** Runtime Topology / Discovery  
**Status:** DRAFT — Awaiting review and approval  
**Depends on:** KES-004 Slice 1  
**Scope:** Mutable topology-builder observation and evidence registration

---

# 1. Context

Kestrel is being developed as a neural-network library with a heterogeneous execution runtime.

The runtime must eventually understand the compute and memory resources available on a machine so that it can make correct, hardware-aware execution decisions.

However, different discovery mechanisms describe hardware differently.

Linux may report physical devices and operating-system topology. CUDA may report execution targets visible through its API. Vulkan may expose devices and memory heaps. NVML may report physical NVIDIA GPUs and partition information.

These reports are not automatically equivalent.

For example, CUDA and Vulkan may both expose the same underlying GPU, but they do not necessarily describe it using identical identifiers, metadata, or resource boundaries.

Likewise, one physical NVIDIA GPU may expose several independently schedulable MIG execution targets.

The original flat device model cannot safely represent all these situations.

KES-004 therefore separates physical devices, execution targets, backend instances and memory spaces.

Slice 2 addresses the next problem:

**How should Kestrel receive, retain and validate discovery information before determining which observations represent the same canonical resources?**

---

# 2. The fundamental distinction: Observation versus canonical entity

An observation is a report produced by a discovery mechanism.

A canonical entity is Kestrel's resolved representation of a real resource after the necessary evidence has been examined.

They are not interchangeable.

Example:

CUDA reports an execution target. Vulkan independently reports another execution target.

Slice 2 stores two execution observations.

It does not immediately create one canonical execution target, even if their descriptions look identical.

Later reconciliation may establish that the observations describe the same entity.

Alternatively, it may establish that they describe different entities, discover contradictory evidence, or lack enough information to decide.

The possible reconciliation outcomes remain:

- MATCH
    
- DISTINCT
    
- INDETERMINATE
    
- CONFLICT
    

Slice 2 does not compute those outcomes.

Its responsibility is to preserve the information needed to compute them later.

**Decision:** Registration records source-specific observations without merging or canonicalizing them.

**Rejected:** Constructing finalized topology entities directly from each discovery adapter's reports.

**Why:** Doing so would allow discovery order and incomplete evidence to determine canonical identity.

---

# 3. Discovery sources

A discovery source identifies the mechanism responsible for gathering information.

Examples include Linux sysfs, x86 CPUID, the CUDA Driver API, Vulkan, NVIDIA NVML and Level Zero.

These are discovery mechanisms, not necessarily executable backend instances.

For example, NVML can provide useful information about NVIDIA hardware without being a Kestrel execution backend.

Likewise, registering the CUDA discovery source does not prove that CUDA execution has been initialized successfully.

## Source ID versus source kind

A source kind describes the discovery mechanism.

A source ID identifies a particular registered source record within the current builder.

Example:

- Source S1: Linux sysfs
    
- Source S2: CUDA Driver API
    
- Source S3: Vulkan
    

An observation stores its source ID.

To discover which mechanism produced that observation, Kestrel uses the source ID to retrieve the corresponding source record and inspect its kind.

The ID itself does not encode the source kind.

## Decisions

One topology builder maintains one shared registry of participating discovery sources.

Each distinct source kind may be registered once per builder.

Duplicate source-kind registration is rejected.

A registered source may produce zero observations.

A source record describes participation in collection, not proof of hardware availability.

## Rejected alternatives

**One builder per discovery source:** Rejected because independent builder ownership would introduce a later builder-merging problem, complicating IDs, evidence references and cleanup.

**Broad source labels only:** Rejected where they would obscure useful provenance, such as conflating CPUID with Linux sysfs.

**Automatically returning an existing source ID for duplicate registration:** Rejected because duplicate source registration may indicate an orchestration mistake that should be visible.

---

# 4. Temporary observation IDs

Each successful observation registration receives a temporary, strongly typed ID.

The observation categories have independent ID sequences:

- Physical observations
    
- Execution observations
    
- Memory observations
    
- Backend observations
    

Source records have a separate source-ID sequence.

Valid IDs begin at one.

Zero is invalid.

An ID identifies a registered record within its category and the builder that created it.

It does not identify hardware globally.

For example, physical observation POBS1 and execution observation EOBS1 may both have numeric value one without referring to the same record type.

## Why IDs are builder-wide rather than source-local

All execution observations live in one shared execution-observation array.

The first successfully registered execution observation receives EOBS1, the next EOBS2, and so on, regardless of reporting source.

Each record independently retains its source ID.

This eliminates the need for fixed numerical ranges per discovery source.

It also avoids composite identifiers containing both a source ID and a source-local observation number.

## Decisions

Temporary IDs are strongly typed wrappers around 32-bit unsigned values.

Array position determines the corresponding one-based record ID.

IDs are stable for the builder's lifetime because records are append-only.

A failed registration consumes no ID.

An observation ID is not interchangeable with a finalized topology ID.

## Consequence

A temporary ID is meaningful only within its originating builder.

Passing an ID from another builder is a contract violation. If its numeric value also identifies an existing record in the receiving builder, the simple ID representation cannot always detect the mistake.

This limitation is accepted instead of introducing generation tokens or more elaborate ID machinery.

---

# 5. Four separate observation categories

Kestrel retains four distinct kinds of discovery observations.

## Physical observation

Represents a reported root physical compute resource.

Its descriptive information consists of:

- Compute class
    
- Name
    
- Vendor
    
- Reporting source
    

Examples include CPU packages and physical GPUs.

Physical observations do not embed execution targets, backends, memory spaces or physical locators.

## Execution observation

Represents a reported schedulable compute target.

Its descriptive information consists of:

- Compute class
    
- Name
    
- Reporting source
    

An execution observation may represent an entire GPU, a MIG instance, a Level Zero subdevice, a host CPU target or an aggregate execution construct.

It does not embed a physical parent, execution parent, backend list or memory-space ID.

Those are separate structural relationships.

## Memory observation

Represents a reported memory domain relevant to Kestrel execution and storage.

It contains:

- Memory kind
    
- Name
    
- Whether capacity is known
    
- Capacity in bytes
    
- Reporting source
    

The supported broad kinds are SYSTEM, DEVICE_LOCAL and UNKNOWN.

If capacity is not known, its stored byte count must be zero.

The capacity representation remains a 64-bit unsigned quantity, consistent with the existing finalized memory-space structure.

Two observations with the same memory kind and capacity are not automatically the same memory domain.

## Backend observation

Represents a reported backend-family association with one execution observation.

It contains:

- Backend family
    
- Exactly one execution-observation reference
    
- Reporting source
    

Supported backend families currently include Scalar, AVX2, Vulkan and CUDA.

The referenced execution observation must belong to the reporting source.

A backend observation is not proof that a usable backend instance has been initialized or that a particular operation is supported.

## Decision

Use four separate, builder-owned typed observation arrays.

The builder copies all retained metadata.

No observation is canonicalized during registration.

## Why

The four categories have different semantics, fields and reference rules.

Separate arrays preserve those differences directly in the data model and allow strongly typed registration interfaces.

---

# 6. Identity claims

An identity claim is source-provided evidence that an observation has a particular identifier within a defined namespace.

Examples:

- NVIDIA physical GPU UUID
    
- CUDA device UUID
    
- Vulkan device UUID
    
- NVIDIA MIG UUID
    
- Level Zero device UUID
    

An identity claim has two essential components:

**Kind:** Which identity namespace is being used?

**Value:** The identifier represented according to that namespace's documented encoding.

The kind cannot be ignored.

Two UUID-shaped values are not necessarily comparable just because they contain the same number of bytes.

## Identity ownership

The currently supported identity kinds have defined owner categories.

NVIDIA physical GPU UUIDs belong to physical observations.

CUDA device UUIDs, Vulkan device UUIDs, NVIDIA MIG UUIDs and Level Zero device UUIDs belong to execution observations.

These associations reflect what the identifiers are intended to identify.

Attaching a MIG execution-target identity to an entire physical GPU observation would misrepresent the evidence.

## Encoding

Each identity kind must have an explicitly documented representation.

Binary API UUIDs use their defined byte lengths.

Textual NVIDIA identifiers preserve documented complete identifier formats within Kestrel's fixed payload limit.

Unsupported formats and malformed values are rejected.

Normalization establishes a valid representation.

It does not establish cross-source identity equivalence.

## Multiple and conflicting claims

An observation may have zero, one or several identity claims.

Registration does not silently discard duplicate claims.

Structurally valid but potentially conflicting evidence is retained for later reconciliation.

For example, two reported identifiers may disagree without either claim being malformed.

That is a reconciliation concern, not justification for destroying evidence during registration.

## Decision

Identity claims occupy separate, builder-owned typed evidence storage.

Physical and execution identity owners remain distinct.

Identity validation checks representation and semantic owner compatibility, not cross-source equivalence.

---

# 7. Physical locators

A physical locator describes an observation's association with a particular hardware or operating-system location.

It is not automatically an identity claim.

This distinction is essential for partitioned hardware.

Consider two MIG execution targets derived from one physical GPU.

They may have distinct execution identities while reporting the same physical PCI location.

Their common PCI location may help establish physical backing.

It does not mean they are the same execution target.

## Supported locator categories

The initial locator vocabulary includes:

**PCI BDF:** A PCI domain/segment, bus, device and function.

**Linux DRM device node:** A defined Linux-local DRM primary or render device reference.

PCI locations are represented through validated numerical components rather than arbitrary address strings.

DRM references use a documented node category and device numbers, with system-local scope.

Neither kind is treated as a globally permanent hardware identity.

## Locator ownership

Both physical observations and execution observations may report physical locators.

Example:

A Linux physical GPU observation and a CUDA execution observation may each report PCI location 0000:01:00.0.

The two observations remain independent.

## Why two locator arrays

A single locator array with mixed owner categories would require interpreting a tagged owner as either a physical or execution observation ID.

We instead maintain:

- Physical locator evidence owned by physical observations
    
- Execution locator evidence owned by execution observations
    

Both categories use the same locator-value vocabulary.

The separate arrays make owner typing explicit and reduce the opportunity to interpret a valid ID as belonging to the wrong category.

## Decision

Physical locators are distinct from identity claims and stored in two separate typed, builder-owned evidence arrays.

An observation may have zero or multiple locators.

---

# 8. Three structural relationship categories

Kestrel's discovery model must preserve structural relationships reported by discovery mechanisms.

These are not identity claims.

They answer different questions.

## BACKED_BY

Connects an execution observation to a physical observation.

Question answered:

> Which reported physical resource backs this execution target?

An execution target may have zero, one or multiple known physical backing resources.

Zero means the source cannot establish a physical parent.

It does not mean the execution target has no underlying hardware.

Multiple backing resources support aggregate execution constructs.

## SUBTARGET_OF

Connects a child execution observation to its direct parent execution observation.

Question answered:

> Was this execution target reported as a direct subdivision of another execution target?

This preserves hierarchical targets such as Level Zero root devices and subdevices.

Only direct relationships are reported.

Registration rejects a child whose ID is identical to its parent's ID.

More complicated hierarchy cycles, competing parent claims and relationships that become self-referential after reconciliation remain for later graph validation.

## Execution-memory association

Connects an execution observation to a memory observation.

Question answered:

> Which reported memory domain is structurally associated with this execution target?

The relationship does not prove that every memory operation is legal, that the memory is directly accessible, that an allocation provider exists, or that access is fast.

Those questions belong to capabilities, storage/providers and performance analysis.

## Why three arrays

A single tagged relationship array would require inspecting a discriminator to determine the valid endpoint types.

Three separate arrays allow each relationship category to retain only its corresponding typed references.

The arrays are:

- Backing evidence
    
- Execution hierarchy evidence
    
- Execution-memory evidence
    

Each array is builder-owned.

Registration preserves duplicate reports instead of deduplicating them prematurely.

---

# 9. Same-source relationship registration

A structural relationship may connect only observations reported by the same registered discovery source.

For example, NVML may report both a physical GPU and a related execution target, along with their backing association.

That relationship can be registered within NVML's discovery context.

However, a CUDA execution observation cannot directly reference a physical observation owned by Linux during Slice 2.

Those cross-source associations must be resolved later from appropriately validated evidence.

## Explicit source attribution

Registration operations receive the reporting source ID.

The builder checks that the source exists and that the owning observation or every relationship endpoint belongs to it.

The source ID is not redundantly stored inside a relationship record when it can be derived from its endpoints.

## Why

This prevents accidental construction of cross-source edges during raw discovery.

It also catches an adapter declaring one source while referencing observations belonging to another.

It does not authenticate the caller's real identity.

## Limitation

Same-source registration alone does not guarantee the final canonical graph is valid.

Two distinct execution observations may later reconcile to the same canonical target.

A relationship between them could consequently become a canonical self-parent relationship.

The final graph must therefore be validated after reconciliation.

---

# 10. Memory identity and incomplete evidence

Memory observations and execution-memory associations are fully supported in Slice 2.

However, Kestrel has not established a generally valid memory-identity namespace for correlating memory domains across discovery APIs.

Consider CUDA and Vulkan each reporting an 8 GB device-local memory domain.

Their equal capacities do not prove they represent the same memory resource.

Likewise, an API-local heap index is not automatically a globally meaningful memory identity.

## Decision

Preserve the architectural extension point for typed memory identity claims.

Do not expose functional memory-identity registration in Slice 2 until at least one documented, meaningful identity namespace exists.

Memory observations without identity claims remain valid.

Execution-memory relationships remain fully functional.

## Consequence

Later reconciliation may be unable to prove that two independently reported memory observations refer to the same underlying domain.

It must not merge them based only on kind, name or capacity.

It must also avoid treating unproven distinct observations as guaranteed independent physical memory capacities.

---

# 11. Registration as a transactional operation

A discovery adapter may provide information through multiple registration calls.

For example, it may register a physical observation and subsequently register its UUID, PCI locator and relationships.

Slice 2 does not require all those facts to arrive in one call.

Each registration operation is independently transactional.

## Success

The builder validates the input, prepares any necessary owned storage, commits a complete record, and returns success.

Observation and source registrations assign the next available typed ID.

## Failure

Previously committed records remain intact.

The attempted record is not committed.

No new observation or source ID is consumed.

The builder remains valid and destroyable.

For functions returning an ID, a valid output pointer is set to zero on failure, without reading its previous value.

## Validation boundary

Registration validates local structural correctness.

Examples:

- Required arguments are present.
    
- Source and observation references exist.
    
- Endpoint types are correct.
    
- Same-source ownership is respected.
    
- Enums contain supported values.
    
- Fixed-size strings are terminated within their bounds.
    
- Identity and locator payloads follow their defined formats.
    
- Direct hierarchy self-parenting is rejected.
    

Registration does not perform global graph reconciliation, complex cycle detection or hardware verification.

## Logical versus physical rollback

All malformed inputs should be rejected before unnecessary allocation.

A failed operation must not partially commit its logical record.

However, private array capacity may have increased before an unsuccessful later preparation step.

Restoring exact private capacities or allocation addresses is not required.

This avoids expensive rollback machinery without weakening observable correctness.

---

# 12. Storage ownership and allocation

The topology builder is the sole owner of its registered observation and evidence storage.

Each table is a dynamically growable typed array.

The number of records is not known in advance.

Array capacity may increase geometrically.

Growth must be checked for integer overflow, allocation-size overflow and temporary-ID exhaustion.

Representational limits are reported separately from allocation failure.

## Why standard dynamic allocation

The topology builder manages several independently growing arrays.

A conventional malloc/realloc approach matches that storage pattern directly.

A bump-style arena would not automatically provide efficient independent array growth.

Using an arena could require allocating replacement arrays and retaining abandoned blocks until the arena is reset.

That additional complexity offers little demonstrated benefit for this construction-only subsystem.

## Decision

Use standard dynamically managed storage for the topology builder, with a private deterministic allocation-failure testing mechanism.

Retain builder-owned copies of all accepted metadata.

No caller-owned pointer may be retained as a substitute for copying required information.

The builder is single-writer. Concurrent mutation requires external synchronization.

This decision does not dictate the allocator architecture for tensors, GPU memory or future storage providers.

---

# 13. Builder lifecycle

The existing Slice 1 builder-creation and destruction semantics remain the foundation.

Creating a builder produces a live, initially empty construction object.

Destroying it releases all storage owned by that builder and clears the caller's builder pointer.

Repeated destruction is safe under the existing pointer-to-pointer lifecycle contract.

Slice 2 extends destruction to include all owned observation and evidence arrays.

It does not implement canonical topology finalization.

Later successful finalization will produce an immutable runtime-topology snapshot.

Temporary observation IDs and raw discovery evidence must not become permanent runtime handles.

---

# 14. Discovery orchestration is separate from registration

The topology builder records information supplied to it.

It does not decide which hardware APIs to query.

It does not decide how many discovery adapters to run.

It does not automatically retry an adapter.

Those responsibilities belong to future discovery orchestration.

An optional adapter reporting no available devices is not necessarily a fatal runtime failure.

A fatal builder-registration failure is different.

If a construction attempt cannot be completed reliably, the orchestrator must not publish the partially populated builder as though discovery succeeded.

The affected construction attempt must be abandoned or otherwise explicitly recovered according to the future orchestration contract.

Slice 2 does not implement that orchestration policy.

A successfully committed observation may temporarily exist without all its intended evidence. The failure of a subsequent registration does not invalidate or remove that observation.

Registration failure must be reported to the caller. The discovery adapter is responsible for distinguishing legitimately unavailable optional evidence from a failure to register evidence it intended to preserve.

A fatal registration failure renders the current construction attempt unsuccessful unless an explicit recovery policy resolves the failure. Such an attempt must not proceed to successful topology publication.

Out-of-memory failures preserve the builder's committed logical state, but do not imply that discovery can safely continue or that the resulting observations are sufficiently complete for finalization.

---

# 15. Rejected architectural alternatives

**Flat device observations:** Cannot reliably distinguish physical resources from schedulable execution targets.

**One builder per source:** Requires builder merging and complicates observation references.

**Per-source ID ranges:** Introduces artificial capacity limits and unnecessary bookkeeping.

**Composite source-local IDs:** Adds reference complexity when shared builder-owned arrays already provide unique typed IDs.

**Embedded identity and locator fields:** Restricts the number and kinds of evidence an observation can carry.

**One generic relationship array:** Requires runtime interpretation of heterogeneous endpoint types.

**One mixed locator-owner array:** Requires tagged owner interpretation rather than separate physical/execution owner types.

**Registration-time deduplication:** Risks losing provenance and contradictory evidence.

**Registration-time cross-source reconciliation:** Introduces unsupported identity assumptions into the observation layer.

**Registering invented memory identities:** Creates misleading evidence without a justified namespace.

**Requiring every observation to contain complete identity and location information:** Excludes partially visible, virtualized and partitioned hardware configurations.

**Using an arena for builder arrays:** Adds complexity without a demonstrated requirement.

---

# 16. Consequences

## Benefits

- Independent discovery evidence is preserved.
    
- Observation provenance remains recoverable.
    
- Partitioned hardware can be represented.
    
- Aggregate execution targets can report multiple physical backing relationships.
    
- Memory domains remain independent of backend allocation mechanisms.
    
- Discovery does not depend on premature canonical identity decisions.
    
- Typed arrays reduce category confusion.
    
- Registration failure cannot corrupt previously committed records.
    
- The builder can accept incomplete but structurally valid discovery reports.
    
- Future reconciliation can use evidence without reconstructing information lost during registration.
    

## Costs

- More typed arrays and registration interfaces.
    
- Additional validation logic.
    
- Separate evidence records require source and owner lookups.
    
- Raw duplicate reports may increase builder memory usage.
    
- Cross-source equivalence remains unresolved until later slices.
    
- Some valid hardware associations cannot be expressed directly under the same-source-only relationship rule.
    
- A separate finalization system must reconcile evidence, normalize metadata and validate the canonical graph.
    

These costs are accepted because they preserve correctness and architectural separation.

---

# 17. Explicit implementation boundaries

Slice 2 implements discovery-source registration, four observation categories, supported identity claims, physical locators and three structural relationship categories.

It does not implement:

- Hardware API discovery adapters
    
- Cross-source identity reconciliation
    
- Physical-association resolution across sources
    
- Canonical topology finalization
    
- Complex hierarchy graph validation
    
- Backend initialization
    
- Backend capability evaluation
    
- GPU memory allocation
    
- Tensor allocation or tensor semantics
    
- Performance measurements
    
- Planner policies
    
- Automatic runtime discovery orchestration
    

Those systems must consume or build upon the observation model without changing what Slice 2's records mean.

---

# 18. Required invariants

1. Every registered observation belongs to exactly one registered source.
    
2. Source kind and source ID are distinct concepts.
    
3. A source kind is registered at most once per builder.
    
4. Zero is invalid for temporary IDs.
    
5. Temporary IDs are typed and local to one builder.
    
6. Failed registrations consume no ID.
    
7. Observation records are never silently merged during registration.
    
8. Metadata is copied into builder-owned storage.
    
9. Identity kinds retain their documented namespaces.
    
10. Identity claims are not interchangeable with physical locators.
    
11. Identity owner categories must be valid for their kinds.
    
12. Locator owners are explicitly typed.
    
13. Relationship endpoints must belong to the declared reporting source.
    
14. Direct self-parenting is rejected.
    
15. Complex graph validation remains outside registration.
    
16. Duplicate valid evidence is preserved.
    
17. Memory observations do not require identity claims.
    
18. Backend observations do not prove executable capabilities.
    
19. Failed registration preserves committed records.
    
20. All builder-owned allocations are released during destruction.
    
21. The builder is single-writer.
    
22. No finalized topology IDs are assigned during observation registration.
    

---

# 19. Architectural result

At the end of Slice 2, Kestrel will have a mutable construction object capable of collecting source-specific observations and the evidence describing them.

It will know what each source reported.

It will know which observations own particular identifiers and locators.

It will know which source-local structural relationships were reported.

It will **not yet know which independently reported observations are the same canonical entities**.

That distinction is intentional.

Slice 2 establishes the evidence-preserving construction layer upon which later reconciliation and finalization depend.


# KES-004 Slice 2 — Amendment 1: Single Active Topology Builder

**Applies to:** KES-004 Slice 2, Discovery Observation Storage and Registration  
**Decision:** One live topology builder per process  
**Status:** Proposed amendment to the approved ADR  
**Affected dependency:** KES-004 Slice 1 builder lifecycle

---

## Amendment to §3 — Discovery sources

### Single construction context

Kestrel's topology discovery operates through one active topology builder.

All participating discovery mechanisms register their observations and evidence into that same builder. This includes Linux, CPUID, CUDA, Vulkan, NVML, Level Zero and any explicitly supported advanced discovery integrations.

A discovery source does not own an independent topology builder.

Advanced discovery integrations participate in the active construction context. Their participation does not require creating another builder or merging independently constructed topologies.

The source registry and all observation and evidence arrays remain shared within the active builder.

The source ID identifies provenance within that construction context; it does not identify a separate builder or globally unique discovery session.

**Decision:** Kestrel permits at most one live topology builder within a process at any given time.

This rule concerns topology construction. It does not restrict the number of physical devices, execution targets, discovery sources or memory domains represented by the builder.

It does not impose a machine-wide restriction across independent operating-system processes.

---

## Replacement for §4 — Consequence: Temporary ID lifetime and validity

Temporary observation IDs are local to a topology construction attempt.

They are strongly typed, one-based identifiers corresponding to records in the active builder's typed arrays.

They remain valid only while the originating builder exists and the referenced record remains registered.

The single-active-builder policy prevents two different live builders from issuing numerically overlapping temporary IDs within the same process at the same time.

However, temporary ID values may be reused by later construction attempts.

For example, a destroyed builder may have issued execution-observation ID 1. A subsequently created builder may also issue execution-observation ID 1.

If a stale ID from the destroyed builder is passed into the new builder, its numeric representation alone does not establish its original ownership.

The registration API may accept that value if it identifies an otherwise valid record satisfying the applicable structural and source-ownership checks.

Therefore, **the single-active-builder rule is not a guarantee of stale-ID detection**.

Callers must not retain or reuse temporary observation IDs after their originating builder has been destroyed.

The construction lifecycle and expert integration contracts must enforce this usage boundary to the extent supported by their APIs.

This limitation is accepted without introducing generation counters, globally unique temporary handles or builder-incarnation tokens.

Temporary IDs must never be exposed as finalized runtime resource handles.

---

## Addition to §12 — Lifecycle synchronization versus builder mutation

The builder remains single-writer.

Observation and evidence registration operations do not become independently thread-safe merely because only one builder may exist.

Concurrent mutation of the active builder requires external synchronization under the existing registration contract.

The process-wide restriction on builder creation and destruction introduces a separate synchronization requirement.

The implementation must prevent concurrent creation attempts from both successfully establishing live builders.

The mechanism used to claim and release the active-builder slot must therefore be synchronized independently of ordinary builder mutation.

This synchronization requirement does not authorize concurrent registration or change the existing single-writer semantics.

---

## Addition to §13 — Single active builder lifecycle

### Lifecycle invariant

At most one live topology builder may exist within a Kestrel process at any time.

A builder is considered live after successful creation and remains live until destruction completes.

The process maintains a private construction-lifecycle guard that enforces this invariant.

### Creation

If no live builder exists, a creation attempt may proceed according to the existing Slice 1 creation contract.

On successful creation, the new builder becomes the sole live builder in the process.

If a live builder already exists, another creation attempt must fail with the appropriate invalid-state result.

A rejected creation attempt must not modify, destroy or replace the existing builder.

A failed creation attempt must not permanently reserve the active-builder slot.

Concurrent creation attempts must never both succeed while their returned builders remain live.

The existing creation API's output-pointer and error semantics must remain consistent with this new failure case.

### Destruction

Destroying the live builder releases all storage owned by that builder.

The implementation must release the active-builder reservation as part of successful destruction.

After destruction has completed, a new builder may be created for a subsequent construction attempt.

Repeated destruction through the existing pointer-to-pointer contract remains safe.

Destruction of an already-null builder pointer must not release a reservation belonging to a different live builder.

### Construction attempts

Every independent topology construction attempt uses a fresh builder.

The architecture does not support multiple simultaneous builders or merging observations between builders.

A new builder may be required for explicit reconstruction, failed-attempt recovery or isolated tests, but only after the previous builder has been destroyed.

Tests requiring independent construction scenarios may execute them sequentially.

### Relationship to finalization

Slice 2 does not implement finalized topology construction.

Future successful finalization will produce an immutable runtime-topology snapshot.

The builder remains a temporary construction object and is destroyed after it is no longer required.

The single-active-builder rule governs mutable builders, not the lifetime of previously finalized immutable topology snapshots.

---

## Addition to §14 — Automatic-first construction and advanced participation

### Automatic-first operation

Ordinary Kestrel users do not need to create a topology builder, register discovery sources or manage temporary observation IDs.

The future runtime initialization system is responsible for establishing and coordinating the topology construction attempt.

It supplies the active construction context to supported discovery adapters.

Those adapters register observations and evidence into the shared builder.

The orchestration layer controls the sequence of discovery, handles fatal construction failures and determines whether the attempt may proceed to reconciliation and finalization.

### Advanced user control

Advanced users may require control over hardware visibility, backend selection, execution-target preferences or other topology-related policies.

Such preferences must not be represented as fabricated hardware-discovery observations.

Advanced configuration of how Kestrel uses discovered resources is conceptually separate from the registration of factual discovery evidence.

Where expert discovery extensions are supported, they participate in the same active construction context through an explicitly authorized registration interface.

They do not create an additional active builder.

They must respect the source registry, typed observation IDs, registration validation rules, builder lifecycle and construction boundaries.

### Construction versus published topology

The builder exists to collect mutable discovery evidence.

After canonical topology finalization, runtime consumers operate against the finalized topology and its separately validated configuration.

The published topology is not directly mutated through the temporary discovery builder.

Future topology reconstruction, if supported, begins a new construction attempt rather than reopening a destroyed builder.

### Scope limitation

This amendment defines the intended automatic-first interface boundary and construction ownership.

It does not implement discovery orchestration, user configuration policies, adapter scheduling or topology reconstruction in Slice 2.

---

## Additions to §15 — Rejected architectural alternatives

**Multiple simultaneous builders:** Rejected because Kestrel's intended discovery flow requires one shared construction context. Independent concurrent builders introduce unnecessary ownership ambiguity and would require coordinating or preventing inconsistent construction attempts.

**Independent builders for advanced users:** Rejected. Authorized expert discovery integrations contribute through the active builder, while ordinary advanced execution preferences belong to separate validated configuration mechanisms.

**Process-lifetime permanent builder:** Rejected. The mutable builder is temporary and is destroyed when its construction attempt is finished. Restricting simultaneous builders does not prevent future sequential construction attempts.

**Globally unique temporary observation handles:** Not adopted for Slice 2. One active builder prevents concurrent-builder confusion, while construction-lifetime discipline and temporary-ID contracts are accepted in place of generation-based stale-ID detection.

---

## Additions to §18 — Required invariants

23. At most one live topology builder exists within a process at any time.
    
24. All discovery sources participating in a construction attempt register into the same builder.
    
25. A second builder-creation attempt fails while another builder remains live.
    
26. Concurrent builder-creation attempts cannot both establish live builders.
    
27. A failed creation attempt does not leave a permanent active-builder reservation.
    
28. Destruction releases the builder's storage and active-builder reservation.
    
29. A new construction attempt may begin after the previous builder has been destroyed.
    
30. Temporary observation IDs must not be reused across builder lifetimes, even when their numerical values coincide.
    
31. The single-active-builder restriction does not establish automatic detection of stale temporary IDs.
    
32. Automatic-first runtime construction and advanced discovery integrations share the same construction context.
    
33. Advanced execution configuration is conceptually separate from raw hardware-discovery evidence.
    
34. The single-active-builder rule does not imply thread-safe concurrent registration.
    
35. Previously finalized immutable topology snapshots are not mutable builders and are outside the single-active-builder restriction.
    

---

## Consequence of the amendment

The topology builder becomes an explicitly controlled, temporary construction resource with process-wide single-instance lifetime semantics.

The shared source registry, four observation arrays, identity and locator evidence, structural relationships, registration validation and transactional guarantees are unchanged.

The additional implementation obligation is a synchronized builder-lifecycle guard, together with corresponding creation, destruction and concurrency tests.

This decision avoids foreign-builder confusion during normal active construction without claiming that positional temporary IDs provide stale-handle protection across sequential construction attempts.

**The amendment does not introduce builder merging, registration-time reconciliation, permanent temporary IDs or mutable finalized topology snapshots.**