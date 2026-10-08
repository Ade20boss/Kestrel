For **KES-004 Slice 1**, keep `tests/test_topology.c` deliberately boring. We are **not testing topology reconciliation yet**. We are only testing two things:

1. the lifecycle contract we just implemented;
2. that the public topology types can represent the model we froze.

I’d write the tests in this order.

### 1. Builder creation — happy path

Start with:

```
builder = NULL

call create(&builder)

expect:
    status == SUCCESS
    builder != NULL
```

This proves:

```
NULL → LIVE
```

Do **not** inspect the opaque builder internals. The whole point of the opaque type is that the test can't.

---

### 2. Builder destruction — happy path

Using a live builder:

```
builder != NULL

destroy(&builder)

expect:
    builder == NULL
```

This proves:

```
LIVE → NULL
```

The important assertion isn't merely that it didn't crash.

It's that the function changed the caller's pointer back to `NULL`.

---

### 3. Destroy safety

Test these separately:

```
destroy(NULL)
```

must not crash.

Then:

```
builder = NULL
destroy(&builder)
```

must leave:

```
builder == NULL
```

Then:

```
create(&builder)
destroy(&builder)
destroy(&builder)
```

must also be safe.

That covers our idempotent-ish destruction contract.

---

### 4. `create(NULL)`

Call creation with no output-pointer storage:

```
create(NULL)
```

Expected:

```
KESTREL_STATUS_INVALID_ARGUMENT
```

This corresponds to:

```
builder**
    ↓
NULL
```

There is literally nowhere for Kestrel to place the created pointer.

---

### 5. Creation into an already-live pointer

This one matters because of our transactional lifecycle contract.

Conceptually:

```
create(&builder)
original = builder

create(&builder) again
```

Second call must return:

```
KESTREL_STATUS_INVALID_STATE
```

and crucially:

```
builder == original
```

The existing live builder must **not be destroyed, replaced, nulled, or modified**.

That's the first real test of the:

> failure leaves caller-visible state unchanged

rule.

---

### 6. Runtime-topology destruction

We can't publicly create a runtime topology yet because finalization doesn't exist.

So Slice 1 can only test its ZERO-state destruction:

```
topology = NULL

runtime_topology_destroy(NULL)
runtime_topology_destroy(&topology)

expect:
    topology == NULL
```

Don't invent a fake topology creator just to test the destructor.

We'll properly test:

```
LIVE topology → NULL
```

once finalize exists.

---

Then switch from lifecycle tests to **representation-contract tests**.

### 7. Invalid values are zero

Verify:

```
KESTREL_COMPUTE_CLASS_INVALID == 0
KESTREL_BACKEND_FAMILY_INVALID == 0
KESTREL_MEMORY_KIND_INVALID == 0
KESTREL_IDENTITY_KIND_INVALID == 0
```

This protects a surprisingly important invariant:

```
zero-initialized object
        ↓
does not accidentally represent a valid semantic value
```

Same conceptual rule for IDs:

```
ID.value == 0
```

represents invalid/unassigned.

---

### 8. Public constants

Lock down:

```
KESTREL_MAX_TOPOLOGY_NAME  == 128
KESTREL_MAX_VENDOR_NAME    == 64
KESTREL_MAX_IDENTITY_BYTES == 64
```

These are part of the public representation now, so changing them later shouldn't happen accidentally.

---

### 9. Strong ID representation

Construct each ID wrapper and confirm its `.value` behaves as expected:

```
PhysicalDevice ID
ExecutionTarget ID
BackendInstance ID
MemorySpace ID
```

You aren't testing some sophisticated ID system yet.

You're protecting the fact that these are **different semantic types wrapping `uint32_t`**, rather than loose integer soup.

---

### 10. Can the structs represent our ordinary CPU case?

Build values locally on the stack representing:

```
PhysicalDevice P1
    id = 1
    class = CPU

ExecutionTarget E1
    id = 1
    class = CPU

BackendInstance B1
    family = SCALAR
    target = E1

BackendInstance B2
    family = AVX2
    target = E1

MemorySpace M1
    kind = SYSTEM

E1 BACKED_BY P1
E1 ↔ M1
```

You're not asking Kestrel to _validate_ this graph.

You're proving the public vocabulary can express:

> one CPU execution target with multiple backend implementations.

That's an architectural representation test.

---

### 11. Can it represent hierarchy?

Construct:

```
E1 = root target
E2 = child target

E2 SUBTARGET_OF E1
```

Again, no cycle detection, no graph traversal.

Just prove:

```
child_target_id
parent_target_id
```

can encode the relationship correctly.

---

### 12. Can it represent multi-physical backing?

Given what we literally just discussed, this is worth having.

Construct:

```
P1
P2
E3

E3 BACKED_BY P1
E3 BACKED_BY P2
```

That protects the architectural reason `BACKED_BY` is a separate relation instead of:

```
ExecutionTarget.physical_device_id
```

This is a very nice regression test because if somebody later tries to "simplify" the topology model, this test documents why that's wrong.

---

### 13. Identity value representation

Construct something like:

```
IdentityValue
    kind   = CUDA_DEVICE_UUID
    length = 16
    bytes  = some known 16 bytes
```

and verify the fields preserve what you assigned.

But **do not test reconciliation**.

We're only testing:

> `kestrel_identity_value_t` can carry typed identity evidence.

Not:

```
CUDA UUID ↔ Vulkan UUID → MATCH
```

That's later.

---

And that's it.

Your Slice 1 test file should feel almost disappointingly simple:

```
test_topology.c

Lifecycle
    create success
    create NULL output
    create already-live
    destroy live
    destroy NULL pointer
    destroy pointer-to-NULL
    destroy twice
    topology destroy ZERO

Representation
    INVALID == 0
    constants
    strong IDs
    CPU + Scalar/AVX2 model
    BACKED_BY
    SUBTARGET_OF
    multi-physical backing
    memory relation
    identity value
```

### Explicitly do **not** wander into

```
MIG reconciliation
CUDA/Vulkan matching
PCI locator semantics
identity namespace comparison
duplicate relation detection
cycle detection
dynamic arrays
observation registration
finalization
query APIs
allocation failure injection
```