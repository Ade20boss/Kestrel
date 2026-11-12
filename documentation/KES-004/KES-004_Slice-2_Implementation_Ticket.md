# Texyn Labs — Kestrel Engineering Implementation Ticket

# KES-004 — Slice 2: Discovery Observation Storage and Registration

**Version:** Consolidated review draft — 11 October 2026  
**Status:** PROPOSED — Awaiting review and approval  
**Priority:** Critical  
**Language:** C  
**Subsystem:** Heterogeneous Runtime / Topology Construction  
**Depends on:** Approved KES-004 Slice 2 ADR (including one-active-builder decision), Slice 1, Kestrel core status/failure contracts  
**Blocks:** Discovery adapters; reconciliation; immutable canonical topology; later memory/storage, capability and execution work  
**Owner:** Kestrel engineering

> **One document, one review.** This consolidated ticket replaces the two proposed documents previously called *KES-004 Slice 2 Engineering Implementation Ticket* and *KES-004 Slice 2 Implementation Contract Freeze*. It combines the implementation requirements, frozen contract proposals, work packages, validation matrix and acceptance criteria without requiring the reader to consult the second draft. The approved ADR remains architectural authority; this document is still a proposal until explicitly approved.

## Review decisions requiring explicit confirmation

The following are *new specific implementation decisions*, not all part of the previously approved architectural ADR. **Approval of this consolidated ticket approves them**, except where marked OPEN:

1. Add `KESTREL_STATUS_LIMIT_EXCEEDED = 4` after the existing four status values.
2. Introduce five typed 32-bit temporary ID classes and the seven initial discovery-source kinds.
3. Use the registration operation names and parameter order set out in §4.
4. Adopt precise NVIDIA GPU/MIG text grammars; 16-byte CUDA/Vulkan/Level Zero binary UUID formats; and the typed PCI/DRM locator representations set out in §§7–8.
5. Retain the existing legacy PCI/DRM identity enum values without allowing them in new identity-claim registration.
6. Use twelve independently growable builder tables, initial capacity 4, checked geometric doubling, and the private deterministic failure-injection semantics below.
7. Implement the one-active-builder guard with thread-safe creation reservation while keeping registration single-writer.
8. **OPEN DECISION — Legacy MIG GI/CI decimal leading zeros:** the original contract forbids them except for zero; a later correction proposed accepting and normalizing them. Both choices are identified in §7. Select one during this review; do not silently pick during implementation.

Other than that last explicit review point, the two sources had no competing high-level Slice 2 objective: one specified the work; the other made the contracts precise.

---

## 1. Objective and delivery boundary

Implement the mutable, evidence-preserving topology construction layer described in the approved KES-004 Slice 2 Architecture Decision Record.

At the end of this slice, Kestrel must be capable of accepting discoveries from multiple independent hardware and execution APIs, recording their observations and associated evidence in one builder-owned construction context, and validating each registration operation without making premature decisions about canonical hardware identity.

This slice must implement actual registration functionality, not merely allocate arrays or declare placeholder types.

It must support:

1. Discovery-source registration.
2. Four typed observation categories.
3. Supported physical and execution identity claims.
4. Typed PCI and Linux DRM physical locators.
5. Three typed structural relationship categories.
6. Builder-owned dynamic storage.
7. Strict local validation.
8. Transactional registration.
9. Deterministic allocation-failure testing.
10. Complete builder destruction.
11. The agreed single-active-builder lifecycle policy.

The output of this slice remains **a mutable topology builder containing observations and evidence**.

It is not a canonical topology.

**Slice 2 is complete only when these registration operations are implemented, tested and validated against the ADR.**

---

## 2. Architectural authority and evolution from old tickets

### 2.1 Source of truth

Implementation must conform to:

- The approved KES-004 Slice 2 ADR, *Discovery Observation Storage and Registration*.
- The accepted KES-004 canonical topology model.
- The existing Slice 1 public topology types and lifecycle contracts.
- The agreed decision to permit at most one live topology builder per process.
- Existing Kestrel conventions for statuses, numeric limits, memory safety and testing.

Where an older KES-004 ticket conflicts with the approved Slice 2 ADR, the newer ADR takes precedence for Slice 2.

### 2.2 Correction to the old implementation sequence

The older KES-004 ticket divided registration into:

- Slice 2 — Dynamic arrays.
- Slice 3 — Observation registration.
- Slice 4 — Evidence registration.

That separation has been superseded by the approved Slice 2 ADR.

For this implementation ticket, Slice 2 includes **storage, observations and evidence registration together**.

Later KES-004 slice numbers must be updated consistently in the parent engineering plan.

This ticket does not authorize reconciliation or finalization merely because the earlier numbering assigned them to later slices.

---

## 3. Scope boundaries

### Included

This slice implements the receipt and retention of discovery evidence.

Every successfully registered record must remain accessible to the builder's later reconciliation and canonicalization stages.

The implementation must preserve:

- Observation provenance.
- Identity namespaces.
- Owner categories.
- Physical locations.
- Source-local structural relationships.
- Conflicting but structurally valid evidence.
- Duplicate valid evidence.
- Incomplete but valid observations.

### Excluded

Do not implement any of the following in this ticket:

- Calling CUDA, Vulkan, NVML, Level Zero or Linux discovery APIs.
- Automatic discovery adapter invocation.
- Discovery adapter scheduling.
- Cross-source reconciliation.
- Identity equivalence resolution.
- Canonical topology ID assignment.
- Immutable topology finalization.
- Full execution-hierarchy cycle validation.
- Backend initialization.
- Backend operation capability evaluation.
- Memory-provider or allocation logic.
- Tensor allocation.
- Kernel execution.
- Scheduling or planning.
- Performance measurement.
- Runtime hardware-policy selection.
- Configuration UI or user-facing device management.

The builder records submitted facts. It does not determine whether those facts describe the same physical or schedulable entity.

---

## 3A. Repository layout and API ownership (binding contract proposal)

Use the existing repository file layout:

- `include/kestrel_topology.h` — common topology definitions, canonical topology types, existing identity representation and lifecycle declarations.
- `include/kestrel_topology_builder.h` — new expert discovery-registration interface.
- `include/kestrel_core.h` — existing status vocabulary, extended as described below.
- `src/kestrel_topology.c` — existing lifecycle implementation, extended for builder storage and lifecycle ownership.
- `tests/test_topology.c` — existing Slice 1 tests, retained and expanded.
- `tests/test_topology_builder.c` — dedicated Slice 2 registration and failure tests.

