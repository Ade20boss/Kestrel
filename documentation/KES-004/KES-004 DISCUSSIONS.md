
The whole purpose of KES-004 is to teach Kestrel **what machine it is running on**.

Not how fast that machine is.  
Not which operation should run where.  
Not whether a backend supports BF16 or matmul.

Just:

> **What hardware exists, what execution systems exist, what memory exists, and how they are connected.**

That is the mental model for everything below.

---

# 1. Why are we even building runtime topology?

Originally Kestrel had something simplistic like:

```
DEV_CPU
DEV_CUDA
```

The problem is that this mixes two completely different concepts.

A CPU is physical hardware.

CUDA is not hardware.

CUDA is a software execution platform/API for NVIDIA GPUs.

So this:

```
CPU
CUDA
```

is conceptually like comparing:

```
Car
Automatic transmission
```

They're not the same category.

KES-004 fixes that.

We now separate:

```
DEVICE
physical hardware

BACKEND FAMILY
execution technology

BACKEND INSTANCE
one backend attached to one actual device

MEMORY SPACE
where bytes physically live
```

That distinction is the foundation of the new Kestrel runtime.

---

# 2. What exactly is a DEVICE?

A device is physical compute hardware.

Examples:

```
Intel Core i5
Intel Iris Xe
NVIDIA T4
```

Those are things that physically exist in the machine.

So Kestrel can say:

```
Device 0 = Intel CPU
Device 1 = Intel Iris Xe
Device 2 = NVIDIA T4
```

But Kestrel must never say:

```
Device = CUDA
```

because CUDA is software, not hardware.

---

# 3. Device classes

We decided that devices belong to broad classes:

```
CPU
GPU
ACCELERATOR
UNKNOWN
```

That is intentionally broad.

For example:

```
Intel Core i5
→ CPU

Intel Iris Xe
→ GPU

NVIDIA T4
→ GPU
```

`ACCELERATOR` exists for hardware that isn't cleanly a CPU or general GPU.

For example, future NPUs, TPUs, AI accelerators, DSP-style compute hardware, etc., might fit there.

`UNKNOWN` lets Kestrel represent hardware it cannot classify properly instead of lying.

---

# 4. Why don't we have IGPU and DGPU classes?

This was the part you asked about before.

We decided:

```
Intel Iris Xe
class = GPU

NVIDIA T4
class = GPU
```

They're both GPUs.

What makes them different is mainly **their memory relationship**.

An integrated GPU might use system RAM:

```
Iris Xe
   ↕
System RAM
```

A discrete GPU often has its own VRAM:

```
NVIDIA T4
   ↕
T4 VRAM
```

So we don't need:

```
IGPU
DGPU
```

as fundamental device kinds.

Instead:

> **Device class says what kind of compute hardware it is. Memory topology says how that device is connected to memory.**

This is more flexible long-term.

---

# 5. What is a device ID?

Every discovered device gets an ID for the current runtime.

Example:

```
Device 0 = CPU
Device 1 = Iris Xe
Device 2 = NVIDIA T4
```

That ID is only guaranteed to be meaningful while that Kestrel runtime instance exists.

If you close the program and start it tomorrow:

```
Device 1
```

is not guaranteed to be the Iris Xe again.

So:

```
runtime-local ID
```

is not the same thing as:

```
persistent hardware identity
```

Those are different.

Persistent performance profiles later need stronger identity information.

---

# 6. What information belongs inside a device?

A device should contain basic hardware identity information.

Things like:

```
ID
class
vendor
name
hardware identity when available
```

Example:

```
id = 1
class = GPU
vendor = Intel
name = Intel Iris Xe Graphics
```

That's fine.

But things like:

```
supports FP16
supports matmul
supports negative strides
CUDA version
Vulkan limits
```

do **not** belong inside the device.

Why?

Because those are not pure properties of the hardware.

They often depend on the **backend** used to access that hardware.

---

# 7. What is a BACKEND FAMILY?

A backend family is a method/API/implementation Kestrel can use to execute operations.

We currently have:

```
SCALAR
AVX2
VULKAN
CUDA
```

Meaning:

```
SCALAR
→ basic portable CPU execution

AVX2
→ SIMD CPU execution

VULKAN
→ cross-vendor GPU compute

CUDA
→ NVIDIA-specific GPU compute
```

These are software execution mechanisms.

So remember:

```
CPU
GPU
```

are device classes.

While:

```
SCALAR
AVX2
VULKAN
CUDA
```

are backend families.

---

# 8. What is a BACKEND INSTANCE?

This is probably the most important concept in KES-004.

A backend family by itself isn't enough.

Take:

```
VULKAN
```

Suppose your machine has two GPUs:

```
Intel Iris Xe
NVIDIA T4
```

