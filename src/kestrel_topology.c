#include "kestrel_topology.h"
#include "kestrel_core.h"
#include "kestrel_topology_builder.h"
#include <ctype.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static atomic_bool is_builder_active = false;

#ifdef KESTREL_TEST_FAIL_FIRST_BUILDER_ALLOC
static bool fail_next_allocation = true;
#endif

#ifdef KESTREL_TEST_FAIL_FIRST_BUILDER_REALLOC
static bool fail_next_realloc = true;
#endif


typedef struct
{
    kestrel_physical_observation_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_physical_obs_obj;

typedef struct
{
    kestrel_execution_observation_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_execution_obs_obj;


typedef struct
{
    kestrel_memory_observation_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_memory_obs_obj;

typedef struct
{
    kestrel_backend_observation_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_backend_obs_obj;


typedef struct
{
    kestrel_physical_observation_id_t observation_id;
    kestrel_discovery_source_id_t reporting_source;
    kestrel_identity_value_t identity;
} kestrel_physical_identity_obs_obj;


typedef struct
{
    kestrel_execution_observation_id_t observation_id;
    kestrel_discovery_source_id_t reporting_source;
    kestrel_identity_value_t identity;
} kestrel_execution_identity_obs_obj;

typedef struct
{
    kestrel_memory_observation_id_t observation_id;
    kestrel_discovery_source_id_t reporting_source;
    kestrel_identity_value_t identity;
} kestrel_memory_identity_obs_obj;


typedef struct
{
    kestrel_execution_observation_id_t execution_id;
    kestrel_physical_observation_id_t physical_id;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_backing_obs_obj;


typedef struct
{
    kestrel_execution_observation_id_t child_id;
    kestrel_execution_observation_id_t parent_id;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_subtarget_obs_obj;


typedef struct
{
    kestrel_execution_observation_id_t execution_id;
    kestrel_memory_observation_id_t memory_id;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_execution_mem_obs_obj;


typedef struct
{
    kestrel_physical_observation_id_t physical_id;
    kestrel_physical_locator_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_physical_locator_obs_obj;


typedef struct
{
    kestrel_execution_observation_id_t execution_id;
    kestrel_execution_locator_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_execution_locator_obs_obj;


typedef struct
{
    kestrel_memory_observation_id_t memory_id;
    kestrel_memory_locator_descriptor_t description;
    kestrel_discovery_source_id_t reporting_source;
} kestrel_memory_locator_obs_obj;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_source_descriptor_t *data;
} kestrel_sources_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_physical_obs_obj *data;
} kestrel_physical_observation_storage;

typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_execution_obs_obj *data;
} kestrel_execution_observation_storage;

typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_memory_obs_obj *data;
} kestrel_memory_observation_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_backend_obs_obj *data;
} kestrel_backend_observation_storage;

typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_physical_identity_obs_obj *data;
} kestrel_physical_identity_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_execution_identity_obs_obj *data;
} kestrel_execution_identity_storage;

typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_backing_obs_obj *data;
} kestrel_backing_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_subtarget_obs_obj *data;
} kestrel_subtarget_storage;

typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_execution_mem_obs_obj *data;
} kestrel_execution_memory_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_memory_identity_obs_obj *data;
} kestrel_memory_identity_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_physical_locator_obs_obj *data;
} kestrel_physical_locator_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_execution_locator_obs_obj *data;
} kestrel_execution_locator_storage;


typedef struct
{
    size_t count;
    size_t capacity;
    kestrel_memory_locator_obs_obj *data;
} kestrel_memory_locator_storage;


struct kestrel_topology_builder
{
    kestrel_sources_storage sources_store;
    kestrel_physical_observation_storage physical_observation_store;
    kestrel_execution_observation_storage execution_observation_store;
    kestrel_memory_observation_storage memory_observation_store;
    kestrel_backend_observation_storage backend_observation_store;

    kestrel_physical_identity_storage physical_identity_store;
    kestrel_execution_identity_storage execution_identity_store;
    kestrel_memory_identity_storage memory_identity_store;

    kestrel_backing_storage backing_relationship_store;
    kestrel_subtarget_storage subtarget_relationship_store;
    kestrel_execution_memory_storage execution_memory_store;