An additional private builder implementation source/header may be introduced if useful.

The finalized topology public records remain independent of raw builder observations.

All builder internals remain opaque.

The registration header may be used by Kestrel's trusted discovery adapters and explicitly supported expert integrations.

Ordinary tensor/runtime users are not required to include it.

---

## 3B. Public status code taxonomy (binding contract proposal)

Preserve the existing status values and meanings:

| Status | Numeric value |
|---|---:|
| `KESTREL_STATUS_SUCCESS` | 0 |
| `KESTREL_STATUS_INVALID_ARGUMENT` | 1 |
| `KESTREL_STATUS_INVALID_STATE` | 2 |
| `KESTREL_STATUS_OUT_OF_MEMORY` | 3 |
| `KESTREL_STATUS_LIMIT_EXCEEDED` | 4 — new |

Append `LIMIT_EXCEEDED` without renumbering existing values.

Interpret the statuses as follows:

**INVALID_ARGUMENT**

- Missing required pointer.
- Invalid or unsupported enum.
- Malformed descriptor.
- Invalid identity encoding or locator.
- Zero or nonexistent source/observation ID.
- Direct hierarchy self-parenting.

**INVALID_STATE**

- Duplicate registered source kind.
- Otherwise valid observation referenced through the wrong reporting source.
- An additional builder-creation attempt while one is active.
- Any other explicitly documented violation of live construction state.

**OUT_OF_MEMORY**

An allocator reports failure for a representable allocation request.

**LIMIT_EXCEEDED**

The requested record, ID, count, capacity or allocation size exceeds an applicable representation limit, independently of allocator success.

Do not map arithmetic overflow to OUT_OF_MEMORY.

Registration argument validation should take precedence over allocation attempts. Malformed inputs should not trigger unnecessary storage growth.

---

## 3C. Temporary source and observation ID representation

Define five additional strongly typed wrappers around unsigned 32-bit values:

1. Discovery source ID.
2. Physical observation ID.
3. Execution observation ID.
4. Memory observation ID.
5. Backend observation ID.

These are distinct from the canonical ID wrappers already present in `kestrel_topology.h`.

Rules:

- Zero is invalid.
- The first committed record in a category has ID 1.
- Subsequent committed records have contiguous IDs.
- Each category has an independent sequence.
- The corresponding array index is ID minus one.
- IDs do not contain the source kind.
- IDs do not contain a generation counter.
- Failed registration does not consume an ID.
- IDs cease to be valid when the builder is destroyed.

A successful registration may return ID `UINT32_MAX` if representable storage exists for it. Attempting to create a further record in that ID-bearing category returns LIMIT_EXCEEDED.

No category may wrap around to zero.

Source IDs are builder-wide. Observation IDs are builder-wide within their respective categories, not per discovery source.

Evidence registrations do not need separate public IDs in Slice 2.

---

## 3D. Discovery-source kinds and registry descriptor

Introduce a discovery-source enum with zero reserved for INVALID.

The initial supported source kinds are:

| Source kind | Intended mechanism |
|---|---|
| `HOST_CPU` | Kestrel's portable host execution discovery |
| `LINUX_SYSFS` | Linux hardware and OS topology |
| `X86_CPUID` | x86 processor identification and features |
| `CUDA_DRIVER` | CUDA Driver API |
| `VULKAN` | Vulkan discovery |
| `NVIDIA_NVML` | NVIDIA management discovery |
| `LEVEL_ZERO` | Intel/oneAPI Level Zero discovery |

Each source kind may be registered at most once per builder.

A source descriptor contains exactly one source kind in Slice 2.

No arbitrary source name, platform pointer, API context pointer, callback, ordinal or native handle is stored in the source registry.

A source that reports zero observations is valid.

Additional source kinds may be introduced in later versions without changing the source-ID representation.

This is a closed initial vocabulary, not a claim that all future discovery mechanisms are already supported.

A registered source kind does not establish backend availability.

---

## 4. Public registration API: operations, parameters, output semantics

All registration functions return `kestrel_status_t`.

Use the following public operation names and parameter order.

### Source registration

**`kestrel_topology_builder_register_source`**

Inputs, in order:

1. Builder pointer.
2. Const source descriptor pointer.
3. Output source-ID pointer.

Commits one source record.

### Observation registration

**`kestrel_topology_builder_register_physical_observation`**

**`kestrel_topology_builder_register_execution_observation`**

**`kestrel_topology_builder_register_memory_observation`**

**`kestrel_topology_builder_register_backend_observation`**

Each receives, in order:

1. Builder pointer.
2. Reporting source ID.
3. Const category-specific descriptor pointer.
4. Output typed observation-ID pointer.

Each commits one observation to its corresponding table.

### Identity registration

**`kestrel_topology_builder_register_physical_identity`**

**`kestrel_topology_builder_register_execution_identity`**

Each receives:

1. Builder pointer.
2. Reporting source ID.
3. Appropriately typed observation-owner ID.
4. Const `kestrel_identity_value_t` pointer.

These operations return status only; no evidence ID is assigned.

### Locator registration

**`kestrel_topology_builder_register_physical_locator`**

**`kestrel_topology_builder_register_execution_locator`**

Each receives:

1. Builder pointer.
2. Reporting source ID.
3. Appropriately typed observation-owner ID.
4. Const locator descriptor pointer.

These return status only.

### Structural evidence registration

**`kestrel_topology_builder_register_backing`**

Inputs: builder, reporting source ID, execution observation ID, physical observation ID.

**`kestrel_topology_builder_register_subtarget`**

Inputs: builder, reporting source ID, child execution observation ID, parent execution observation ID.

**`kestrel_topology_builder_register_execution_memory`**

Inputs: builder, reporting source ID, execution observation ID, memory observation ID.

These operations return status only.

### Common API behavior

Registration operations do not create or destroy builders.

Registration operations never invoke hardware APIs.

Registration operations do not expose pointers to internal records.

Every registration copies or normalizes supplied data into builder-owned storage.

For any operation returning an ID:

- A null output-ID pointer is INVALID_ARGUMENT.
- When a nonnull output-ID pointer is supplied, write the invalid-zero value before any potentially failing validation or allocation.
- On success, replace zero with the assigned ID.
- On failure, leave the output ID at zero.

Do not dereference invalid/null pointers.

A caller passing arbitrary dangling or corrupted pointers is outside the guaranteed contract.

---

## 5. Observation descriptor representations and string rules