Then Vulkan could potentially run on both.

So you don't have one generic "Vulkan".

You have:

```
Vulkan + Iris Xe

Vulkan + NVIDIA T4
```

Those are two separate backend instances.

So the rule is:

> **Backend instance = backend family + one specific physical device.**

Example:

```
Backend instance 0
SCALAR + CPU

Backend instance 1
AVX2 + CPU

Backend instance 2
VULKAN + Iris Xe

Backend instance 3
VULKAN + NVIDIA T4

Backend instance 4
CUDA + NVIDIA T4
```

---

# 9. Why do we need backend instances instead of just backend families?

Because:

```
CUDA
```

does not tell you which GPU.

Imagine a machine with:

```
NVIDIA GPU 0
NVIDIA GPU 1
```

Both support CUDA.

Then you need:

```
CUDA + GPU 0
CUDA + GPU 1
```

Those are different execution targets.

Likewise:

```
Vulkan + GPU A
Vulkan + GPU B
```

must be separate.

So the planner later chooses:

```
backend instance
```

not merely:

```
backend family
```

That is a major architectural distinction.

---

# 10. Backend instances also get runtime IDs

Just like devices:

```
backend_instance_id
```

Example:

```
0 = Scalar + CPU
1 = AVX2 + CPU
2 = Vulkan + Iris Xe
3 = CUDA + T4
```

Again, those IDs exist to identify runtime objects during one Kestrel process.

---

# 11. What is a MEMORY SPACE?

A memory space answers:

> **Where do bytes physically live?**

Examples:

```
System RAM
NVIDIA T4 VRAM
RTX 4090 VRAM
```

This is not an allocator.

It is not:

```
malloc
cudaMalloc
VkBuffer
```

Those are mechanisms for obtaining memory.

A memory space is the physical place.

---

# 12. Memory-space kinds

We currently freeze:

```
SYSTEM
DEVICE_LOCAL
UNKNOWN
```

Meaning:

### SYSTEM

General machine RAM.

Example:

```
DDR4 / DDR5
```

### DEVICE_LOCAL

Memory physically local to a device.

Example:

```
GPU VRAM
```

### UNKNOWN

Something Kestrel cannot classify confidently.

This makes the model extensible.

---

# 13. Why don't we have CUDA memory or Vulkan memory?

Because:

```
CUDA
Vulkan
```

are backends.

They are not physical memory.

Suppose an NVIDIA GPU has:

```
24 GB VRAM
```

CUDA might access that memory.

Vulkan might also access that same physical device memory.

But physically, it's still:

```
NVIDIA GPU VRAM
```

not:

```
CUDA memory
```

or:

```
Vulkan memory
```

Backend-specific allocation and resource compatibility comes later.

---

# 14. Memory spaces are actual instances, not just categories

Suppose you have:

```
GPU A
8 GB VRAM

GPU B
24 GB VRAM
```

We do not say:

```
DEVICE_LOCAL = 32 GB
```

because that's not one memory space.

You have:

```
Memory space 1
DEVICE_LOCAL
8 GB
associated with GPU A

Memory space 2
DEVICE_LOCAL
24 GB
associated with GPU B
```

That's important because bytes stored on GPU A are not automatically bytes stored on GPU B.

---

# 15. Integrated GPU example

Your laptop gives the cleanest example.

You might have:

```
Device 0
Intel CPU

Device 1
Intel Iris Xe

Memory Space 0
System RAM
```

Both CPU and iGPU are related to the same system memory.

Now compare with:

```
Device 0
CPU

Device 1
NVIDIA GPU

Memory Space 0
System RAM

Memory Space 1
NVIDIA VRAM
```

Now the discrete GPU has another local memory region.

That's why memory topology is useful.

---

# 16. Device ↔ memory relationships

We decided to describe the physical relationship using:

```
LOCAL
SHARED
REMOTE
```

This is a topology relationship only.

### LOCAL

The memory belongs to/is physically local to the device.

Example:

```
NVIDIA T4
   ↓ LOCAL
T4 VRAM
```

### SHARED

The device shares that physical memory.

Example:

```
CPU
   ↓ SHARED
System RAM

Iris Xe
   ↓ SHARED
System RAM
```

### REMOTE

The memory exists elsewhere from that device's perspective.

Example:

```
CPU
   ↓ REMOTE
T4 VRAM
```

or:

```
T4
   ↓ REMOTE
System RAM
```

---

# 17. Very important: REMOTE does NOT mean inaccessible

This is subtle.

If Kestrel says:

```
T4 → System RAM = REMOTE
```

that does **not** mean:

```
T4 can never access system RAM
```

It only means:

> physically, that memory is not local/shared in the same sense as its own VRAM.