    kestrel_physical_locator_storage physical_locator_store;
    kestrel_execution_locator_storage execution_locator_store;
    kestrel_memory_locator_storage memory_locator_store;
};


struct kestrel_runtime_topology
{
    int reserved;
};


typedef struct
{
    kestrel_status_t status;
    void *data;
    size_t capacity;
} growth_output;


static growth_output kestrel_growth_helper(void *data, size_t current_cap, size_t required_count, size_t element_size)
{
    growth_output growth_result;
    growth_result.status = KESTREL_STATUS_INVALID_STATE;
    growth_result.data = NULL;
    growth_result.capacity = 0;

    bool safe_to_double = true;
    size_t new_cap = 0;
    size_t allocation_bytes;

    if (element_size == 0)
    {
        growth_result.status = KESTREL_STATUS_INVALID_ARGUMENT;
        return growth_result;
    }

    if ((current_cap == 0 && data != NULL) || (data == NULL && current_cap > 0))
    {
        return growth_result;
    }

    if (required_count <= current_cap)
    {
        growth_result.status = KESTREL_STATUS_SUCCESS;
        growth_result.data = data;
        growth_result.capacity = current_cap;

        return growth_result;
    }


    if (current_cap > (SIZE_MAX / 2))
    {
        safe_to_double = false;
    }


    if (current_cap == 0 && required_count <= 4)
    {
        new_cap = 4;
    }

    else if (safe_to_double && ((current_cap * 2) >= required_count))
    {
        new_cap = current_cap * 2;
    }

    else
    {
        if (required_count > SIZE_MAX - (required_count / 4))
        {
            growth_result.status = KESTREL_STATUS_INVALID_ARGUMENT;
            growth_result.data = NULL;
            growth_result.capacity = 0;
            return growth_result;
        }
        new_cap = required_count + (required_count / 4);
    }

    if (new_cap > (SIZE_MAX / element_size))
    {
        growth_result.status = KESTREL_STATUS_INVALID_ARGUMENT;
        growth_result.data = NULL;
        growth_result.capacity = 0;
        return growth_result;
    }

    allocation_bytes = new_cap * element_size;

#ifdef KESTREL_TEST_FAIL_FIRST_BUILDER_REALLOC
    if (fail_next_realloc)
    {
        fail_next_realloc = false;
        growth_result.status = KESTREL_STATUS_OUT_OF_MEMORY;
        growth_result.data = NULL;
        growth_result.capacity = 0;
        return growth_result;
    }
#endif

    void *tmp = realloc(data, allocation_bytes);
    if (tmp == NULL)
    {
        growth_result.status = KESTREL_STATUS_OUT_OF_MEMORY;
        growth_result.data = NULL;
        growth_result.capacity = 0;
        return growth_result;
    }

    growth_result.status = KESTREL_STATUS_SUCCESS;
    growth_result.data = tmp;
    growth_result.capacity = new_cap;

    return growth_result;
}


static bool kestrel_source_kind_is_valid(kestrel_discovery_source_kind_t kind)
{
    switch (kind)
    {
    case KESTREL_DISCOVERY_SOURCE_HOST_CPU:
    case KESTREL_DISCOVERY_SOURCE_LINUX_SYSFS:
    case KESTREL_DISCOVERY_SOURCE_X86_CPUID:
    case KESTREL_DISCOVERY_SOURCE_CUDA_DRIVER:
    case KESTREL_DISCOVERY_SOURCE_VULKAN:
    case KESTREL_DISCOVERY_SOURCE_NVIDIA_NVML:
    case KESTREL_DISCOVERY_SOURCE_LEVEL_ZERO:
        return true;

    default:
        return false;
    }
}

static bool kestrel_compute_class_is_valid(kestrel_compute_class_t compute_class)
{
    switch (compute_class)
    {
    case KESTREL_COMPUTE_CLASS_CPU:
    case KESTREL_COMPUTE_CLASS_GPU:
    case KESTREL_COMPUTE_CLASS_ACCELERATOR:
    case KESTREL_COMPUTE_CLASS_UNKNOWN:
        return true;

    default:
        return false;
    }
}