Use fixed-size, bounded descriptors. Avoid heap-allocated names inside individual observation records.

### Physical observation

Fields:

- Compute class.
- Name: 128-byte character array.
- Vendor: 64-byte character array.

The reporting source ID is a separate registration argument and is stored in the observation record.

### Execution observation

Fields:

- Compute class.
- Name: 128-byte character array.

The source ID is stored separately.

### Memory observation

Fields:

- Memory kind.
- Name: 128-byte character array.
- `capacity_known`: Boolean.
- `capacity_bytes`: unsigned 64-bit integer.

If capacity is unknown, capacity bytes must be zero.

If capacity is known, zero bytes is permitted; the registration layer does not invent a hardware-capacity requirement.

### Backend observation

Fields:

- Backend family.
- Execution observation ID.

The execution observation must exist and belong to the reporting source.

The initial backend families remain SCALAR, AVX2, VULKAN and CUDA.

Do not add a Level Zero backend family in Slice 2 merely because Level Zero is a supported discovery source.

Do not require a source kind to match a particular backend family at this registration layer. Source/adapter scheduling and backend availability remain later responsibilities. The recorded provenance makes such policies possible without hard-coding them into generic storage.

### String rules

All fixed character arrays must contain a NUL byte within their declared capacity.

Empty names/vendors are permitted unless a future adapter-specific contract requires otherwise.

Strings are copied through the first NUL, including that terminator.

All remaining destination bytes are set to zero.

Input bytes following the first NUL are ignored, not preserved as meaningful metadata.

An unterminated input array is rejected.

No silent truncation is permitted.

No UTF-8 validity requirement is imposed in Slice 2; names remain bounded byte strings with NUL termination.

Supported compute and memory UNKNOWN values are valid. INVALID values and unrecognized enum values are rejected.

---

## 5A. Internal typed storage: required twelve tables and ownership


The builder must own twelve separate typed, dynamically growable tables.

The tables are grouped as follows.

| Group             | Required storage table         | Purpose                                        |
| ----------------- | ------------------------------ | ---------------------------------------------- |
| Discovery sources | Source records                 | Provenance registry                            |
| Observations      | Physical observations          | Reported physical compute resources            |
| Observations      | Execution observations         | Reported schedulable compute targets           |
| Observations      | Memory observations            | Reported memory domains                        |
| Observations      | Backend observations           | Reported execution backend associations        |
| Identity evidence | Physical identity claims       | Identifiers attached to physical observations  |
| Identity evidence | Execution identity claims      | Identifiers attached to execution observations |
| Locator evidence  | Physical observation locators  | Physical/OS location evidence                  |
| Locator evidence  | Execution observation locators | Execution-target physical/OS location evidence |
| Relationships     | Execution-to-physical backing  | BACKED_BY evidence                             |
| Relationships     | Execution hierarchy            | SUBTARGET_OF evidence                          |
| Relationships     | Execution-memory association   | Memory relationship evidence                   |

A future memory-identity storage extension is permitted by the architecture, but no functional memory-identity registration is required or authorized in this slice.

---

## 5B. Exact checked dynamic-array growth and commit protocol

The builder owns twelve typed tables.

Every table has:

- Element storage.
- Committed record count.
- Allocated capacity.

Use `size_t` for private counts and capacities.

For source and observation tables, counts must also satisfy the unsigned 32-bit ID representation limit.

### Growth policy

Initial capacity on first required allocation: **4 elements**.

Subsequent required growth:

- Double capacity until sufficient for the requested count.
- Check multiplication before performing it.
- Never exceed the applicable table limit.
- If doubling would exceed a limit but the requested count remains representable, use the greatest representable permitted capacity.
- Reject the request with LIMIT_EXCEEDED when required capacity cannot be represented.

This policy is the default for all twelve tables.

No automatic shrinking is required.

### Allocation-size checks

Before allocation:

- Verify required count addition cannot overflow.
- Verify capacity multiplication cannot overflow.
- Verify `capacity × sizeof(element)` fits `size_t`.
- Enforce ID limits for source and observation arrays.

Use ordinary heap allocation and reallocation.

Never overwrite the only pointer to a live allocation with the direct result of an unsuccessful reallocation.

### Commit protocol

Construct each candidate record completely before making it logically visible.

When storage growth is needed, prepare the required capacity.

After all failure points have passed:

- Write the committed record.
- Increment the logical count.
- Publish its output ID where applicable.

A failed append must not advance the count.

Private unused capacity need not be restored to its prior size after failure.

All committed fields must be initialized.

Private structure padding bytes do not need to be part of the externally tested semantic contract.

---

## 5C. Atomic single-active-builder lifecycle enforcement

Extend the existing builder lifecycle without changing its public function signatures.

Use a private process-wide atomic reservation state for builder creation.

The lifecycle mechanism must prevent two independent creation attempts from both claiming the active-builder slot.

A suitable implementation approach is a C11 atomic compare-and-exchange reservation.

Contract:

- Creation first validates the caller's output pointer state.
- One creation attempt claims the reservation.
- If the reservation is already held, return INVALID_STATE.
- If subsequent builder allocation fails, release the reservation.
- On success, the reservation remains held for the builder lifetime.
- Destroying the live builder releases owned storage, then releases the reservation.
- Destruction must not release the reservation prematurely.
- A null destruction operation changes no reservation state.
- After successful destruction, another creation attempt may succeed.

Do not introduce a process-wide registry of multiple builders.

Do not introduce builder merging.

### Concurrency scope

The atomic reservation makes competing creation attempts safe under the documented lifecycle contract.

It does **not** make concurrent observation registration safe.

It does **not** make destroying a builder concurrently with another thread using it safe.

The caller must ensure no registration, inspection or other builder operation remains active when destruction begins.

### Existing failure contract

Creation retains its existing caller-visible semantics:

- Success: null pointer becomes live.
- Failure: caller's builder pointer remains unchanged.
- Null pointer-to-pointer: INVALID_ARGUMENT.
- Nonnull existing pointer: INVALID_STATE.
- Allocation failure: OUT_OF_MEMORY.

---

## 6A. Discovery-source registration: validation and tests

### Required operation

Provide an operation that registers one discovery source using:

- The active builder.
- A source descriptor.
- A valid output pointer for the assigned source ID.

The exact function declaration is governed by the approved registration header.

### Source validation

Before registration:

