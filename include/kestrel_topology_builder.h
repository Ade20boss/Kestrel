#ifndef KESTREL_TOPOLOGY_BUILDER_H
#define KESTREL_TOPOLOGY_BUILDER_H

/*
 * KESTREL TOPOLOGY BUILDER -- DISCOVERY OBSERVATION INTERFACE
 *
 * PURPOSE
 * -------
 * Collect what individual discovery mechanisms report about a machine,
 * without prematurely deciding that two reports refer to the same resource.
 * Examples of reporting sources: Linux sysfs, CUDA, Vulkan and Level Zero.
 * This header declares the input types and registration API; it does not
 * perform hardware discovery or provide the builder's storage implementation.
 *
 * THE FOUR DIFFERENT KINDS OF INFORMATION
 * ---------------------------------------
 * 1. Observation: "A source reports a resource exists."
 *    Physical device P1, execution target E1, memory space M1 or backend B1.
 * 2. Identity: "Which resource is this, within a specific ID namespace?"
 *    Example: a source-reported GPU UUID; not every resource has one.
 * 3. Locator: "Where/how is the resource addressed or enumerated?"
 *    Example: PCI 0000:01:00.0 or Vulkan heap 0 of execution target E1.
 * 4. Relationship: "How are two observed resources associated?"
 *    Example: E1 is backed by P1, or E1 is associated with memory M1.
 *
 * These facts are stored separately because none implies the others.
 * In particular, registering a GPU does not automatically register its
 * memory; a GPU may expose several memory domains or share system memory.
 *
 * WHY SOURCE-SPECIFIC OBSERVATIONS?
 * ---------------------------------
 * CUDA and Vulkan can report the same physical GPU through different API
 * objects; other sources might also report it. Equal names, locators or
 * superficially similar IDs do NOT authorize the builder to merge records.
 * Identity kinds have distinct namespaces; a locator is not automatically
 * a unique or permanent physical identity. Conflicting/partial evidence is
 * preserved for a later reconciliation stage, which may conclude "unknown".
 *
 * Builder IDs are temporary, builder-local, one-based record references.
 * They are NOT the canonical resource IDs from kestrel_topology.h. The
 * canonical topology is a later product of reconciliation, not this header.
 *
 * EXAMPLE OF A SOURCE-LOCAL SET OF CLAIMS
 * ----------------------------------------
 * Source S1 (Vulkan) reports execution target E1 and memory heap M1.
 * Register E1, register M1, associate E1 with M1, then optionally attach
 * a locator "Vulkan heap index 0 as enumerated by E1" to M1.
 * E1/M1 and their relationship must belong to the same reporting source.
 * Heap index 0 by itself does not uniquely identify a memory resource.
 */

#include "kestrel_core.h"
#include "kestrel_topology.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* ================================================================
 * 1. DISCOVERY AND LOCATOR TYPES
 * ================================================================ */

/*
 * Linux Direct Rendering Manager (DRM) device-node category.
 * PRIMARY usually corresponds to /dev/dri/cardN; RENDER to
 * /dev/dri/renderD<N>. These are different node types, potentially for
 * the same underlying GPU. The numeric node address is kept separately.
 * INVALID (0) is not a usable node type.
 */

typedef enum
{
    KESTREL_DRM_NODE_KIND_INVALID = 0,
    KESTREL_DRM_NODE_KIND_PRIMARY,
    KESTREL_DRM_NODE_KIND_RENDER
} kestrel_drm_node_kind_t;


/*
 * Kind of discovery mechanism, NOT a device kind or device identity.
 *
 * HOST_CPU: host CPU discovery; LINUX_SYSFS: Linux kernel sysfs view;
 * X86_CPUID: x86 CPU instruction-based information;
 * CUDA_DRIVER: CUDA Driver API; VULKAN: Vulkan device enumeration;
 * NVIDIA_NVML: NVIDIA management API;
 * LEVEL_ZERO: Level Zero device/subdevice discovery.
 *
 * A builder registers at most one source record of each kind (Slice 2
 * design), but a source may report many physical/execution/memory records.
 * Two different source kinds may describe the very same hardware. Do NOT
 * collapse their records here; cross-source reconciliation comes later.
 */