static bool kestrel_gpu_integration_is_valid(kestrel_gpu_integration_t gpu_integration)
{
    switch (gpu_integration)
    {
    case KESTREL_GPU_INTEGRATION_UNKNOWN:
    case KESTREL_GPU_INTEGRATION_INTEGRATED:
    case KESTREL_GPU_INTEGRATION_NON_INTEGRATED:
    case KESTREL_GPU_INTEGRATION_NOT_APPLICABLE:
        return true;

    default:
        return false;
    }
}

static bool kestrel_physical_descriptor_is_valid(const kestrel_physical_observation_descriptor_t *descriptor)
{
    if (descriptor == NULL)
    {
        return false;
    }
    if (kestrel_compute_class_is_valid(descriptor->compute_class) == false)
    {
        return false;
    }
    if (kestrel_gpu_integration_is_valid(descriptor->gpu_integration) == false)
    {
        return false;
    }

    if (memchr(descriptor->name, '\0', sizeof(descriptor->name)) == NULL)
    {
        return false;
    }

    if (memchr(descriptor->vendor, '\0', sizeof(descriptor->vendor)) == NULL)
    {
        return false;
    }

    switch (descriptor->compute_class)
    {
    case KESTREL_COMPUTE_CLASS_CPU:
    case KESTREL_COMPUTE_CLASS_ACCELERATOR:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_NOT_APPLICABLE;

    case KESTREL_COMPUTE_CLASS_GPU:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_INTEGRATED ||
               descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_NON_INTEGRATED ||
               descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_UNKNOWN;

    case KESTREL_COMPUTE_CLASS_UNKNOWN:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_UNKNOWN;

    default:
        return false;
    }
}


static bool kestrel_execution_descriptor_is_valid(const kestrel_execution_observation_descriptor_t *descriptor)
{
    if (descriptor == NULL)
    {
        return false;
    }
    if (kestrel_compute_class_is_valid(descriptor->compute_class) == false)
    {
        return false;
    }
    if (kestrel_gpu_integration_is_valid(descriptor->gpu_integration) == false)
    {
        return false;
    }

    if (memchr(descriptor->name, '\0', sizeof(descriptor->name)) == NULL)
    {
        return false;
    }

    switch (descriptor->compute_class)
    {
    case KESTREL_COMPUTE_CLASS_CPU:
    case KESTREL_COMPUTE_CLASS_ACCELERATOR:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_NOT_APPLICABLE;

    case KESTREL_COMPUTE_CLASS_GPU:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_INTEGRATED ||
               descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_NON_INTEGRATED ||
               descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_UNKNOWN;

    case KESTREL_COMPUTE_CLASS_UNKNOWN:
        return descriptor->gpu_integration == KESTREL_GPU_INTEGRATION_UNKNOWN;

    default:
        return false;
    }
}

static bool kestrel_validate_memory_kind(kestrel_memory_kind_t memory_kind)
{
    switch (memory_kind)
    {
    case KESTREL_MEMORY_KIND_SYSTEM:
    case KESTREL_MEMORY_KIND_DEVICE_LOCAL:
    case KESTREL_MEMORY_KIND_UNKNOWN:
        return true;
    default:
        return false;
    }
}

static bool kestrel_memory_descriptor_is_valid(const kestrel_memory_observation_descriptor_t *description)
{
    if (description == NULL)
    {
        return false;
    }

    if (kestrel_validate_memory_kind(description->kind) == false)
    {
        return false;
    }

    if (memchr(description->name, '\0', sizeof(description->name)) == NULL)
    {
        return false;
    }

    return true;
}

static bool kestrel_validate_backend_family(kestrel_backend_family_t family)
{
    switch (family)
    {
    case KESTREL_BACKEND_FAMILY_SCALAR:
    case KESTREL_BACKEND_FAMILY_AVX2:
    case KESTREL_BACKEND_FAMILY_VULKAN:
    case KESTREL_BACKEND_FAMILY_CUDA:
    case KESTREL_BACKEND_FAMILY_LEVEL_ZERO:
        return true;
    default:
        return false;
    }
}

static bool kestrel_backend_descriptor_is_valid(const kestrel_backend_observation_descriptor_t *description)
{
    if (description == NULL)
    {
        return false;
    }

    if (kestrel_validate_backend_family(description->family) == false)
    {
        return false;
    }


    return true;
}