1. Validate the builder.
2. Validate the output pointer.
3. Validate the source descriptor.
4. Validate the source-kind enum.
5. Validate any descriptor strings or other metadata.
6. Check that the source kind is not already registered.
7. Check source-ID representability.
8. Prepare storage capacity.
9. Commit the complete source record.
10. Return the new ID.

Each distinct source kind may appear at most once in a builder.

Do not silently return an existing source ID when a duplicate kind is submitted.

Do not identify source kind by examining numeric bits of the source ID.

### Source-ID assignment

Source IDs are:

- One-based.
- Strongly typed.
- Local to the builder.
- Stable during the builder's lifetime.
- Issued in successful registration order.

If source registration fails, no source ID is consumed.

If an output pointer is valid, it must receive zero when registration fails.

### Required test scenarios

- First source registration.
- Several distinct source kinds.
- Duplicate source kind.
- Invalid source kind.
- Invalid descriptor.
- Null builder.
- Null output pointer.
- Allocation failure.
- ID representational exhaustion.
- Builder destruction after one or more source registrations.

A source with zero observations must remain a valid registered source.

---

## 6B. Physical observation registration: validation and tests

### Meaning

A physical observation reports an identifiable root physical compute resource.

It does not assert a canonical physical-device identity.

### Required data

Store:

- Reporting source ID.
- Compute class.
- Name.
- Vendor.

Do not embed:

- A canonical physical-device ID.
- Execution targets.
- Backend instances.
- Memory-space IDs.
- Physical locators.
- Identity claims.

These relationships and evidence types have separate storage tables.

### Validation

Registration must reject:

- Invalid builder.
- Missing required output pointer.
- Unknown source ID.
- Invalid compute class.
- Malformed descriptor.
- Unterminated bounded strings.
- Unrepresentable new observation ID.
- Allocation failure.

The physical observation must retain its source ID.

### Successful result

Registration commits exactly one physical observation.

The returned physical observation ID identifies that record in the physical observation array.

The physical observation may initially have no UUID, no locator and no associated execution target.

That is valid.

### Required scenarios

Support registration of:

- CPU package observations.
- Physical GPU observations.
- Other broad accelerator observations.
- Multiple physical devices from one source.
- Similar or identical descriptions from different sources.

Identical names and vendors do not imply identical hardware.

No registration-time merging is permitted.

---

## 6C. Execution observation registration: validation and tests

### Meaning

An execution observation reports a schedulable compute target exposed by a discovery mechanism.

Examples include:

- Host CPU execution target.
- Whole-GPU execution target.
- MIG execution instance.
- Level Zero device or subdevice.
- Aggregate GPU execution target.
- Virtualized execution target.

### Required data

Store:

- Reporting source ID.
- Compute class.
- Name.

Do not embed:

- A physical parent.
- Execution parent.
- Memory association.
- Backend collection.
- Canonical execution-target ID.

All structural relationships remain separate.

### Validation

Apply the same builder, source, enum, bounded-string, ID-limit and allocation validations required for physical observations.

### ID assignment

Execution observation IDs have their own independent sequence.

For example, after registering physical observation P1, the next execution observation may still receive E1.

The first execution observation registered by CUDA and the next registered by Vulkan occupy the same builder-wide execution observation table.

They do not receive separate per-source ID ranges.

### Required behavior

Two independently reported execution observations must remain distinct records even when their names, compute classes or physical locators appear identical.

Registration must not infer equivalence or backing relationships.

---

## 6D. Memory observation registration: validation and tests

### Required data

Store:

- Reporting source ID.
- Memory kind.
- Name.
- Capacity-known indicator.
- Capacity in bytes.

The approved memory kinds are:

- SYSTEM
- DEVICE_LOCAL
- UNKNOWN

The invalid enum value must be rejected.

### Capacity rules

If capacity is unknown:

- Capacity-known must be false.
- Capacity bytes must be zero.

If capacity is known:

- Capacity-known must be true.
- Capacity bytes are interpreted as a 64-bit unsigned value.

Do not introduce an arbitrary nonzero-capacity requirement when capacity is marked known.

Reject inconsistent descriptors rather than silently guessing missing capacity information.

### Identity rule

Memory observations do not require identity claims.

No general-purpose memory identity namespace is frozen for Slice 2.

Therefore, this ticket must not introduce fabricated memory UUIDs or infer identity from:

- Name.
- Kind.
- Capacity.
- Equal reported size.
- Matching heap index without documented scope.

### Required scenarios

- System memory with known capacity.
- Device-local memory with known capacity.
- Unknown capacity represented correctly.
- Invalid capacity-known/byte-count combination.
- Two same-sized memory observations from different sources.
- Multiple memory observations with the same memory kind.

All structurally valid observations remain separate.

---

## 6E. Backend observation registration: validation and tests

### Meaning

A backend observation reports that a particular backend family is associated with one execution observation.

This is not proof that backend initialization has succeeded.

### Required data

Store:

- Reporting source ID.
- Backend family.
- Exactly one execution observation ID.

Supported families currently include:

- SCALAR
- AVX2
- VULKAN
- CUDA

Do not introduce new backend families as part of this slice.

### Validation

Registration must confirm:

- Builder exists.
- Source exists.
- Backend family is valid.
- Execution observation exists.
- The execution observation belongs to the declared reporting source.
- The record can be stored successfully.

A backend observation referring to an execution observation owned by another source must be rejected.

### Required semantics

Multiple backend observations may refer to the same execution observation when they satisfy the reporting-source rules.

Registration must not deduplicate backend observations or infer executable capability.

The backend observation itself is not an initialized native CUDA or Vulkan backend object.

Do not store persistent native backend handles as substitutes for observation metadata.

---

## 7. Identity claim formats, ownership and normalization

Reuse the existing `kestrel_identity_value_t` representation:

- Identity-kind enum.
- Unsigned 8-bit payload length.
- Fixed 64-byte payload array.

No new public identity container is required.

### Supported identity kinds

| Identity kind | Owner | Encoding |
|---|---|---|
| NVIDIA_GPU_UUID | Physical observation | Validated text |
| CUDA_DEVICE_UUID | Execution observation | Exactly 16 raw bytes |
| VULKAN_DEVICE_UUID | Execution observation | Exactly 16 raw bytes |
| NVIDIA_MIG_UUID | Execution observation | Validated text |
| LEVEL_ZERO_DEVICE_UUID | Execution observation | Exactly 16 raw bytes |

### Legacy enum compatibility

The existing public identity enum includes:

- PCI_BDF.
- LINUX_DRM_DEVICE.

Do not renumber those existing enum constants during Slice 2.