Whether CUDA or Vulkan can actually work with that memory is a **backend capability question**.

This distinction prevents topology from becoming polluted with backend semantics.

---

# 18. Topology vs backend memory compatibility

Suppose:

```
NVIDIA GPU
     ↕
VRAM
```

Topology says:

```
this VRAM belongs to that GPU
```

That's all.

Then later the backend capability layer can answer:

```
Can CUDA allocate there?
Can Vulkan allocate there?
Can Vulkan import CUDA-created memory?
Can CUDA import Vulkan-created memory?
Can a backend execute directly from it?
```

Those are different questions.

So don't mix:

```
where memory physically exists
```

with:

```
what software API can do with it
```

---

# 19. What is the runtime topology registry?

All these objects need one owner.

So Kestrel gets something conceptually like:

```
runtime_topology
├── devices[]
├── backend_instances[]
├── memory_spaces[]
└── relationships[]
```

This is Kestrel's authoritative model of the machine.

Later systems use this.

For example:

```
Storage
→ references memory space

Capability
→ references backend instance

Planner
→ queries all of it
```

---

# 20. Who owns all these objects?

The topology owns them.

So a user should not individually create/destroy:

```
device
memory_space
backend_instance
```

Instead:

```
topology_init()
```

discovers/creates everything.

Then:

```
topology_destroy()
```

destroys everything.

Why?

Because otherwise you get dangerous situations like:

```
backend_instance points to device
device gets destroyed
backend_instance still exists
```

Now you have a dangling relationship.

Central ownership avoids that.

---

# 21. Why IDs instead of pointers?

Suppose a backend instance needs to identify its device.

We prefer:

```
backend_instance.device_id
```

instead of exposing:

```
device *
```

as the semantic identity.

Why?

IDs are:

```
easy to print
easy to debug
stable across array moves
easy to serialize later
useful for profiling keys
not tied to memory addresses
```

Pointers can still be used internally for convenience, but identity should be based on IDs.

---

# 22. What is discovery?

Discovery is simply:

> Kestrel looks at the current machine and builds the topology.

Conceptually:

```
start runtime

↓
discover CPU

↓
create Scalar + CPU

↓
detect AVX2

↓
if AVX2 exists:
    create AVX2 + CPU

↓
discover Vulkan GPUs

↓
create Vulkan backend instances

↓
discover CUDA GPUs

↓
create CUDA backend instances
```

---

# 23. Optional backend failure should not kill Kestrel

Suppose:

```
CUDA library unavailable
```

Kestrel should not say:

```
runtime startup failed
```

If other paths work, it should continue.

For example:

```
Scalar
AVX2
Vulkan
```

may still be available.

This is important because Kestrel is heterogeneous.

One backend failing does not mean the whole system is unusable.

---

# 24. How do we represent CPUs?

For v2:

```
the whole host CPU
→ one Kestrel device
```

Not:

```
core 0 = device
core 1 = device
core 2 = device
```

Cores are execution resources within the CPU backend.

We may eventually care about:

```
NUMA
cache topology
core topology
SMT
```

but not in KES-004.

---

# 25. Scalar backend is always the baseline

On a supported machine, we want:

```
SCALAR + CPU
```

to exist.

Why?

Because Scalar will eventually be the reference implementation.

Meaning:

> If AVX2/Vulkan/CUDA disagree with Scalar, Scalar is our correctness baseline unless proven otherwise.

So Scalar is not merely the slow backend.

It is the semantic reference path.

---

# 26. AVX2 needs runtime detection

We do not say:

```
x86 machine
→ automatically AVX2
```

Because an x86 CPU may not support AVX2.

So:

```
AVX2 backend compiled
```

and:

```
CPU supports AVX2
```

are different facts.

Only if runtime detection says AVX2 is usable should we create:

```
AVX2 + CPU
```

---

# 27. One physical GPU may have multiple backend instances

Example:

```
NVIDIA GPU
├── Vulkan
└── CUDA
```

That is valid.

Because the same hardware may be reachable through several backend APIs.

Again:

```
physical device
```

and:

```
execution mechanism
```

are different layers.

---

# 28. Deduplicating devices across APIs

This one matters later.

Imagine:

```
Vulkan discovers:
NVIDIA T4

CUDA discovers:
NVIDIA T4
```

We ideally want:

```
1 physical GPU
2 backend instances
```

not:

```
2 fake physical GPUs
```

But we only merge them if we have trustworthy hardware identity.

We do **not** do:

```
same name
→ must be same GPU
```

because names can be duplicated.

So the rule is:

```
strong identity available
→ deduplicate

identity uncertain
→ keep them separate
```

This is conservative by design.

---

# 29. Compile-time knowledge vs runtime knowledge

This distinction is important.

Kestrel may know at compile time:

