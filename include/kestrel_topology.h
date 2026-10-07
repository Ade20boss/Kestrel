#ifndef KESTREL_TOPOLOGY_H
#define KESTREL_TOPOLOGY_H

#include "kestrel_core.h"
#include <stdbool.h>
#include <stdint.h>


#define KESTREL_MAX_TOPOLOGY_NAME 128
#define KESTREL_MAX_VENDOR_NAME 64
#define KESTREL_MAX_IDENTITY_BYTES 64

typedef struct kestrel_topology_builder kestrel_topology_builder_t;
typedef struct kestrel_runtime_topology kestrel_runtime_topology_t;

typedef struct
{
    uint32_t value;
} kestrel_physical_device_id_t;

typedef struct
{
    uint32_t value;
} kestrel_execution_target_id_t;

typedef struct
{
    uint32_t value;
} kestrel_backend_instance_id_t;

typedef struct
{
    uint32_t value;
} kestrel_memory_space_id_t;


typedef enum
{
    KESTREL_COMPUTE_CLASS_INVALID = 0,
    KESTREL_COMPUTE_CLASS_CPU,
    KESTREL_COMPUTE_CLASS_GPU,
    KESTREL_COMPUTE_CLASS_ACCELERATOR,
    KESTREL_COMPUTE_CLASS_UNKNOWN
} kestrel_compute_class_t;

typedef enum
{
    KESTREL_BACKEND_FAMILY_INVALID = 0,
    KESTREL_BACKEND_FAMILY_SCALAR,
    KESTREL_BACKEND_FAMILY_AVX2,
    KESTREL_BACKEND_FAMILY_VULKAN,
    KESTREL_BACKEND_FAMILY_CUDA
} kestrel_backend_family_t;

typedef enum
{
    KESTREL_MEMORY_KIND_INVALID = 0,
    KESTREL_MEMORY_KIND_SYSTEM,
    KESTREL_MEMORY_KIND_DEVICE_LOCAL,
    KESTREL_MEMORY_KIND_UNKNOWN
} kestrel_memory_kind_t;

typedef enum
{
    KESTREL_IDENTITY_KIND_INVALID = 0,
    KESTREL_IDENTITY_KIND_PCI_BDF,
    KESTREL_IDENTITY_KIND_LINUX_DRM_DEVICE,
    KESTREL_IDENTITY_KIND_NVIDIA_GPU_UUID,
    KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID,
    KESTREL_IDENTITY_KIND_VULKAN_DEVICE_UUID,
    KESTREL_IDENTITY_KIND_NVIDIA_MIG_UUID,
    KESTREL_IDENTITY_KIND_LEVEL_ZERO_DEVICE_UUID,
} kestrel_identity_kind_t;


typedef struct
{
    kestrel_physical_device_id_t id;
    kestrel_compute_class_t compute_class;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
    char vendor[KESTREL_MAX_VENDOR_NAME];
} kestrel_physical_device_t;


typedef struct
{
    kestrel_execution_target_id_t id;
    kestrel_compute_class_t compute_class;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
} kestrel_execution_target_t;


typedef struct
{
    kestrel_backend_instance_id_t id;
    kestrel_backend_family_t family;
    kestrel_execution_target_id_t execution_target_id;
} kestrel_backend_instance_t;

typedef struct
{
    kestrel_memory_space_id_t id;
    kestrel_memory_kind_t kind;
    char name[KESTREL_MAX_TOPOLOGY_NAME];
    bool capacity_known;
    uint64_t capacity_bytes;
} kestrel_memory_space_t;


typedef struct
{
    kestrel_execution_target_id_t execution_target_id;
    kestrel_physical_device_id_t physical_device_id;
} kestrel_physical_execution_relation_t;

typedef struct
{
    kestrel_execution_target_id_t child_target_id;
    kestrel_execution_target_id_t parent_target_id;
} kestrel_execution_hierarchy_relation_t;


typedef struct
{
    kestrel_execution_target_id_t execution_target_id;
    kestrel_memory_space_id_t memory_space_id;
} kestrel_execution_memory_relation_t;


typedef struct
{
    kestrel_identity_kind_t identity_kind;
    uint8_t length;
    uint8_t bytes[KESTREL_MAX_IDENTITY_BYTES];
} kestrel_identity_value_t;


typedef struct
{
    kestrel_physical_device_id_t physical_device_id;
    kestrel_identity_value_t identity;
} kestrel_physical_identity_metadata_t;

typedef struct
{
    kestrel_execution_target_id_t execution_target_id;
    kestrel_identity_value_t identity;
} kestrel_execution_identity_metadata_t;

typedef struct
{
    kestrel_memory_space_id_t memory_space_id;
    kestrel_identity_value_t identity;
} kestrel_memory_identity_metadata_t;

kestrel_status_t kestrel_topology_builder_create(kestrel_topology_builder_t **builder);


void kestrel_topology_builder_destroy(kestrel_topology_builder_t **builder);

void kestrel_runtime_topology_destroy(kestrel_runtime_topology_t **topology);


#endif /* KESTREL_TOPOLOGY_H */