typedef enum
{
    KESTREL_DISCOVERY_SOURCE_INVALID = 0,
    KESTREL_DISCOVERY_SOURCE_HOST_CPU,
    KESTREL_DISCOVERY_SOURCE_LINUX_SYSFS,
    KESTREL_DISCOVERY_SOURCE_X86_CPUID,
    KESTREL_DISCOVERY_SOURCE_CUDA_DRIVER,
    KESTREL_DISCOVERY_SOURCE_VULKAN,
    KESTREL_DISCOVERY_SOURCE_NVIDIA_NVML,
    KESTREL_DISCOVERY_SOURCE_LEVEL_ZERO
} kestrel_discovery_source_kind_t;


/*
 * Which location/address scheme was used for a PHYSICAL or EXECUTION
 * observation? PCI_BDF is a PCI domain:bus:device.function address;
 * LINUX_DRM_DEVICE is a Linux DRM primary/render node and device numbers.
 *
 * Unlike a source-issued UUID identity, a locator primarily says HOW to
 * find/reference a resource within a particular system or API context.
 * The same locator may occur in several observations and is not, by
 * itself, permission to merge those observations.
 */

typedef enum
{
    KESTREL_LOCATOR_KIND_INVALID = 0,
    KESTREL_LOCATOR_KIND_PCI_BDF,
    KESTREL_LOCATOR_KIND_LINUX_DRM_DEVICE,
} kestrel_locator_kind_t;


/*
 * Memory uses DIFFERENT locator namespaces from PCI/DRM device locators.
 *
 * LINUX_NUMA_NODE: Linux OS NUMA node number (e.g., node2 -> 2).
 * LINUX_CXL_REGION: Linux CXL sysfs region number (region0 -> 0).
 *   For both Linux forms the reporting Linux context supplies the scope;
 *   no execution-target ID belongs in the locator value.
 * VULKAN_HEAP: memory-heap INDEX in a particular Vulkan device's list.
 * LEVEL_ZERO_LOCAL: local-memory ORDINAL in a particular Level Zero
 *   device's memory-properties enumeration.
 *   Both API-local forms require an explicit execution-observation scope.
 *
 * Important: a NUMA node number, region number, heap index or ordinal
 * is a locator, not a UUID/identity for the memory's physical silicon.
 * A NUMBER of zero can be valid; only locator KIND zero is invalid.
 */
typedef enum
{
    KESTREL_MEMORY_LOCATOR_KIND_INVALID = 0,
    KESTREL_MEMORY_LOCATOR_KIND_LINUX_NUMA_NODE,
    KESTREL_MEMORY_LOCATOR_KIND_LINUX_CXL_REGION,
    KESTREL_MEMORY_LOCATOR_KIND_VULKAN_HEAP,
    KESTREL_MEMORY_LOCATOR_KIND_LEVEL_ZERO_LOCAL,
} kestrel_memory_locator_kind_t;


/* ================================================================
 * 2. TEMPORARY BUILDER-LOCAL IDENTIFIERS
 * ================================================================ */

/*
 * Each ID type refers to a different CATEGORY of builder record:
 * source S, physical observation P, execution observation E, memory
 * observation M or backend observation B.
 *
 * Why structs containing uint32_t instead of typedef uint32_t ... ?
 * Bare integer aliases can be mixed accidentally. Distinct struct types
 * make passing P1 where E1 is required a type error diagnosed by C.
 *
 * Rules:
 * - value == 0 is invalid; the first successful record of each category
 *   receives value 1 (IDs are one-based, not C array indices).
 * - Each category has its OWN numbering: P1 and M1 are unrelated.
 * - IDs are only valid within their creating builder's lifetime.
 * - A relationship or evidence record references already registered IDs.
 * - These are not persistent device UUIDs or canonical topology IDs.
 * - A failed registration must not consume an ID or leave a partial record
 *   (intended atomic registration contract, to implement/test in .c).
 *
 * The internal representation may use arrays; these public ID types do
 * not imply that the private builder uses an arena allocator.
 */