However, these are no longer accepted as identity claims by the Slice 2 registration interface.

Passing either value to physical-identity or execution-identity registration returns INVALID_ARGUMENT.

PCI and DRM information is submitted through the separate typed-locator operations.

The old enum constants may be formally removed or deprecated in a separately approved compatibility cleanup.

### Binary identity validation

For CUDA, Vulkan and Level Zero identity kinds:

- Payload length must be exactly 16.
- The payload is opaque binary data.
- It is copied byte-for-byte without endianness transformation.
- No textual UUID parsing is performed.
- All unused destination bytes are zeroed.
- Bytes beyond the declared input length are ignored.
- The registration layer does not perform cross-kind equivalence comparisons.

Do not reject a 16-byte value based solely on its byte pattern unless the defining API explicitly makes that pattern invalid.

### NVIDIA physical GPU UUID

Accept text in the following complete form:

`GPU-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`

Each `x` is a hexadecimal digit.

Requirements:

- Total length: exactly 40 bytes.
- Prefix: uppercase `GPU-`.
- UUID groups: 8-4-4-4-12 hexadecimal digits.
- Hyphens at the prescribed positions.
- No leading/trailing whitespace.
- No embedded NUL in the declared payload.
- Uppercase or lowercase hexadecimal digits accepted.
- Stored hexadecimal letters normalized to lowercase.
- Prefix case preserved as documented.

Text length is given by the identity value's `length` field; no separate NUL terminator is required inside the payload.

### NVIDIA MIG UUID

Accept either of two complete forms:

**Modern form**

`MIG-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`

Length: exactly 40 bytes.

Apply the same UUID-group validation and hexadecimal normalization as above.

**Legacy form**

`MIG-GPU-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx/GI/CI`

Here GI and CI are unsigned decimal GPU-instance and compute-instance identifiers.

Requirements:

- Complete UUID grammar as above.
- Exactly two slash-separated decimal fields.
- Each numeric field contains at least one digit.
- No signs, spaces or nondecimal characters.
- Numeric values must fit an unsigned 32-bit integer.
- No leading zeros except for the value zero.
- Complete payload length must not exceed 64 bytes.
- Hexadecimal letters normalized to lowercase.
- No conversion between the legacy and modern MIG namespaces is attempted.

Unsupported or oversized textual forms return INVALID_ARGUMENT.

No partial identifier is retained.

A zero-filled unused payload tail is required in builder-owned storage.

### Multiple claims

Multiple claims on the same observation remain valid.

Exact duplicate claims are retained.

Structurally valid conflicting claims are retained.

Identity registration performs no cross-source reconciliation and does not decide which claim is authoritative.

No memory-identity registration exists in this slice.

---

### Open review detail — legacy MIG GI/CI leading-zero policy

The original contract's legacy MIG format rule states: **“No leading zeros except for the value zero.”** A subsequent review suggested the alternative: **accept decimal GI/CI with leading zeros, parse them as decimal and normalize to minimal decimal representation**. Both are compatible with the high-level ADR, but they produce different accepted inputs. This consolidated ticket deliberately leaves the choice **OPEN** until the engineer approves one. The final accepted text must replace the original rule before coding the parser. All other rules for nonnegative decimal integers and bounds remain as stated.

---

### Identity claim registration — mandatory tests

Test:

- Correct physical identity owner.
- Correct execution identity owner.
- Wrong owner category.
- Every supported binary identity kind.
- Valid textual NVIDIA forms.
- Malformed textual identifiers.
- Wrong binary lengths.
- Zero-length identifier where prohibited.
- Payload-length overflow.
- Unknown identity kind.
- Repeated valid identity.
- Multiple potentially conflicting identities.
- Unknown observation reference.
- Wrong source ownership.
- Allocation failure.
- No partially committed identity evidence after failure.

---

## 8. Typed PCI and Linux DRM locators: formats and validation

Introduce a locator-kind enum:

- INVALID = 0.
- PCI_BDF.
- LINUX_DRM_DEVICE.

A locator descriptor contains a locator kind and one tagged value corresponding to the selected kind.

Only the selected value is interpreted.

Inactive tagged storage is not required to be initialized by callers and is zeroed in the retained record.

### PCI BDF

Represent a PCI location using four numeric components:

| Component | Representation | Valid range |
|---|---|---|
| Domain/segment | Unsigned 32-bit | 0 through UINT32_MAX |
| Bus | Unsigned 8-bit | 0 through 255 |
| Device | Unsigned 8-bit | 0 through 31 |
| Function | Unsigned 8-bit | 0 through 7 |

The domain field is deliberately not restricted to a four-hex-digit number.

This initial locator describes conventional PCI BDF addressing.

Extended/alternative function representations require a later explicit extension.

Do not silently truncate unsupported function values.

No PCI hardware lookup is performed during registration.

### Linux DRM device node

Represent a Linux DRM node using:

- Node category: PRIMARY or RENDER.
- Unsigned 32-bit major device number.
- Unsigned 32-bit minor device number.

The supported Linux DRM device-node major is 226.

Reject other major numbers for this locator kind.

For the conventional minor ranges:

- 0–63: PRIMARY.
- 64–127: unsupported historical CONTROL range; reject.
- 128–191: RENDER.

For extended minor numbers at or above 192, retain the explicitly supplied PRIMARY or RENDER category without assuming the historical fixed ranges determine its category.

Registration validates the typed representation only.

It does not call `stat`, inspect `/dev/dri`, open a device node or verify current device existence.

Only PRIMARY and RENDER are supported in this slice.

### Locator semantics

Physical and execution observations may each own zero or more locators.

A physical and execution observation may share a PCI location.

Several execution observations may share a PCI location.

Duplicate valid locators are retained.

Matching locations do not establish execution identity.

Locator values remain distinct from identity claims.

---

### Locator registration — mandatory tests

- Valid PCI locator for a physical observation.
- Valid PCI locator for an execution observation.
- Same PCI locator on multiple execution observations.
- Invalid PCI device/function component.
- Valid DRM primary node.
- Valid DRM render node.
- Invalid DRM node kind or representation.
- Wrong owner category.
- Unknown owner.
- Wrong source ownership.
- Duplicate valid locator preserved.
- Allocation failure with unchanged logical record count.

---

## 9A. BACKED_BY registration contract and tests

### Meaning

BACKED_BY records a source-reported relationship from an execution observation to a physical observation.

Direction:

**Execution observation → Physical observation**

### Required inputs

