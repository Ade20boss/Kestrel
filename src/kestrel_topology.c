#include "kestrel_topology.h"
#include <stdlib.h>


struct kestrel_topology_builder
{
    int reserved;
};

struct kestrel_runtime_topology
{
    int reserved;
};


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

    kestrel_topology_builder_t *tmp = calloc(1, sizeof(struct kestrel_topology_builder));
    if (tmp == NULL)
    {
        status = KESTREL_STATUS_OUT_OF_MEMORY;
        return status;
    }

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

    free(*builder);

    *builder = NULL;
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