/* Identifies a registered discovery source. */
typedef struct
{
    uint32_t value;
} kestrel_discovery_source_id_t;


/* Identifies a physical-device observation reported by a source. */
typedef struct
{
    uint32_t value;
} kestrel_physical_observation_id_t;


/* Identifies an execution-target observation reported by a source. */
typedef struct
{
    uint32_t value;
} kestrel_execution_observation_id_t;


/* Identifies a memory-space observation reported by a source. */
typedef struct
{
    uint32_t value;
} kestrel_memory_observation_id_t;


/* Identifies a backend-instance observation reported by a source. */
typedef struct
{
    uint32_t value;
} kestrel_backend_observation_id_t;

/*
 * An API-local memory entry is numbered relative to the execution device
 * that was queried, NOT relative to all memory observations in Kestrel.
 *
 * scope_execution_id: which previously registered execution observation
 *                     defines this API's numbering namespace.
 * index:              Vulkan heap index OR Level Zero memory ordinal,
 *                     depending on the enclosing memory-locator kind.
 *
 * Example: Vulkan E1 heap 0 and Vulkan E2 heap 0 are different locator
 * claims: (VULKAN_HEAP, E1, 0) versus (VULKAN_HEAP, E2, 0).
 *
 * The scope is NOT the memory observation that owns the evidence: that
 * owner is passed separately to register_memory_locator(). Nor does scope
 * imply sole ownership/access. A memory observation can be associated
 * with multiple execution observations, so this extra context matters.
 * This struct is ONLY used for Vulkan and Level Zero locator kinds.
 */
typedef struct
{
    kestrel_execution_observation_id_t scope_execution_id;
    uint32_t index;
} kestrel_device_scoped_memory_locator_t;


/* ================================================================
 * 3. DEVICE LOCATOR REPRESENTATIONS
 * ================================================================ */

/*
 * Numeric PCI bus/device/function address, e.g. 0000:01:00.0:
 *   domain = 0, bus = 1, device = 0, function = 0.
 * The fields identify a PCI topology ADDRESS, not an immutable device.
 * PCI positions can change across system configurations/reboots; a
 * source may report the same PCI address for both a physical device and
 * one of its API execution views. Reconciliation must consider context.
 */
typedef struct
{
    uint32_t domain;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
} kestrel_pci_bdf_locator_t;


/*
 * Linux DRM node locator. node_kind distinguishes primary (/dev/dri/cardN)
 * from render (/dev/dri/renderD<N>) nodes. major/minor are the Linux
 * device numbers; they are NOT a pathname, GPU UUID or PCI address.
 * One GPU may have both a primary and a render node.
 */
typedef struct
{
    kestrel_drm_node_kind_t node_kind;
    uint32_t major;
    uint32_t minor;
} kestrel_drm_locator_t;


/*
 * TAGGED UNION for PCI/DRM device locators.
 *
 * A union reuses one block of storage for alternative shapes. The enum
 * kind is the tag that tells the caller which union member is meaningful:
 *   PCI_BDF          -> address.pci_bdf_locator
 *   LINUX_DRM_DEVICE -> address.drm_locator
 *
 * Always check kind before reading the matching member. An unrelated
 * union member is NOT another valid interpretation of the same evidence.
 * Do not compare raw struct/union bytes: padding and inactive storage
 * are not part of the semantic locator value.
 */
typedef struct
{
    kestrel_locator_kind_t kind;

    union
    {
        kestrel_drm_locator_t drm_locator;
        kestrel_pci_bdf_locator_t pci_bdf_locator;
    } address;
} kestrel_locator_value_t;


/*
 * TAGGED UNION for the FOUR approved memory locator namespaces.
 * Select exactly one meaningful union member using kind:
 *   LINUX_NUMA_NODE    -> address.linux_numa_node_id
 *   LINUX_CXL_REGION   -> address.linux_cxl_region_id
 *   VULKAN_HEAP        -> address.device_scoped
 *   LEVEL_ZERO_LOCAL   -> address.device_scoped
 *
 * Linux node/region numbers are system-scoped within the reporting Linux
 * discovery context. They do not need an execution scope. The union still
 * occupies enough bytes for its largest alternative; that's normal C
 * storage layout, not a claim that NUMA/CXL carry a device scope.
 *
 * For the device_scoped alternative, index == 0 is allowed: "heap 0"
 * is not invalid. Validation must require a real, same-source execution
 * observation for scope_execution_id, and a matching API source kind.
 * This value locates an API enumeration entry, NOT a byte address in RAM.
 */