- Active builder.
- Reporting source ID.
- Execution observation ID.
- Physical observation ID.

### Validation

Verify:

- Both endpoint IDs are valid and exist.
- Their categories are correct.
- Both belong to the declared reporting source.
- The relationship can be stored.

Do not allow a CUDA execution observation to directly register a BACKED_BY relationship to a Linux-owned physical observation.

That association belongs to later cross-source resolution.

### Cardinality

Support:

- Zero known physical backing observations.
- One known physical backing observation.
- Multiple backing observations.
- Multiple execution observations backed by the same physical observation.

Do not impose one-to-one cardinality.

### Duplicates

Repeated valid BACKED_BY evidence remains preserved during registration.

Canonical relationship deduplication belongs to later processing.

### Required tests

- One execution to one physical observation.
- Multiple execution observations sharing a physical observation.
- One execution observation backed by multiple physical observations.
- Unknown execution ID.
- Unknown physical ID.
- Cross-source endpoint.
- Repeated valid relationship.
- Allocation failure.

---

## 9B. SUBTARGET_OF registration contract and tests

### Meaning

SUBTARGET_OF records a source-reported direct execution hierarchy relationship.

Direction:

**Child execution observation → Parent execution observation**

### Required inputs

- Active builder.
- Reporting source ID.
- Child execution observation ID.
- Parent execution observation ID.

### Validation

Require:

- Both endpoint IDs exist.
- Both refer to execution observations.
- Both belong to the declared source.
- Child and parent IDs are different.
- Storage can be committed successfully.

A direct self-parent relationship must be rejected.

### Deliberately deferred validation

Do not perform full graph-cycle detection at registration.

Do not enforce the final canonical one-parent invariant here.

Do not reject all potentially contradictory but structurally valid parent evidence at registration.

For example, different non-self observations may form a hierarchy cycle after several separate registrations.

That must be preserved for later graph validation rather than silently altered.

Likewise, two distinct temporary observations may eventually reconcile to one canonical entity.

Registration cannot know that yet.

### Required tests

- Root and direct child.
- Nested subtarget chain.
- Direct self-parent rejection.
- Two-node cycle accepted as locally valid evidence, pending later graph validation.
- Multiple competing parent claims retained pending canonical validation.
- Unknown endpoint.
- Cross-source relationship.
- Duplicate valid relationship.
- Allocation failure.

---

## 9C. Execution-memory association contract and tests

### Meaning

Records a structural association between one execution observation and one memory observation.

It answers which reported memory domain is associated with a reported compute target.

### Required inputs

- Active builder.
- Reporting source ID.
- Execution observation ID.
- Memory observation ID.

### Validation

Require:

- Existing execution observation.
- Existing memory observation.
- Correct endpoint categories.
- Same-source ownership.
- Successful storage preparation.

### Cardinality

Support:

- Multiple execution observations associated with one memory observation.
- One execution observation associated with multiple memory observations.
- Observations without known associations.

No one-to-one assumption is permitted.

### Prohibited inferences

An execution-memory association does not guarantee:

- Allocation legality.
- Memory access legality.
- Direct access.
- Physical locality.
- Transfer support.
- Low latency.
- High bandwidth.
- Zero-copy behavior.

Do not attach such capability or performance information to this relationship.

### Required tests

- One-to-one association.
- Multiple executions sharing one memory observation.
- One execution linked to multiple memory observations.
- Missing execution ID.
- Missing memory ID.
- Cross-source association.
- Duplicate association.
- Allocation failure.

---

## 9D. Source-provenance and same-source enforcement

This section applies to all evidence and relationship operations.

Every registered observation has exactly one registered reporting source.

An evidence registration operation receives a declared reporting source ID.

The builder must confirm:

1. The source exists.
2. The owner observation exists.
3. The owner observation's source equals the declared source.
4. For structural relationships, all endpoints belong to the declared source.

The source ID need not be duplicated inside each relationship record when it can be recovered from the endpoints.

A supplied source ID is not a security capability or proof of caller identity.

This validation checks logical consistency; it does not authenticate discovery adapters.

No cross-source relationship may be committed during Slice 2.

---

## 10. Transactional registration and construction-failure policy

### Fundamental requirement

**Every individual registration operation is transactional.**

A successful registration commits one complete logical record.

A failed registration commits no new logical record.

Previously committed records remain intact.

This rule applies to:

- Sources.
- All four observation categories.
- Identity claims.
- Locators.
- All three relationship categories.

### Registration operation sequence

Each registration function must logically perform:

1. Validate required pointers and the live-builder state.
2. Initialize a valid output-ID destination to zero where applicable.
3. Validate descriptors, enums and bounded data.
4. Validate source and owner references.
5. Validate source ownership and local structural rules.
6. Validate representational capacity and arithmetic limits.
7. Construct or normalize the complete candidate record.
8. Prepare any necessary storage growth.
9. Commit the completed record.
10. Increment the corresponding committed count.
11. Return success and the new ID, if applicable.

The exact internal order may differ where necessary for correct cleanup, but **all potentially failing work must happen before the logical commit**.

### Failed operation guarantees

On failure:

- The candidate record does not exist.
- The relevant committed count is unchanged.
- Previous records retain their contents.
- No new source or observation ID is consumed.
- Valid output-ID pointers receive zero.
- No partially initialized logical record becomes registered.
- The builder remains internally valid.
- Builder destruction remains safe.
- No allocation owned by an abandoned candidate is leaked.

### Private capacity changes

A failed registration may leave private array capacity larger than it was before the attempt.

This is permitted.

Logical rollback is mandatory.

Restoring allocation addresses or exact private capacities is not.

### Multi-operation registration example

An adapter may:

1. Register physical observation P1.
2. Register a UUID claim for P1.
3. Register a PCI locator for P1.

If step 1 succeeds and step 2 fails:

- P1 remains committed.
- No UUID claim is committed.
- Step 3 is not automatically attempted by the builder.
- The adapter receives the failure result.

The adapter or future orchestrator determines whether to retry, recover or abandon the overall construction attempt.

The builder must not automatically erase P1.

### Fatal failures and overall construction

An out-of-memory result leaves the current builder logically valid but does not mean that discovery as a whole succeeded.

The future orchestrator must not publish an incompletely collected topology as a successful discovery result after an unrecovered fatal registration failure.

In the absence of an explicitly defined recovery policy, the default orchestration response to fatal registration failure is to abandon the construction attempt and destroy its builder.

Slice 2 does not implement orchestration, but its tests and documentation must preserve this separation.

