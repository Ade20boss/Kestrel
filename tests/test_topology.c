#include "kestrel_core.h"
#include "kestrel_topology.h"
#include <stdio.h>
#include <string.h>


static int fails = 0, total = 0;
#define CHECK(cond, msg)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        total++;                                                                                                       \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            fails++;                                                                                                   \
            printf("  FAIL: %s (line %d)\n", msg, __LINE__);                                                           \
        }                                                                                                              \
    } while (0)


int main(void)
{
    /*1. BUILDER CREATION - HAPPY PATH */
    kestrel_topology_builder_t *builder = NULL;

    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_SUCCESS,
          "builder create suceeds on a NULL builder pointer object");

    CHECK(builder != NULL, "builder pointer is now LIVE (not null)");

    /*2. BUILDER DESTRUCTION - HAPPY PATH */

    kestrel_topology_builder_destroy(&builder);

    CHECK(builder == NULL, "After destruction, builder object is NULL");

    /*3. DESTROY SAFETY */

    kestrel_topology_builder_destroy(NULL);

    kestrel_topology_builder_destroy(&builder);

    CHECK(builder == NULL, "After destruction of NULL object, builder object is NULL");

    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_SUCCESS,
          "builder create suceeds on a NULL builder pointer object");
    kestrel_topology_builder_destroy(&builder);
    kestrel_topology_builder_destroy(&builder);
    CHECK(builder == NULL, "After destruction of NULL object, builder object is NULL");

    /*4. CREATE NULL */
    CHECK(kestrel_topology_builder_create(NULL) == KESTREL_STATUS_INVALID_ARGUMENT,
          "Passing a NULL object to create function returns an Invalid status");

    /*5. Creation into an already-live pointer */

    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_SUCCESS,
          "builder create suceeds on a NULL builder pointer object");

    kestrel_topology_builder_t *original = builder;

    CHECK(kestrel_topology_builder_create(&builder) == KESTREL_STATUS_INVALID_STATE,
          "builder create on a live object returns an invalid state");

    CHECK(builder == original, "Live builder pointer remains untouched after failed create");

    kestrel_topology_builder_destroy(&builder);

    /*6. Runtime Topology Destruction */

    kestrel_runtime_topology_t *topology = NULL;

    kestrel_runtime_topology_destroy(NULL);

    kestrel_runtime_topology_destroy(&topology);

    CHECK(topology == NULL, "After destruction of NULL object, topology object is NULL");

    /* 7. INVALID VALUES ARE ZERO */

    CHECK(KESTREL_COMPUTE_CLASS_INVALID == 0, "Compute class invalid state is zero");
    CHECK(KESTREL_BACKEND_FAMILY_INVALID == 0, "Backend family invalid state is zero");
    CHECK(KESTREL_MEMORY_KIND_INVALID == 0, "Memory kind invalid state is zero");
    CHECK(KESTREL_IDENTITY_KIND_INVALID == 0, "Identity kind invalid state is zero");
    CHECK(KESTREL_MAX_TOPOLOGY_NAME == 128, "Max topology name is 128");
    CHECK(KESTREL_MAX_VENDOR_NAME == 64, "Max vendor name is 64");
    CHECK(KESTREL_MAX_IDENTITY_BYTES == 64, "Max identity bytes is 64");

    /*8. Strong ID representation */
    kestrel_physical_device_id_t p_id;
    p_id.value = 42;
    CHECK(p_id.value == 42, "Physical device ID securely wraps uint32_t");

    kestrel_execution_target_id_t exec_id;
    exec_id.value = 100;
    CHECK(exec_id.value == 100, "Execution target ID securely wraps uint32_t");

    kestrel_backend_instance_id_t binst_id;
    binst_id.value = 50;
    CHECK(binst_id.value == 50, "Backend instance ID securely wraps uint32_t");

    kestrel_memory_space_id_t memspace_id;
    memspace_id.value = 39;
    CHECK(memspace_id.value == 39, "Memory space ID securely wraps uint32_t");

    kestrel_physical_device_t p1;
    p1.id.value = 1;
    p1.compute_class = KESTREL_COMPUTE_CLASS_CPU;
    strncpy(p1.name, "CPU 1", KESTREL_MAX_TOPOLOGY_NAME);
    strncpy(p1.vendor, "INTEL CPU", KESTREL_MAX_VENDOR_NAME);
    CHECK(p1.id.value == 1, "Physical device stores ID");
    CHECK(p1.compute_class == KESTREL_COMPUTE_CLASS_CPU, "Physical device stores CPU class");
    CHECK(strncmp(p1.name, "CPU 1", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Physical device stores name");
    CHECK(strncmp(p1.vendor, "INTEL CPU", KESTREL_MAX_VENDOR_NAME) == 0, "Physical device stores vendor name");


    kestrel_execution_target_t e1;
    e1.id.value = 2;
    e1.compute_class = KESTREL_COMPUTE_CLASS_CPU;
    strncpy(e1.name, "EXECUTION TARGET 1", KESTREL_MAX_TOPOLOGY_NAME);
    CHECK(e1.id.value == 2, "Execution target stores ID");
    CHECK(e1.compute_class == KESTREL_COMPUTE_CLASS_CPU, "Execution target stores CPU class");
    CHECK(strncmp(e1.name, "EXECUTION TARGET 1", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Execution target stores name");

    kestrel_backend_instance_t b1, b2;
    b1.id.value = 3;
    b1.family = KESTREL_BACKEND_FAMILY_SCALAR;
    b1.execution_target_id = e1.id;

    CHECK(b1.id.value == 3, "Backend instance stores ID");
    CHECK(b1.family == KESTREL_BACKEND_FAMILY_SCALAR, "Backend instance stores family");
    CHECK(b1.execution_target_id.value == e1.id.value, "Backend instance stores execution target ID");

    b2.id.value = 4;
    b2.family = KESTREL_BACKEND_FAMILY_AVX2;
    b2.execution_target_id = e1.id;

    CHECK(b2.id.value == 4, "Backend instance stores ID");
    CHECK(b2.family == KESTREL_BACKEND_FAMILY_AVX2, "Backend instance stores family");
    CHECK(b2.execution_target_id.value == e1.id.value, "Backend instance stores execution target ID");

    // 4. Create the Memory Space
    kestrel_memory_space_t m1;
    m1.id.value = 1;
    m1.kind = KESTREL_MEMORY_KIND_SYSTEM;
    strncpy(m1.name, "16gb DDR5 RAM", KESTREL_MAX_TOPOLOGY_NAME);
    m1.capacity_known = true;
    m1.capacity_bytes = 17179869184;

    CHECK(m1.id.value == 1, "Memory space stores ID");
    CHECK(m1.kind == KESTREL_MEMORY_KIND_SYSTEM, "Memory space stores SYSTEM kind");
    CHECK(strncmp(m1.name, "16gb DDR5 RAM", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Memory space stores name");
    CHECK(m1.capacity_known == true, "Memory spaces stores capacity known state");
    CHECK(m1.capacity_bytes == 17179869184, "Memory spaces stores capacity in bytes in capacity is known");


    // 5. Prove the graph can be connected via relation structs
    kestrel_physical_execution_relation_t p_e_rel;
    p_e_rel.physical_device_id = p1.id;
    p_e_rel.execution_target_id = e1.id;
    CHECK(p_e_rel.physical_device_id.value == 1 && p_e_rel.execution_target_id.value == 2,
          "Physical-Execution relation successfully maps device to target");

    kestrel_execution_memory_relation_t e_m_rel;
    e_m_rel.execution_target_id = e1.id;
    e_m_rel.memory_space_id = m1.id;
    CHECK(e_m_rel.execution_target_id.value == 2 && e_m_rel.memory_space_id.value == 1,
          "Execution-Memory relation successfully maps target to memory");


    kestrel_execution_target_t e2;
    e2.id.value = 3;
    e2.compute_class = KESTREL_COMPUTE_CLASS_CPU;
    strncpy(e2.name, "EXECUTION TARGET 2", KESTREL_MAX_TOPOLOGY_NAME);
    CHECK(e2.id.value == 3, "Execution target stores ID");
    CHECK(e2.compute_class == KESTREL_COMPUTE_CLASS_CPU, "Execution target stores CPU class");
    CHECK(strncmp(e2.name, "EXECUTION TARGET 2", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Execution target stores name");

    kestrel_execution_hierarchy_relation_t h_rel;
    h_rel.parent_target_id = e1.id;
    h_rel.child_target_id = e2.id;

    CHECK(h_rel.parent_target_id.value == 2 && h_rel.child_target_id.value == 3,
          "Hierarchy relation successfully maps parent to child");


    kestrel_execution_target_t e3;
    e3.id.value = 4;
    e3.compute_class = KESTREL_COMPUTE_CLASS_CPU;
    strncpy(e3.name, "EXECUTION TARGET 3", KESTREL_MAX_TOPOLOGY_NAME);
    CHECK(e3.id.value == 4, "Execution target stores ID");
    CHECK(e3.compute_class == KESTREL_COMPUTE_CLASS_CPU, "Execution target stores CPU class");
    CHECK(strncmp(e3.name, "EXECUTION TARGET 3", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Execution target stores name");


    kestrel_physical_device_t p2;
    p2.id.value = 2;
    p2.compute_class = KESTREL_COMPUTE_CLASS_CPU;
    strncpy(p2.name, "CPU 2", KESTREL_MAX_TOPOLOGY_NAME);
    strncpy(p2.vendor, "AMD CPU", KESTREL_MAX_VENDOR_NAME);
    CHECK(p2.id.value == 2, "Physical device stores ID");
    CHECK(p2.compute_class == KESTREL_COMPUTE_CLASS_CPU, "Physical device stores CPU class");
    CHECK(strncmp(p2.name, "CPU 2", KESTREL_MAX_TOPOLOGY_NAME) == 0, "Physical device stores name");
    CHECK(strncmp(p2.vendor, "AMD CPU", KESTREL_MAX_VENDOR_NAME) == 0, "Physical device stores vendor name");


    kestrel_physical_execution_relation_t multi_rel_1, multi_rel_2;
    multi_rel_1.execution_target_id = e3.id;
    multi_rel_1.physical_device_id = p1.id;

    multi_rel_2.execution_target_id = e3.id;
    multi_rel_2.physical_device_id = p2.id;

    CHECK(multi_rel_1.execution_target_id.value == 4 && multi_rel_1.physical_device_id.value == 1,
          "Target 3 is backed by Physical Device 1");
    CHECK(multi_rel_2.execution_target_id.value == 4 && multi_rel_2.physical_device_id.value == 2,
          "Target 3 is ALSO backed by Physical Device 2");


    /* 13. Identity Value Representation */
    kestrel_identity_value_t cuda_id;
    cuda_id.identity_kind = KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID;
    cuda_id.length = 16;
    cuda_id.bytes[0] = 0xAB; // Just setting one byte to prove the array is accessible

    CHECK(cuda_id.identity_kind == KESTREL_IDENTITY_KIND_CUDA_DEVICE_UUID, "Identity stores kind");
    CHECK(cuda_id.length == 16, "Identity stores length");
    CHECK(cuda_id.bytes[0] == 0xAB, "Identity stores raw byte evidence");

    // Final tally and exit
    printf("\nTotal: %d, Fails: %d\n", total, fails);
    return fails == 0 ? 0 : 1;
}