typedef struct
{
    kestrel_memory_locator_kind_t kind;

    union
    {
        uint32_t linux_numa_node_id;
        uint32_t linux_cxl_region_id;
        kestrel_device_scoped_memory_locator_t device_scoped;
    } address;

} kestrel_memory_locator_value_t;


/* ================================================================
 * 4. DISCOVERY OBSERVATION DESCRIPTORS
 * ================================================================ */

/*
 * OBSERVATION DESCRIPTORS ARE INPUT DATA, NOT OWNED OBJECTS.
 *
 * Callers may construct a descriptor on the stack, heap or elsewhere.
 * The intended builder implementation validates it and stores an owned
 * copy of its values on success; it must not retain a pointer to the
 * caller's descriptor. Every descriptor below currently contains only
 * scalar values, fixed-size arrays or builder-local IDs (no borrowed
 * string pointers), so this is a value-copy requirement.
 *
 * The new observation's ID is returned separately in output_id. It is
 * not embedded in the descriptor because the builder assigns it.
 * A descriptor describes what ONE reporting source claims, not a fact
 * that Kestrel has already verified against other APIs or hardware.
 */

/*
 * Registers which discovery mechanism supplied subsequent observations.
 * For example, one source record of kind VULKAN can report several GPU
 * execution observations and several memory observations. A source ID is
 * provenance (who made the claim), NOT a PCI/device identifier.
 */
typedef struct
{
    kestrel_discovery_source_kind_t kind;
} kestrel_source_descriptor_t;


/*
 * PHYSICAL observation: what the source reports as underlying compute
 * hardware (CPU, GPU, accelerator or unknown). This is not an API handle.
 *
 * compute_class: broad compute category; ACCELERATOR can represent an NPU
 *                or TPU without a new resource category.
 * name/vendor:  source-reported fixed-size text; neither is unique identity.
 * gpu_integration: INTEGRATED, NON_INTEGRATED, or UNKNOWN for GPUs;
 *                  must be NOT_APPLICABLE for non-GPU observations.
 *
 * Integration describes how a GPU is integrated, NOT whether the GPU has
 * separately addressable/dedicated VRAM. No identity/locator is required;
 * source-specific evidence is registered via separate functions below.
 */
typedef struct
{
    kestrel_compute_class_t compute_class;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
    char vendor[KESTREL_MAX_VENDOR_NAME];
    kestrel_gpu_integration_t gpu_integration;
} kestrel_physical_observation_descriptor_t;


/*
 * EXECUTION observation: a compute resource as exposed through some API
 * or execution interface (e.g., CUDA device, Vulkan physical-device view,
 * Level Zero root device/subdevice). This is NOT necessarily a separate
 * physical chip. Two different APIs may each observe the same GPU.
 *
 * compute_class/gpu_integration have the same interpretation as above;
 * name is a descriptive label, not reliable matching evidence.
 * The physical backing, subtarget hierarchy, memory associations and
 * identity/locator evidence are all registered separately when known.
 */
typedef struct
{
    kestrel_compute_class_t compute_class;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
    kestrel_gpu_integration_t gpu_integration;
} kestrel_execution_observation_descriptor_t;


/*
 * MEMORY observation: a source-reported memory domain, independent of
 * which CPU/GPU/accelerator might use it. One execution target may have
 * several memory observations; several targets may refer to one memory
 * observation. An integrated GPU may share underlying system RAM.
 *
 * kind: SYSTEM, DEVICE_LOCAL or UNKNOWN as reported by the source.
 *       DEVICE_LOCAL does NOT necessarily mean exclusive/dedicated VRAM.
 * name: source-supplied description, NOT a unique identifier.
 * capacity_known: whether the source actually knows the capacity.
 * capacity_bytes: meaningful as a reported capacity only if known == true.
 *
 * This is NOT a free-memory figure, permission grant, allocation API,
 * bandwidth number or physical-RAM de-duplication claim. Capacities of
 * different observations may overlap and must not be blindly summed.
 * Memory UUIDs/locators are optional additional evidence, not mandatory.
 */