---

## 11. Test-only allocation failure, private inspection and boundary tests

Introduce a private topology allocation-failure test interface.

Do not expose these controls from either public topology header.

### Allocation failure injection

Tests must be able to configure:

- Failure on the Nth subsequent topology-owned allocation attempt.
- N counted from 1.
- Exactly one injected failure per configuration.
- Automatic restoration of normal allocation after that failure.
- Explicit disable/reset before the next scenario.
- An observed allocation-attempt count.

This must cover initial builder allocation and subsequent builder table growth.

The test allocator wrapper must preserve normal allocation semantics when failure injection is disabled.

### Private inspection

Tests must be able to inspect, without modifying:

- All twelve committed record counts.
- All twelve capacities.
- Committed source kinds.
- Observation source IDs.
- Descriptor values.
- Identity payloads.
- Locator values.
- Relationship endpoints.

No mutable public array getter is introduced.

### Record-limit tests

Use private capacity-arithmetic helper tests or controlled test seams to exercise representational boundaries without allocating billions of records.

Do not manufacture an invalid live builder with inconsistent count/capacity state merely to reach an overflow branch.

### Isolation

All test-only state must be reset between scenarios.

The public release interface must contain no fault-injection operations.

---

## 12. Ordered implementation work packages and exit criteria

Implement in the following dependency order.

Each work package should leave the relevant subsystem compiling and testable.

### ## WP-01 — Baseline and contract reconciliation

- Confirm the current Slice 1 implementation and passing tests.
- Confirm actual repository paths.
- Review the approved registration-header boundary.
- Freeze descriptors and temporary ID representations.
- Resolve the old PCI/DRM identity-versus-locator declarations.
- Confirm the exact error taxonomy.
- Document the single-active-builder lifecycle amendment against Slice 1.

**Exit criterion:** No unresolved public representation or status mismatch blocks implementation.

### ## WP-02 — Builder lifecycle and active-builder guard

- Implement or update the single-active-builder enforcement.
- Preserve the Slice 1 creation and destruction contracts.
- Ensure thread-safe reservation behavior.
- Ensure creation failure releases its reservation.
- Ensure destruction releases the reservation only for the live builder being destroyed.

**Exit criterion:** All lifecycle and concurrent-creation tests pass.

### ## WP-03 — Typed storage infrastructure

- Define private typed record storage.
- Implement checked dynamic growth for twelve tables.
- Implement full cleanup.
- Add deterministic allocator-failure test support.
- Add private test inspection where required.

**Exit criterion:** Every table can grow, preserve committed contents, report failure and be destroyed safely.

### ## WP-04 — Discovery source registration

- Validate source descriptors.
- Enforce one source record per source kind.
- Assign one-based source IDs.
- Preserve copied source metadata.

**Exit criterion:** Source registration tests pass, including duplicate rejection and OOM rollback.

### ## WP-05 — Observation registration

Implement and test:

- Physical observations.
- Execution observations.
- Memory observations.
- Backend observations.

Preserve independent typed ID sequences.

Validate all enum, descriptor, source and backend-reference rules.

**Exit criterion:** All four observation categories can be registered and independently verified through the private test interface.

### ## WP-06 — Identity evidence

- Implement physical identity claims.
- Implement execution identity claims.
- Validate namespace-owner compatibility.
- Validate binary UUID representations.
- Validate documented textual NVIDIA forms.
- Preserve valid duplicate/conflicting evidence.

**Exit criterion:** All identity validation and transactionality tests pass.

### ## WP-07 — Physical locator evidence

- Implement typed PCI BDF values.
- Implement typed Linux DRM values.
- Implement separate physical and execution locator registration.
- Validate owner and source.
- Preserve duplicate valid locator evidence.

**Exit criterion:** PCI, DRM, ownership, duplication and failure tests pass.

### ## WP-08 — Structural relationship evidence

Implement:

- BACKED_BY.
- SUBTARGET_OF.
- Execution-memory association.

Enforce correct endpoint categories and same-source ownership.

Reject direct self-parenting.

Do not introduce premature global graph validation.

**Exit criterion:** All relationship tests pass, including multi-backing, nested hierarchy and shared memory cases.

### ## WP-09 — Failure and boundary verification

- Exercise every registration family under deterministic allocation failure.
- Exercise invalid enums, unknown references and foreign-source references.
- Exercise temporary ID exhaustion through controlled private test setup.
- Verify committed record preservation.
- Verify destruction after failures.
- Run sanitizer-enabled tests where supported.

**Exit criterion:** No observed partial logical commits, leaks, invalid frees or broken ID sequences.

### ## WP-10 — Final Slice 2 integration and documentation

- Verify that mixed-source registration works in one builder.
- Verify all twelve storage tables.
- Verify source provenance.
- Verify registration ordering.
- Verify duplicate evidence retention.
- Verify no canonical topology objects are created.
- Verify the approved public interface contracts.
- Update documentation and parent ticket slice numbering.

**Exit criterion:** The complete Slice 2 acceptance suite passes.

---

## 13. Mandatory end-to-end registration scenarios

These scenarios exercise the system beyond isolated function tests.

### ## Scenario A — Single-source CPU discovery

Register:

- One CPU-oriented discovery source.
- A physical CPU observation.
- A host execution observation.
- A SYSTEM memory observation.
- A scalar backend observation.
- Any valid source-local backing and execution-memory associations.

Verify the records, ownership and typed IDs.

No finalized topology is created.

### ## Scenario B — Independent Linux and CUDA observations

Register Linux and CUDA sources.

Linux reports a physical GPU observation.

CUDA reports a GPU execution observation.

Both may report the same PCI BDF through their own typed locator evidence.

Verify:

- Observations remain independent.
- Source attribution remains correct.
- The shared locator does not merge the records.
- A direct relationship between the Linux-owned physical observation and CUDA-owned execution observation is rejected during Slice 2.

### ## Scenario C — NVIDIA MIG

Within a compatible source-local discovery scenario, register:

- One physical GPU observation.
- Several MIG execution observations.
- Distinct valid MIG identities.
- Shared PCI locators where appropriate.
- BACKED_BY evidence to the physical GPU.
- Associated memory observations.

Verify the system does not merge MIG execution observations merely because they share a physical location.

### ## Scenario D — Aggregate execution target

Register two physical observations and one aggregate execution observation from the same source.

Register two BACKED_BY relationships connecting the aggregate to both physical observations.

Verify both backing records are preserved.