static void *kestrel_alloc_builder(size_t size)
{
#ifdef KESTREL_TEST_FAIL_FIRST_BUILDER_ALLOC
    if (fail_next_allocation)
    {
        fail_next_allocation = false;
        return NULL;
    }
#endif
    void *ptr = calloc(1, size);
    return ptr;
}


kestrel_status_t kestrel_topology_builder_create(kestrel_topology_builder_t **builder)
{
    kestrel_status_t status = KESTREL_STATUS_SUCCESS;
    if (builder == NULL)
    {
        status = KESTREL_STATUS_INVALID_ARGUMENT;
        return status;
    }

    if (*builder != NULL)
    {
        status = KESTREL_STATUS_INVALID_STATE;
        return status;
    }

    bool expected = false;

    if (!atomic_compare_exchange_strong(&is_builder_active, &expected, true))
    {
        status = KESTREL_STATUS_INVALID_STATE;
        return status;
    }

    kestrel_topology_builder_t *tmp = kestrel_alloc_builder(sizeof(struct kestrel_topology_builder));
    if (tmp == NULL)
    {
        status = KESTREL_STATUS_OUT_OF_MEMORY;
        atomic_store(&is_builder_active, false);
        return status;
    }


    tmp->sources_store.data = NULL;
    tmp->physical_observation_store.data = NULL;
    tmp->execution_observation_store.data = NULL;
    tmp->backend_observation_store.data = NULL;
    tmp->memory_observation_store.data = NULL;
    tmp->physical_identity_store.data = NULL;
    tmp->execution_identity_store.data = NULL;
    tmp->memory_identity_store.data = NULL;
    tmp->backing_relationship_store.data = NULL;
    tmp->subtarget_relationship_store.data = NULL;
    tmp->execution_memory_store.data = NULL;
    tmp->physical_locator_store.data = NULL;
    tmp->execution_locator_store.data = NULL;
    tmp->memory_locator_store.data = NULL;

    *builder = tmp;


    return status;
}

void kestrel_topology_builder_destroy(kestrel_topology_builder_t **builder)
{
    if (builder == NULL)
    {
        return;
    }

    if (*builder == NULL)
    {
        return;
    }

    free((*builder)->sources_store.data);
    free((*builder)->physical_observation_store.data);
    free((*builder)->memory_observation_store.data);
    free((*builder)->execution_observation_store.data);
    free((*builder)->backend_observation_store.data);


    free((*builder)->physical_identity_store.data);
    free((*builder)->execution_identity_store.data);
    free((*builder)->memory_identity_store.data);


    free((*builder)->backing_relationship_store.data);
    free((*builder)->subtarget_relationship_store.data);
    free((*builder)->execution_memory_store.data);


    free((*builder)->physical_locator_store.data);
    free((*builder)->execution_locator_store.data);
    free((*builder)->memory_locator_store.data);

    free(*builder);

    *builder = NULL;

    atomic_store(&is_builder_active, false);
}

kestrel_status_t kestrel_topology_builder_register_source(kestrel_topology_builder_t *builder,
                                                          const kestrel_source_descriptor_t *source_descriptor,
                                                          kestrel_discovery_source_id_t *source_id)