typedef struct
{
    kestrel_memory_kind_t kind;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
    bool capacity_known;
    uint64_t capacity_bytes;
} kestrel_memory_observation_descriptor_t;


/*
 * BACKEND observation: a source-reported execution-interface instance.
 * family is an enum from kestrel_topology.h (SCALAR, AVX2, VULKAN,
 * CUDA, LEVEL_ZERO); execution_observation_id names the existing
 * source-local execution observation to which this backend is attached.
 *
 * Physical chip != API execution target != backend instance.
 * This record does not itself initialize a driver, prove a backend can
 * dispatch a kernel, or establish the backend's runtime capabilities.
 */
typedef struct
{
    kestrel_backend_family_t family;
    kestrel_execution_observation_id_t execution_observation_id;
} kestrel_backend_observation_descriptor_t;


/* ================================================================
 * 5. LOCATOR EVIDENCE DESCRIPTORS
 * ================================================================ */

/*
 * PCI/DRM locator evidence attached to one PHYSICAL observation.
 * The owner physical-observation ID is supplied to registration separately;
 * this descriptor contains only the tagged locator value.
 */
typedef struct
{
    kestrel_locator_value_t value;
} kestrel_physical_locator_descriptor_t;


/*
 * PCI/DRM locator evidence attached to one EXECUTION observation.
 * The same PCI address may also appear for its underlying physical GPU;
 * equal locators across records do not merge those records here.
 */
typedef struct
{
    kestrel_locator_value_t value;
} kestrel_execution_locator_descriptor_t;

/*
 * NUMA/CXL/Vulkan/Level Zero locator evidence attached to one MEMORY
 * observation. Must contain the MEMORY locator union, not the separate
 * PCI/DRM kestrel_locator_value_t used for physical/execution resources.
 */
typedef struct
{
    kestrel_memory_locator_value_t value;
} kestrel_memory_locator_descriptor_t;


/* ================================================================
 * REGISTRATION API: WHAT EACH PARAMETER MEANS
 * ================================================================
 *
 * builder:              Mutable, opaque builder object. Its lifecycle is
 *                       declared in kestrel_topology.h. Slice 2 intends
 *                       at most ONE live builder per process; this header
 *                       alone cannot enforce that rule.
 * reporting_source_id:  Existing source ID, passed BY VALUE. It records
 *                       provenance and is used to validate same-source
 *                       ownership; it is not an access-control token.
 * const *descriptor:    Borrowed, read-only input for this call. const
 *                       protects against mutation through this pointer;
 *                       it is NOT a guarantee of thread safety.
 * *output_id:           Caller-owned output pointer for a NEW source or
 *                       observation record, assigned only on success.
 * observation_owner:    Existing ID of the resource RECEIVING evidence.
 * relationship IDs:     Existing endpoints, supplied by value.
 *
 * Every registration must validate builder state, required arguments,
 * source ownership and kind-specific rules BEFORE it commits a record.
 * Once a record succeeds, it belongs to the builder; failure must not
 * leave a partial record or consume an ID. Private storage and actual
 * validation are implemented in .c, not guaranteed by these prototypes.
 *
 * Source-local rule: the owner and both ends of a relationship belong
 * to reporting_source_id. A scoped Vulkan/Level Zero memory locator's
 * execution-observation scope also belongs to this SAME source. There
 * is no cross-source linking/matching at registration time.
 */

/* ================================================================
 * 6. DISCOVERY SOURCE REGISTRATION
 * ================================================================ */

/*
 * Register a discovery source FIRST; obtain a one-based source_id.
 * The Slice 2 contract allows one source of each kind per builder.
 * Duplicate source kind, INVALID kind or invalid builder must be rejected.
 */
kestrel_status_t kestrel_topology_builder_register_source(kestrel_topology_builder_t *builder,
                                                          const kestrel_source_descriptor_t *source_descriptor,
                                                          kestrel_discovery_source_id_t *source_id);


