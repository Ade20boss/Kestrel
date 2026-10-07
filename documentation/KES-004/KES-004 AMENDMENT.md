## KES-004 Slice 1 — Amendment A: True Opaque Ownership

Public `topology.h` conceptually declares:

```
typedef struct kestrel_topology_builder kestrel_topology_builder_t;
typedef struct kestrel_runtime_topology kestrel_runtime_topology_t;
```

Their definitions exist **only inside `topology.c`**.

Therefore a caller can do:

```
kestrel_topology_builder_t *builder;
kestrel_runtime_topology_t *topology;
```

but cannot do:

```
kestrel_topology_builder_t builder;
sizeof(kestrel_topology_builder_t);
builder.some_field;
```

That's the opacity we actually want.

### The lifecycle changes

We no longer have:

```
by-value ZERO object = all bytes zero
```

For these two opaque owning objects, ZERO is simply:

```
builder  == NULL
topology == NULL
```

So builder lifecycle becomes:

```
NULL
 ↓ create
LIVE builder*
 ↓ destroy
NULL
```

And runtime topology:

```
NULL
 ↓ successful finalize later
LIVE immutable topology*
 ↓ destroy
NULL
```

That is cleaner.

### Replace `builder_init`

The old ticket said:

```
kestrel_topology_builder_init
```

Drop that.

The public lifecycle should instead conceptually be:

```
kestrel_topology_builder_create
kestrel_topology_builder_destroy

kestrel_runtime_topology_destroy
```

I want `create` to follow Kestrel's existing status/error convention rather than return a raw pointer with ambiguous failure reporting.

The contract should be:

```
create(out_builder)

pre:
    out_builder != NULL
    *out_builder == NULL

success:
    *out_builder → valid LIVE builder

failure:
    *out_builder remains NULL
```

This preserves the transactional failure contract perfectly.

### Destroy should take pointer-to-pointer

This is worth doing:

```
destroy(&builder)
```

rather than:

```
destroy(builder)
```

because then Kestrel can actually restore the caller's ownership variable to ZERO:

```
before:
builder → LIVE object

destroy(&builder)

after:
builder == NULL
```

Same for runtime topology:

```
destroy(&topology)

topology == NULL
```

So the destroy contract becomes:

```
destroy(NULL)        → safe
destroy(&NULL)       → safe
destroy(&LIVE)       → frees object, writes NULL
destroy twice        → safe
```

That is stronger than our previous handle approach.

---

## This also makes future `finalize()` cleaner

We are **not implementing this in Slice 1**, but we should freeze what the eventual ownership transition means.

Conceptually:

```
builder*
   ↓ finalize
runtime_topology*
```

On success:

```
before:

builder  → LIVE
topology = NULL
```

after:

```
builder  = NULL
topology → LIVE immutable snapshot
```

On failure:

```
builder  → still LIVE
topology = NULL
```

That gives us the exact transactional ownership contract we've wanted from the beginning without pretending some public struct has a magical ZERO representation.

---

## What remains public and non-opaque?

The **individual topology records** can still be ordinary public structs:

```
kestrel_physical_device_t

kestrel_execution_target_t

kestrel_backend_instance_t

kestrel_memory_space_t

kestrel_physical_execution_relation_t

kestrel_execution_hierarchy_relation_t

kestrel_execution_memory_relation_t
```

That's okay.

We're hiding the **owning topology container**, not pretending nobody is allowed to know what a physical-device record looks like.

So later:

```
runtime topology
      ↓ query
const PhysicalDevice
```

The user can inspect:

```
id
compute_class
name
vendor
```

They just cannot reach into the topology and manipulate its arrays, counts, capacities, indexes, ownership state, etc.

Same distinction for the builder: the construction machinery is private.

---

## Private `topology.c`

Eventually, only `topology.c` knows something resembling:

```
struct kestrel_topology_builder
{
    observation arrays
    claims
    evidence
    temporary mappings
    ...
}

struct kestrel_runtime_topology
{
    physical devices
    execution targets
    backends
    memory spaces

    relationships
    retained identity metadata

    counts
    private indexes
    ...
}
```

Those layouts can change later without changing `topology.h`.

That's a major advantage.

---

## Slice 1 tests change slightly

Remove tests involving:

```
zero-initialized by-value builder
zero-initialized by-value topology
exact byte-zero restoration
```

Replace them with ownership tests:

```
builder pointer initially NULL

create:
    NULL → LIVE

destroy:
    LIVE → NULL

destroy(&NULL):
    safe

destroy(NULL):
    safe

destroy twice:
    safe

create when *out_builder is already LIVE:
    fails
    existing builder unchanged

failed create:
    output remains NULL
```

For runtime topology in this slice:

```
topology pointer initially NULL

destroy(&NULL):
    safe

destroy(NULL):
    safe
```

There is still no way to create a LIVE runtime topology until `finalize()` arrives.

---

## Everything else in the ticket survives

No change to:

```
Physical Device
Execution Target
Backend Instance
Memory Space

BACKED_BY
SUBTARGET_OF
Execution↔Memory

strongly typed IDs
compute class
backend family
memory kind
identity metadata records
```

No change to the architecture itself.

We're only correcting the **public ownership/API representation**.

So the revised mental model is now:

```
PUBLIC

kestrel_topology_builder_t *       opaque mutable owner
kestrel_runtime_topology_t *       opaque immutable owner

PhysicalDevice                     visible record
ExecutionTarget                    visible record
MemorySpace                        visible record
BackendInstance                    visible record
relationships                      visible records
```

And:

```
NULL builder
     ↓ create
opaque mutable construction object
     ↓ eventually finalize
opaque immutable topology object
```

I would mark the previous **“small public handle containing `impl*`” decision as SUPERSEDED**.

This version is cleaner, actually opaque, and makes ownership much harder to misuse.

**KES-004 Slice 1 remains OPEN with this amendment applied.**