{
    if (builder == NULL || source_id == NULL || source_descriptor == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (kestrel_source_kind_is_valid(source_descriptor->kind) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    size_t cache_count = builder->sources_store.count;


    if (cache_count == SIZE_MAX || cache_count >= UINT32_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    for (size_t i = 0; i < cache_count; i++)
    {
        if (builder->sources_store.data[i].kind == source_descriptor->kind)
        {
            return KESTREL_STATUS_INVALID_STATE;
        }
    }

    size_t required_count = cache_count + 1;


    growth_output result = kestrel_growth_helper(builder->sources_store.data,
                                                 builder->sources_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_source_descriptor_t));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->sources_store.data = result.data;
    builder->sources_store.capacity = result.capacity;
    builder->sources_store.data[builder->sources_store.count] = *source_descriptor;

    builder->sources_store.count = required_count;

    source_id->value = (uint32_t)builder->sources_store.count;


    return KESTREL_STATUS_SUCCESS;
}


kestrel_status_t kestrel_topology_builder_register_physical_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_physical_observation_descriptor_t *physical_descriptor,
    kestrel_physical_observation_id_t *output_id)
{

    if (builder == NULL || output_id == NULL || physical_descriptor == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_physical_descriptor_is_valid(physical_descriptor) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    size_t cache_count = builder->physical_observation_store.count;


    if (cache_count == SIZE_MAX || cache_count >= UINT32_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    growth_output result = kestrel_growth_helper(builder->physical_observation_store.data,
                                                 builder->physical_observation_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_physical_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->physical_observation_store.data = result.data;
    builder->physical_observation_store.capacity = result.capacity;
    kestrel_physical_obs_obj obj_to_insert;

    obj_to_insert.description = *physical_descriptor;
    obj_to_insert.reporting_source = reporting_source_id;

    builder->physical_observation_store.data[builder->physical_observation_store.count] = obj_to_insert;

    builder->physical_observation_store.count = required_count;

    output_id->value = (uint32_t)builder->physical_observation_store.count;
    return KESTREL_STATUS_SUCCESS;
}

kestrel_status_t kestrel_topology_builder_register_execution_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_execution_observation_descriptor_t *execution_descriptor,
    kestrel_execution_observation_id_t *output_id)
{
    if (builder == NULL || output_id == NULL || execution_descriptor == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if ((reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count))
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_execution_descriptor_is_valid(execution_descriptor) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    size_t cache_count = builder->execution_observation_store.count;

    if (cache_count == SIZE_MAX || cache_count >= UINT32_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }


    size_t required_count = cache_count + 1;
    growth_output result = kestrel_growth_helper(builder->execution_observation_store.data,
                                                 builder->execution_observation_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_execution_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->execution_observation_store.data = result.data;
    builder->execution_observation_store.capacity = result.capacity;
    kestrel_execution_obs_obj obj_to_insert;

    obj_to_insert.description = *execution_descriptor;
    obj_to_insert.reporting_source = reporting_source_id;

    builder->execution_observation_store.data[builder->execution_observation_store.count] = obj_to_insert;

    builder->execution_observation_store.count = required_count;

    output_id->value = (uint32_t)builder->execution_observation_store.count;
    return KESTREL_STATUS_SUCCESS;
}


kestrel_status_t kestrel_topology_builder_register_memory_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_memory_observation_descriptor_t *memory_descriptor,
    kestrel_memory_observation_id_t *output_id)
{
    if (builder == NULL || output_id == NULL || memory_descriptor == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_memory_descriptor_is_valid(memory_descriptor) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    size_t cache_count = builder->memory_observation_store.count;

    if (cache_count == SIZE_MAX || cache_count >= UINT32_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }


    size_t required_count = cache_count + 1;
    growth_output result = kestrel_growth_helper(builder->memory_observation_store.data,
                                                 builder->memory_observation_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_memory_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->memory_observation_store.data = result.data;
    builder->memory_observation_store.capacity = result.capacity;
    kestrel_memory_obs_obj obj_to_insert;

    obj_to_insert.description = *memory_descriptor;
    obj_to_insert.reporting_source = reporting_source_id;

    builder->memory_observation_store.data[builder->memory_observation_store.count] = obj_to_insert;

    builder->memory_observation_store.count = required_count;

    output_id->value = (uint32_t)builder->memory_observation_store.count;
    return KESTREL_STATUS_SUCCESS;
}

kestrel_status_t kestrel_topology_builder_register_backend_observation(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    const kestrel_backend_observation_descriptor_t *backend_descriptor,
    kestrel_backend_observation_id_t *output_id)
{
    if (builder == NULL || output_id == NULL || backend_descriptor == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (backend_descriptor->execution_observation_id.value == 0 ||
        backend_descriptor->execution_observation_id.value > builder->execution_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    uint32_t index = backend_descriptor->execution_observation_id.value - 1;
    kestrel_execution_obs_obj observation = builder->execution_observation_store.data[index];

    if (observation.reporting_source.value != reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (kestrel_backend_descriptor_is_valid(backend_descriptor) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    size_t cache_count = builder->backend_observation_store.count;


    if (cache_count == SIZE_MAX || cache_count >= UINT32_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    growth_output result = kestrel_growth_helper(builder->backend_observation_store.data,
                                                 builder->backend_observation_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_backend_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->backend_observation_store.data = result.data;
    builder->backend_observation_store.capacity = result.capacity;
    kestrel_backend_obs_obj obj_to_insert;

    obj_to_insert.description = *backend_descriptor;
    obj_to_insert.reporting_source = reporting_source_id;

    builder->backend_observation_store.data[builder->backend_observation_store.count] = obj_to_insert;

    builder->backend_observation_store.count = required_count;

    output_id->value = (uint32_t)builder->backend_observation_store.count;
    return KESTREL_STATUS_SUCCESS;
}


static bool kestrel_nvml_uuid_text_is_valid(const uint8_t *bytes, uint8_t length, const char *expected_prefix)
{
    if (bytes == NULL || expected_prefix == NULL || length != 40)
    {
        return false;
    }

    if (memcmp(bytes, expected_prefix, 4) != 0)
    {
        return false;
    }

    for (uint8_t i = 4; i < length; i++)
    {
        if (i == 12 || i == 17 || i == 22 || i == 27)
        {
            if (bytes[i] != '-')
            {
                return false;
            }
        }
        else if (!isxdigit(bytes[i]))
        {
            return false;
        }
    }

    return true;
}


static bool kestrel_identity_is_valid(const kestrel_identity_value_t *identity)
{
    if (identity == NULL)
    {
        return false;
    }


    if (identity->length == 0 || identity->length > KESTREL_MAX_IDENTITY_BYTES)
    {
        return false;
    }

    switch (identity->identity_kind)
    {
    case KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID:
    case KESTREL_IDENTITY_KIND_VULKAN_DEVICE_UUID:
    case KESTREL_IDENTITY_KIND_LEVEL_ZERO_DEVICE_UUID:
    case KESTREL_IDENTITY_KIND_NVDIMM_NAMESPACE_UUID:
    case KESTREL_IDENTITY_KIND_CXL_PERSISTENT_REGION_UUID:
        return identity->length == 16;
    case KESTREL_IDENTITY_KIND_NVIDIA_GPU_UUID:
        return kestrel_nvml_uuid_text_is_valid(identity->bytes, identity->length, "GPU-");

    case KESTREL_IDENTITY_KIND_NVIDIA_MIG_UUID:
        return kestrel_nvml_uuid_text_is_valid(identity->bytes, identity->length, "MIG-");
    default:
        return false;
    }
}

kestrel_status_t kestrel_topology_builder_register_physical_identity(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_physical_observation_id_t observation_owner,
    const kestrel_identity_value_t *identity)
{
    if (builder == NULL || identity == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (observation_owner.value == 0 || observation_owner.value > builder->physical_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (builder->physical_observation_store.data[observation_owner.value - 1].reporting_source.value !=
        reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_identity_is_valid(identity) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (identity->identity_kind != KESTREL_IDENTITY_KIND_NVIDIA_GPU_UUID)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (builder->physical_observation_store.data[observation_owner.value - 1].description.compute_class !=
        KESTREL_COMPUTE_CLASS_GPU)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    size_t cache_count = builder->physical_identity_store.count;

    for (size_t i = 0; i < cache_count; i++)
    {
        const kestrel_physical_identity_obs_obj *existing = &builder->physical_identity_store.data[i];

        if (existing->reporting_source.value == reporting_source_id.value &&
            existing->observation_id.value == observation_owner.value &&
            existing->identity.identity_kind == identity->identity_kind &&
            existing->identity.length == identity->length &&
            memcmp(existing->identity.bytes, identity->bytes, identity->length) == 0)
        {
            return KESTREL_STATUS_SUCCESS;
        }
    }


    if (cache_count == SIZE_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    kestrel_identity_value_t new_identity = {0};

    new_identity.identity_kind = identity->identity_kind;
    new_identity.length = identity->length;

    memcpy(new_identity.bytes, identity->bytes, identity->length);

    kestrel_physical_identity_obs_obj obj_to_insert = {0};

    obj_to_insert.identity = new_identity;
    obj_to_insert.observation_id = observation_owner;
    obj_to_insert.reporting_source = reporting_source_id;

    growth_output result = kestrel_growth_helper(builder->physical_identity_store.data,
                                                 builder->physical_identity_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_physical_identity_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->physical_identity_store.data = result.data;
    builder->physical_identity_store.capacity = result.capacity;

    builder->physical_identity_store.data[builder->physical_identity_store.count] = obj_to_insert;

    builder->physical_identity_store.count = required_count;

    return KESTREL_STATUS_SUCCESS;
}


kestrel_status_t kestrel_topology_builder_register_execution_identity(
    kestrel_topology_builder_t *builder,
    kestrel_discovery_source_id_t reporting_source_id,
    kestrel_execution_observation_id_t observation_owner,
    const kestrel_identity_value_t *identity)
{
    if (builder == NULL || identity == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (observation_owner.value == 0 || observation_owner.value > builder->execution_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (builder->execution_observation_store.data[observation_owner.value - 1].reporting_source.value !=
        reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_identity_is_valid(identity) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    switch (identity->identity_kind)
    {
    case KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID:
    case KESTREL_IDENTITY_KIND_VULKAN_DEVICE_UUID:
    case KESTREL_IDENTITY_KIND_NVIDIA_MIG_UUID:
    case KESTREL_IDENTITY_KIND_LEVEL_ZERO_DEVICE_UUID:
        break;

    default:
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (identity->identity_kind == KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID ||
        identity->identity_kind == KESTREL_IDENTITY_KIND_NVIDIA_MIG_UUID)
    {
        kestrel_compute_class_t compute_class =
            builder->execution_observation_store.data[observation_owner.value - 1].description.compute_class;

        if (compute_class != KESTREL_COMPUTE_CLASS_GPU && compute_class != KESTREL_COMPUTE_CLASS_UNKNOWN)
        {
            return KESTREL_STATUS_INVALID_ARGUMENT;
        }
    }


    size_t cache_count = builder->execution_identity_store.count;


    if (cache_count == SIZE_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    kestrel_identity_value_t new_identity = {0};
    new_identity.identity_kind = identity->identity_kind;
    memcpy(new_identity.bytes, identity->bytes, identity->length);
    new_identity.length = identity->length;

    growth_output result = kestrel_growth_helper(builder->execution_identity_store.data,
                                                 builder->execution_identity_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_execution_identity_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->execution_identity_store.data = result.data;
    builder->execution_identity_store.capacity = result.capacity;
    kestrel_execution_identity_obs_obj obj_to_insert;


    obj_to_insert.identity = new_identity;
    obj_to_insert.reporting_source = reporting_source_id;
    obj_to_insert.observation_id = observation_owner;

    builder->execution_identity_store.data[builder->execution_identity_store.count] = obj_to_insert;

    builder->execution_identity_store.count = required_count;

    return KESTREL_STATUS_SUCCESS;
}

kestrel_status_t kestrel_topology_builder_register_memory_identity(kestrel_topology_builder_t *builder,
                                                                   kestrel_discovery_source_id_t reporting_source_id,
                                                                   kestrel_memory_observation_id_t observation_owner,
                                                                   const kestrel_identity_value_t *identity)
{
    if (builder == NULL || identity == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (observation_owner.value == 0 || observation_owner.value > builder->memory_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (builder->memory_observation_store.data[observation_owner.value - 1].reporting_source.value !=
        reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (kestrel_identity_is_valid(identity) == false)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    switch (identity->identity_kind)
    {
    case KESTREL_IDENTITY_KIND_NVDIMM_NAMESPACE_UUID:
    case KESTREL_IDENTITY_KIND_CXL_PERSISTENT_REGION_UUID:
        break;

    default:
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    size_t cache_count = builder->memory_identity_store.count;


    if (cache_count == SIZE_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    kestrel_identity_value_t new_identity = {0};
    new_identity.identity_kind = identity->identity_kind;
    memcpy(new_identity.bytes, identity->bytes, identity->length);
    new_identity.length = identity->length;

    growth_output result = kestrel_growth_helper(builder->memory_identity_store.data,
                                                 builder->memory_identity_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_memory_identity_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->memory_identity_store.data = result.data;
    builder->memory_identity_store.capacity = result.capacity;
    kestrel_memory_identity_obs_obj obj_to_insert;


    obj_to_insert.identity = new_identity;
    obj_to_insert.reporting_source = reporting_source_id;
    obj_to_insert.observation_id = observation_owner;

    builder->memory_identity_store.data[builder->memory_identity_store.count] = obj_to_insert;

    builder->memory_identity_store.count = required_count;

    return KESTREL_STATUS_SUCCESS;
}


kestrel_status_t kestrel_topology_builder_register_backing(kestrel_topology_builder_t *builder,
                                                           kestrel_discovery_source_id_t reporting_source_id,
                                                           kestrel_execution_observation_id_t observation_owner,
                                                           kestrel_physical_observation_id_t physical_backer_id)
{
    if (builder == NULL)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (reporting_source_id.value == 0 || reporting_source_id.value > builder->sources_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    if (observation_owner.value == 0 || observation_owner.value > builder->execution_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (physical_backer_id.value == 0 || physical_backer_id.value > builder->physical_observation_store.count)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    kestrel_discovery_source_id_t physical_source =
        builder->physical_observation_store.data[physical_backer_id.value - 1].reporting_source;

    kestrel_discovery_source_id_t execution_source =
        builder->execution_observation_store.data[observation_owner.value - 1].reporting_source;

    if (physical_source.value != reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    if (execution_source.value != reporting_source_id.value)
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }

    kestrel_compute_class_t physical_class =
        builder->physical_observation_store.data[physical_backer_id.value - 1].description.compute_class;

    kestrel_compute_class_t execution_class =
        builder->execution_observation_store.data[observation_owner.value - 1].description.compute_class;

    if ((physical_class == KESTREL_COMPUTE_CLASS_CPU && execution_class == KESTREL_COMPUTE_CLASS_GPU) ||
        (physical_class == KESTREL_COMPUTE_CLASS_GPU && execution_class == KESTREL_COMPUTE_CLASS_CPU))
    {
        return KESTREL_STATUS_INVALID_ARGUMENT;
    }


    size_t cache_count = builder->backing_relationship_store.count;
    kestrel_backing_obs_obj obj_to_insert;


    obj_to_insert.execution_id = observation_owner;
    obj_to_insert.physical_id = physical_backer_id;
    obj_to_insert.reporting_source = reporting_source_id;

    for (size_t i = 0; i < cache_count; i++)
    {
        const kestrel_backing_obs_obj *existing = &builder->backing_relationship_store.data[i];

        if (existing->reporting_source.value == reporting_source_id.value &&
            existing->execution_id.value == observation_owner.value &&
            existing->physical_id.value == physical_backer_id.value)
        {
            return KESTREL_STATUS_SUCCESS;
        }
    }


    if (cache_count == SIZE_MAX)
    {
        return KESTREL_STATUS_INVALID_STATE;
    }

    size_t required_count = cache_count + 1;


    growth_output result = kestrel_growth_helper(builder->backing_relationship_store.data,
                                                 builder->backing_relationship_store.capacity,
                                                 required_count,
                                                 sizeof(kestrel_backing_obs_obj));


    if (result.status != KESTREL_STATUS_SUCCESS)
    {
        return result.status;
    }

    builder->backing_relationship_store.data = result.data;
    builder->backing_relationship_store.capacity = result.capacity;

    builder->backing_relationship_store.data[builder->backing_relationship_store.count] = obj_to_insert;

    builder->backing_relationship_store.count = required_count;

    return KESTREL_STATUS_SUCCESS;
}

void kestrel_runtime_topology_destroy(kestrel_runtime_topology_t **topology)
{
    if (topology == NULL)
    {
        return;
    }

    if (*topology == NULL)
    {
        return;
    }

    free(*topology);

    *topology = NULL;
}

int main(void)
{
    kestrel_source_descriptor_t source1;
    kestrel_source_descriptor_t source2;
    kestrel_source_descriptor_t source3;

    source1.kind = KESTREL_DISCOVERY_SOURCE_HOST_CPU;
    source2.kind = KESTREL_DISCOVERY_SOURCE_INVALID;
    source3.kind = (kestrel_discovery_source_kind_t)99;

    printf("%d\n", kestrel_source_kind_is_valid(source1.kind));
    printf("%d\n", kestrel_source_kind_is_valid(source2.kind));
    printf("%d\n", kestrel_source_kind_is_valid(source3.kind));
}