/* ================================================================
 * 7. RESOURCE OBSERVATION REGISTRATION
 * ================================================================ */

/*
 * Create P<n>, a PHYSICAL hardware observation from an existing source.
 * The descriptor is copied; output_id receives the new P ID on success.
 * A GPU discovered via another source is ANOTHER observation for now.
 */
kestrel_status_t kestrel_topology_builder_register_physical_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_physical_observation_descriptor_t *physical_descriptor,
    kestrel_physical_observation_id_t *output_id);


/*
 * Create E<n>, an API-visible EXECUTION observation from this source.
 * An E ID is not a physical ID. Later register_backing(E, P) if this
 * source actually establishes which physical observation backs E.
 */
kestrel_status_t kestrel_topology_builder_register_execution_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_execution_observation_descriptor_t *execution_descriptor,
    kestrel_execution_observation_id_t *output_id);


/*
 * Create M<n>, an independently reported MEMORY domain/pool/heap.
 * This is not automatically produced by registering a GPU. Register
 * execution_memory(E, M) separately if the source reports association;
 * optional memory identity and locator evidence can be attached later.
 */
kestrel_status_t kestrel_topology_builder_register_memory_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_memory_observation_descriptor_t *memory_descriptor,
    kestrel_memory_observation_id_t *output_id);


/*
 * Create B<n>, a BACKEND observation associated with an existing E ID.
 * The E ID in backend_descriptor must belong to the reporting source.
 * This only stores the source's claim, not a ready-to-run backend.
 */
kestrel_status_t kestrel_topology_builder_register_backend_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_backend_observation_descriptor_t *backend_descriptor,
    kestrel_backend_observation_id_t *output_id);


/* ================================================================
 * 8. IDENTITY EVIDENCE REGISTRATION
 * ================================================================ */

/*
 * Add an IDENTITY claim for physical observation P<n>, e.g. a source-
 * reported NVIDIA GPU UUID. Identity bytes/kind are defined in
 * kestrel_topology.h. An identity has a namespace: do NOT treat UUIDs
 * with different kinds as automatically interchangeable.
 * This adds evidence, not a new P observation or a canonical merge.
 */
kestrel_status_t kestrel_topology_builder_register_physical_identity(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_physical_observation_id_t observation_owner,
    const kestrel_identity_value_t *identity);


/*
 * Add an IDENTITY claim for execution observation E<n>, e.g. CUDA device
 * UUID, Vulkan device UUID, NVIDIA MIG UUID or Level Zero device UUID.
 * Each kind has source-specific meaning and validation rules. A CUDA
 * UUID does not directly identify a separate piece of memory.
 * Keep claims separate even when multiple observations look similar.
 */
kestrel_status_t kestrel_topology_builder_register_execution_identity(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_execution_observation_id_t observation_owner,
    const kestrel_identity_value_t *identity);


/* ================================================================
 * 9. TOPOLOGY RELATIONSHIP REGISTRATION
 * ================================================================ */

/*
 * RELATIONSHIPS ARE SOURCE-REPORTED GRAPH EDGES, NOT IDENTITIES.
 * Every edge connects already registered, same-source observations.
 *
 * Do not assume a general DAG/acyclicity guarantee: this interface does
 * not by itself check for cycles, and different relationship categories
 * express different meanings. Hierarchy-specific validation belongs in
 * the registration implementation. Graph edges are observations, too.
 */

/*
 * BACKED_BY(E, P): the source reports that execution observation E is
 * backed by physical observation P. Useful when the API exposes a GPU
 * execution handle and the source also knows the underlying device.
 * This is NOT the same thing as identifying E and P as one resource:
 * they have different roles and distinct ID namespaces.
 */
kestrel_status_t kestrel_topology_builder_register_backing(kestrel_topology_builder_t *builder,
                                                           kestrel_discovery_source_id_t reporting_source_id,
                                                           kestrel_execution_observation_id_t observation_owner,
                                                           kestrel_physical_observation_id_t physical_backer_id);