### ## Scenario E — Level Zero hierarchy

Register three execution observations representing a root, child and nested child.

Register their direct parent relationships.

Verify the hierarchy evidence is stored as submitted.

Then separately test a direct self-parent attempt and confirm rejection.

### ## Scenario F — Shared memory domain

Register multiple execution observations and one memory observation from the same source.

Associate both execution observations with the same memory observation.

Verify the many-to-one relationship is preserved.

### ## Scenario G — Duplicate and contradictory evidence

Register repeated valid identity, locator and relationship evidence.

Then register structurally valid identity claims that may be inconsistent.

Verify no premature deduplication, merging or reconciliation failure occurs.

### ## Scenario H — Partial evidence after failure

Register a physical observation successfully.

Force its identity registration to fail.

Verify the physical observation remains committed and the identity claim does not.

Disable failure injection and retry the identity registration.

Verify the retry can succeed without introducing an observation ID gap or corrupting the builder.

### ## Scenario I — Mid-discovery out-of-memory failure

Register several successful source and observation records.

Force a later evidence-storage growth failure.

Verify:

- Earlier records remain correct.
- Failed evidence is absent.
- Committed counts remain correct.
- The builder is destroyable.
- Cleanup releases all storage.

### ## Scenario J — Sequential construction attempts

Create, populate and destroy one builder.

Create another builder afterward.

Verify:

- Creation succeeds only after previous destruction.
- ID sequences begin from the new builder's empty state.
- Old temporary IDs are not treated as permanent runtime handles.
- No builder-owned memory or active reservation survives incorrectly.

---

## 14. Slice 2 acceptance checklist

Slice 2 is accepted only when **all** of the following are true.

#### Storage

- All twelve typed builder tables exist and are independently growable.
- Count/capacity invariants hold.
- Integer and allocation-size overflow are handled.
- All accepted metadata is builder-owned.
- No accepted caller-owned string or descriptor pointer is retained.
- Destruction releases all owned allocations.

#### Sources

- Source registration works.
- Source kind and source ID remain distinct.
- Duplicate source kind is rejected.
- Registered sources may have zero observations.
- Source IDs begin at one and consume no value on failure.

#### Observations

- Four observation categories register correctly.
- Categories have independent temporary-ID sequences.
- Observation provenance is retained.
- Physical and execution descriptions remain distinct.
- Memory capacity-known rules are enforced.
- Backend observations reference exactly one valid, same-source execution observation.
- No canonicalization occurs during registration.

#### Identity and locators

- Identity owner-kind rules are enforced.
- Binary and textual identifiers are validated.
- PCI and DRM locator values are typed and validated.
- Identity claims and physical locators remain separate.
- Duplicate and potentially conflicting valid evidence is retained.
- No functional memory-identity registration is introduced.

#### Relationships

- All three relationship categories register correctly.
- Relationship endpoints are validated.
- Same-source ownership is enforced.
- Multiple physical backings are representable.
- Nested execution hierarchies are representable.
- Shared execution-memory relationships are representable.
- Direct self-parenting is rejected.
- Full graph reconciliation/validation is not performed prematurely.

#### Transactions and failures

- Failed operations do not commit incomplete records.
- Previous committed records remain intact.
- Failed source or observation registration consumes no ID.
- Valid output-ID pointers are zeroed on failure.
- OOM and representational-limit failures remain distinguishable.
- Builder cleanup works after arbitrary tested failure points.
- Every owned table has deterministic allocation-failure coverage.

#### Lifecycle

- At most one live builder exists per process.
- A second creation fails while another builder is live.
- Concurrent creation cannot establish two live builders.
- Failed creation does not strand the active reservation.
- Destroying a builder permits a subsequent construction attempt.
- Registration remains single-writer.
- Temporary IDs are not treated as permanent runtime handles.

#### Integration

- All mandatory integration scenarios pass.
- Existing Slice 1 tests still pass.
- Sanitizer-enabled tests pass where supported.
- No source or observation records are silently merged.
- The public builder header matches the approved contract.
- No unrelated runtime subsystem is implemented.
- The parent KES-004 implementation plan reflects the new Slice 2 boundary.

---

## 15. Definition of Done

KES-004 Slice 2 is complete when a caller operating on the single active topology builder can:

1. Register multiple discovery sources.
2. Register physical, execution, memory and backend observations against those sources.
3. Register supported identity claims for physical and execution observations.
4. Register typed physical location evidence.
5. Register BACKED_BY, SUBTARGET_OF and execution-memory evidence.
6. Retrieve or inspect those records through approved private testing mechanisms.
7. Reliably reject malformed input and invalid local references.
8. Preserve all earlier committed records after failed registration.
9. Recover from allocation failures at the builder-storage level without corruption.
10. Destroy the builder with all owned memory released.

The resulting builder must retain sufficient structured information for later reconciliation and canonicalization.

It must not yet determine which CUDA, Vulkan, Linux, NVML or Level Zero observations refer to the same canonical resource.

It must not yet produce an immutable runtime topology.

**Successful completion means the evidence-preserving discovery construction layer is implemented and validated—not that KES-004 as a whole is finished.**

---

## 16. One approval gate before implementation

Review **this single consolidated ticket** and explicitly resolve the legacy MIG GI/CI leading-zero policy (§7) before treating this contract as frozen. In the accepted version:

- The public builder header's exact type declarations and operation signatures must match the names, parameter orders and field representations specified here.
- The single-active-builder contract must also be reflected in the Slice 1 lifecycle documentation and tests.
- The old overarching KES-004 ticket's separation of Slice 2 dynamic arrays / Slice 3 observations / Slice 4 evidence must be superseded and the subsequent slices renumbered consistently.
- Status numeric values, UUID encodings, PCI/DRM typed locators, failure-injection behavior and test expectations are implementation-contract decisions; a deviation requires explicit review.
- No production C code is supplied by this ticket. The implementation is written and reviewed separately by the Kestrel engineer.

**Status remains PROPOSED until the reviewer explicitly approves this consolidated document.**

---

### Provenance of consolidation

Merged source A: *Texyn Labs — Kestrel Engineering Implementation Ticket: KES-004 Slice 2* (25 sections, 1,600 lines).  
Merged source B: *Texyn Labs — Kestrel: KES-004 Slice 2 Implementation Contract Freeze* (17 sections, 917 lines).  
The shared architecture, operations, storage, tests and stages are consolidated into one reviewable document. Only operationally useful repetition is retained in per-operation tests and the final acceptance checklist.