```
Scalar code exists
AVX2 code exists
Vulkan backend exists
CUDA backend exists
```

But that doesn't mean those backends are usable on the current machine.

At runtime Kestrel discovers:

```
actual CPU
actual GPUs
actual AVX2 support
actual memory
actual Vulkan devices
actual CUDA devices
```

So:

```
CUDA backend compiled
```

does NOT imply:

```
CUDA GPU exists
```

---

# 30. The most important boundary: topology vs capability vs performance vs planner

You should memorize this:

```
TOPOLOGY
What exists?

CAPABILITY
What is legal?

PERFORMANCE
How expensive is it?

PLANNER
What should we choose?
```

Let's make that concrete.

Imagine:

```
NVIDIA T4
CUDA backend instance
```

Topology says:

```
T4 exists
CUDA instance exists
T4 has VRAM
```

Capability says:

```
CUDA instance supports FP16 matmul
```

Performance says:

```
1024×1024 FP16 matmul takes X μs
```

Planner says:

```
Use CUDA for this operation.
```

Four different layers.

Do not collapse them.

---

# 31. Why introspection matters

Kestrel should eventually be able to print something like:

```
Devices:
[0] Intel CPU
[1] Intel Iris Xe

Memory:
[0] System RAM

Backend Instances:
[0] Scalar -> CPU
[1] AVX2 -> CPU
[2] Vulkan -> Iris Xe
```

Or:

```
Devices:
[0] CPU
[1] NVIDIA T4

Memory:
[0] System RAM
[1] T4 VRAM

Backend Instances:
[0] Scalar -> CPU
[1] AVX2 -> CPU
[2] Vulkan -> T4
[3] CUDA -> T4
```

Why is this useful?

Because if the planner later chooses something weird, you can inspect the machine model Kestrel is using.

It also helps benchmark tooling and advanced users.

---

# 32. What does NOT belong in KES-004?

Very important.

Do not get excited and start implementing:

```
tensor allocation
storage
CUDA memory allocation
Vulkan buffers
matmul
kernel execution
planner
benchmarks
autodiff
tensor transfers
```

Those are later systems.

KES-004 is only about the machine model.

---

# 33. The final architecture in one example

Imagine a workstation:

```
Intel CPU
Intel iGPU
NVIDIA T4
```

Kestrel topology might look like:

```
DEVICES
-------

Device 0
Intel CPU
class = CPU

Device 1
Intel Iris Xe
class = GPU

Device 2
NVIDIA T4
class = GPU
```

Memory:

```
MEMORY SPACES
-------------

Memory 0
System RAM
kind = SYSTEM

Memory 1
T4 VRAM
kind = DEVICE_LOCAL
```

Relationships:

```
CPU      -> System RAM = SHARED
Iris Xe  -> System RAM = SHARED

T4       -> T4 VRAM    = LOCAL
T4       -> System RAM = REMOTE
CPU      -> T4 VRAM    = REMOTE
```

Backend instances:

```
BACKEND INSTANCES
-----------------

0
SCALAR + CPU

1
AVX2 + CPU

2
VULKAN + Iris Xe

3
VULKAN + T4

4
CUDA + T4
```

Now Kestrel understands the machine.

But it still does **not yet know**:

```
which backend supports FP16
which backend supports matmul
which one is fastest
where a tensor should run
```

Those come later.

---

# 34. Then the later pipeline becomes obvious

Once KES-004 exists:

```
Topology
↓
What hardware exists?
```

Then later:

```
Capability
↓
What can each backend legally do?
```

Then:

```
Performance profile
↓
How fast is each legal option?
```

Then:

```
Planner
↓
Which option should we choose?
```

Then:

```
Execution
```

That exact dependency chain is part of the design freeze.

---

# 35. The 20 rules you should mentally carry into implementation

The source freezes these invariants: devices are physical hardware; backend families are execution technologies; backend instances bind one backend to one device; multiple instances can target one device; memory spaces model physical memory rather than allocators; topology owns object lifetimes; IDs identify topology objects within one runtime; hardware deduplication requires trustworthy identity; topology does not contain operator capabilities, performance measurements, or planner decisions; optional backend discovery failures are recoverable; and Scalar+CPU is the baseline execution instance. Pasted text.txtTXT

If you remember nothing else, remember this:

> **KES-004 is a map of the machine.**

A map tells you:

```
what things exist
where they are
how they're connected
```

A map does not tell you:

```
what route is fastest
which car should drive it
how much fuel it uses
```

That comes later.

So in Kestrel terms:

```
Topology = map
Capabilities = what each vehicle can do
Performance = how fast/costly each route is
Planner = chooses the route
Execution = actually drives
```

That is the full conceptual model you should understand before we turn this into the implementation ticket.