/*
 * SUBTARGET_OF(child E, parent E): a source-reported execution hierarchy.
 * Example: a Level Zero subdevice is subordinate to a root device.
 * Child and parent are both execution observations from the SAME source.
 * This is a hierarchy claim, not proof of independent physical silicon;
 * cycles/self-parenting require appropriate implementation validation.
 */
kestrel_status_t kestrel_topology_builder_register_subtarget(kestrel_topology_builder_t *builder,
                                                             kestrel_discovery_source_id_t reporting_source_id,
                                                             kestrel_execution_observation_id_t child_execution_id,
                                                             kestrel_execution_observation_id_t parent_execution_id);


/*
 * EXECUTION_MEMORY(E, M): this source ASSOCIATES execution observation
 * E with memory observation M. It does NOT imply that E owns M, has
 * permission to allocate/access every byte, or has a particular bandwidth,
 * NUMA affinity, free capacity or exclusive memory pool.
 *
 * Multiple E observations may associate with one M; one E may associate
 * with multiple M observations. That is WHY memory has its own records.
 * These associations also do not say which API device's numbering system
 * produced a Vulkan heap/Level Zero memory index; a scoped memory locator
 * records that context separately.
 */
kestrel_status_t kestrel_topology_builder_register_execution_memory(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_execution_observation_id_t execution_id,
    kestrel_memory_observation_id_t memory_observation_id);


/* ================================================================
 * 10. PHYSICAL / EXECUTION DEVICE LOCATOR EVIDENCE
 * ================================================================ */

/*
 * Attach a PCI BDF or DRM node locator to physical observation P<n>.
 * The locator identifies a source-reported address/reference, NOT a
 * globally unique physical-resource identity. No deduplication occurs.
 */
kestrel_status_t kestrel_topology_builder_register_physical_locator(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_physical_observation_id_t physical_observation_id,
    const kestrel_physical_locator_descriptor_t *physical_locator_descriptor);


/*
 * Attach a PCI BDF or DRM node locator to execution observation E<n>.
 * P and E may report an equal PCI address yet remain distinct records.
 */
kestrel_status_t kestrel_topology_builder_register_execution_locator(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_execution_observation_id_t execution_observation_id,
    const kestrel_execution_locator_descriptor_t *execution_locator_descriptor);


/* ================================================================
 * 11. MEMORY IDENTITY EVIDENCE
 * ================================================================ */
/*
 * Attach a source-reported IDENTITY to memory observation M<n>.
 * Approved memory ID kinds are NVDIMM namespace UUID and persistent CXL
 * region UUID (defined in kestrel_topology.h, when that enum is updated).
 * These name specific persistent-memory objects, not general system RAM,
 * arbitrary GPU VRAM, or all CXL memory.
 *
 * The builder validates the kind, representation and owner/source; the
 * discovery adapter is responsible for claiming the correct real resource.
 * No memory UUID is required to register an M observation.
 */
kestrel_status_t kestrel_topology_builder_register_memory_identity(kestrel_topology_builder_t *builder,
                                                                   kestrel_discovery_source_id_t reporting_source_id,
                                                                   kestrel_memory_observation_id_t observation_owner,
                                                                   const kestrel_identity_value_t *identity);


/* ================================================================
 * 12. MEMORY LOCATOR EVIDENCE
 * ================================================================ */
/*
 * Attach a NUMA/CXL/Vulkan/Level Zero LOCATOR to memory observation M<n>.
 * The memory observation is the owner, passed as observation_owner.
 * For Vulkan/Level Zero, locator_descriptor->value.address.device_scoped
 * additionally provides the execution E whose API enumeration defines
 * the index. E must already exist and belong to reporting_source_id.
 *
 * Example: owner M1, kind VULKAN_HEAP, scope E1, index 0 means
 * "M1 was reported as heap 0 when Vulkan execution target E1 was queried".
 * It does NOT automatically assert exclusive ownership/access or identity.
 * Linux NUMA/CXL locator values do not carry an execution scope.
 */
kestrel_status_t kestrel_topology_builder_register_memory_locator(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_memory_observation_id_t observation_owner,
    const kestrel_memory_locator_descriptor_t *locator_descriptor);

#endif /* KESTREL_TOPOLOGY_BUILDER_H */
