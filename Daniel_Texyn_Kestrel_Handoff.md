# Daniel / Texyn / Kestrel - Comprehensive Handoff

**Status:** Continuity document for future engineering conversations.

## Purpose and How to Use This Handoff

This file is a continuity document for a future assistant, collaborator, mentor, or Daniel himself. It is intentionally detailed. It records the important personal context, project history, architecture decisions, work style, technical choices, open questions, and next actions from the conversation so work can resume without rebuilding context from scratch.

Treat the status labels carefully:
- FROZEN / MERGED / ACCEPTED = do not reopen casually; require a deliberate architectural reason.
- APPROVED DIRECTION = accepted conceptually, but implementation may still be pending.
- PROPOSAL / FUTURE = not yet part of the current implementation contract.
- SUPERSEDED = historical context only; do not implement as current architecture.

Most important operating rule: do not collapse Kestrel into “a tensor runtime.” Kestrel’s top-level product identity is a C neural-network library for training and inference. The heterogeneous runtime is the execution foundation underneath it.

## 1. Identity, Naming, and Conversation Context

Active engineering speaker: Daniel.

In engineering/system conversations, address the user as Daniel. Daniel is sharing Oluyomi’s account, so account-level profile details may refer to Oluyomi while the engineering project ownership and mentorship workflow in this handoff belong to Daniel.

Internal company identity for project tickets: Texyn Labs.
Longer-term company / legacy concept: Texyn Systems.

Daniel wants technical collaboration to feel like working with a senior systems/ML engineer inside a serious engineering organization, not like receiving generic tutorials. He prefers precise architectural reasoning, concrete tickets, explicit acceptance criteria, and PR-style reviews.

Daniel is learning aggressively and is comfortable being challenged. Do not assume he already knows every systems concept. When introducing low-level ideas such as GPU memory topology, numerical representation, Vulkan, backend capability, or compiler/runtime concepts, start from a concrete example, then formalize.

## 2. Daniel - Personal and Working Profile

Daniel is building toward being a systems + machine-learning engineer/founder. His long-term ambition is to understand both sides of modern intelligent systems:
1. intelligence/model side - machine learning, neural networks, mathematical foundations, training;
2. systems/compute side - runtimes, compilers, memory, hardware, performance, distributed systems, execution.

He does not want to become only a high-level framework user. He wants to understand and build the layers underneath.

Technical background / current skill base:
- C and Python are his main languages.
- Some machine-learning experience.
- Existing systems projects include Kestrel and Ghost VM; future major projects include Raptor and Talon.
- He uses Git and wants projects to be strong enough for serious engineering interviews, OSS collaboration, and eventually Texyn.

Learning style:
- Deep explanations are welcome, but jargon without intuition is not.
- He often understands architecture but freezes when an implementation ticket is too large. Break implementation into the smallest concrete truth/invariant without doing the implementation for him.
- He likes to challenge design. Treat architectural pushback as productive, not resistance.
- He prefers one main focus per day rather than splitting the day into many unrelated goals.
- He can become overloaded by simultaneous projects/courses. Prioritization matters.

Interaction rules:
- Do not write completed implementations for his Kestrel tickets unless he explicitly and deliberately revokes the rule.
- Small illustrative snippets or conceptual pseudocode are okay.
- Review his implementation like a PR: exact findings, why they matter, explicit verdict.
- Verdict vocabulary: APPROVED / MERGED / CHANGES REQUESTED / REJECTED.
- One active engineering ticket at a time.
- Avoid drip-feeding review feedback. Give bounded, holistic reviews.
- Avoid overengineering abstractions. Daniel has become increasingly good at challenging unnecessary complexity.

## 3. Mentorship and Engineering Workflow Contract

The standing workflow for Kestrel and related projects is:

1. Concept Notes
   Explain the problem, constraints, vocabulary, and competing mental models.
2. Alternatives
   Compare realistic approaches and state why one is preferred.
3. ADR
   Record the architectural decision and consequences.
4. Implementation Contract / Ticket
   Define scope, invariants, tests, acceptance criteria, and what is out of scope.
5. Daniel Implements
   The assistant should not hand over a completed implementation.
6. PR-style Review
   Review holistically; explain exact correctness/architecture issues.
7. Tests / Benchmarks
   Objective evidence closes the ticket.

Documentation is mandatory for major decisions.

ADR rule:
- If a decision changes, do not silently rewrite history.
- Mark the old ADR superseded and create a replacement/revision.

Architecture rule:
- Tickets should be architecture-forward.
- Avoid knowingly temporary foundations that predictably require rewrites.
- This does not mean architecture can never evolve; it means we should not intentionally defer obvious foundational requirements.

Daniel’s phrase/rule: “later is now.”
Meaning: if a core architectural capability is already known to be required, decide it now. Sequential tickets are fine when there are real dependencies; they are not an excuse to postpone known architecture.

## 4. Texyn Labs / Texyn Systems

Texyn Labs is the internal fictional/company identity used in engineering tickets.

Texyn Systems is Daniel’s longer-term company/legacy concept. It is not merely a label for Kestrel. Kestrel, Ghost, Raptor, and Talon are personal engineering projects and should not automatically be described as Texyn products unless Daniel chooses to productize them.

Texyn Systems thesis:
- specialized intelligence and specialized computing;
- understand the environment, then specialize;
- “General purpose in. Purpose built out.”

Compute-side vision:
A system understands hardware, workload, memory, available accelerators, model size, quantization, threading, local vs remote execution, and other constraints, then specializes execution for the situation.

Kestrel now strongly aligns with the specialized-computation side of this thesis because its runtime is explicitly becoming hardware-aware, topology-aware, capability-aware, and performance-aware.

Daniel plans Strata as a future team/engineering culture test and first foundation for Texyn. Strata is a future concept, not part of the current Kestrel implementation contract.

## 5. Multidisciplinary Excellence Community / Society Idea

Daniel proposed creating a serious multidisciplinary community for people who want to become exceptionally good at their craft. The members do not all need to be in tech; possible fields include engineering, medicine, physiotherapy, law, design, research, writing, finance, entrepreneurship, and the arts.

Core idea:
A society for people who pursue unusually high standards through deliberate practice, accountability, exposure to other disciplines, and consistent output.

Recommended cultural pillars:
- Craft: every member is actively becoming excellent at something concrete.
- Accountability: members state weekly commitments and report what they actually completed.
- Exposure: members teach difficult concepts from their own fields to others.
- Output: the culture rewards shipping, publishing, building, researching, presenting, competing, or otherwise producing evidence of progress.

Suggested weekly format:
- 30 min accountability;
- ~60 min technical/intellectual deep dive by a member;
- ~60 min work review / critique session.

Monthly ideas:
- workshops;
- guest experts;
- public webinars;
- build/research challenges;
- mini conferences;
- debates/case competitions;
- “Explain Your Field” sessions.

Membership should be selective for seriousness, not social status. Better 6-12 committed founding members than a 150-person passive chat.

Possible member application questions:
- What are you trying to become excellent at?
- What have you done toward it in the last six months?
- What are you currently building/studying?
- What do you want to accomplish over the next year?
- What can you teach the community?
- Why do you want accountability?

Culture rule proposed: no empty motivational culture. Prefer “show me what you built / learned / improved / where you are stuck.”

Potential names discussed: The Forge, Axiom Society, The Crucible, Vanguard Society, The Guild, Ascend, Praxis Society, The Praxis Fellowship. “Praxis” was especially aligned because it means putting knowledge/theory into action.

Texyn recruiting angle:
Do not make recruitment the public purpose. Build the community for excellence. Over time it naturally reveals who ships consistently, learns fast, thinks deeply, takes ownership, handles criticism, collaborates, teaches, and is reliable. That makes it a potentially excellent long-term recruiting ground for Texyn engineers/researchers/designers/operators.

## 6. Kestrel - Correct Top-Level Product Identity

This is critical because there was a temporary framing drift in the conversation.

Original project purpose from Daniel’s project spec:
Kestrel is a neural-network library in C with reverse-mode autodiff and interchangeable compute backends, originally scalar + AVX2 + CUDA, benchmarked against cuBLAS.

Correct evolved framing:
Kestrel is a C neural-network library for training and inference, with reverse-mode autodiff, neural-network operators, losses, optimizers, and a heterogeneous execution runtime underneath it. The runtime understands devices, backend instances, memory spaces, storage, tensor layouts, dtype semantics, capabilities, measured performance, and eventually plans execution across Scalar, AVX2, Vulkan, and CUDA.

Do NOT describe Kestrel as merely a general tensor runtime that “might train models later.” Training machine-learning models is a core project requirement.

High-level architecture:
- Neural-network layer
  - layers/models
  - activations
  - losses
  - optimizers
  - training loop
- Autodiff
  - computation graph
  - reverse-mode differentiation
  - gradient accumulation
  - backward functions
- Tensor/numerical layer
  - tensors
  - dtypes
  - broadcasting
  - views/strides
  - operators
- Heterogeneous runtime
  - hardware discovery
  - backend instances
  - memory spaces/storage
  - Scalar
  - AVX2
  - Vulkan
  - CUDA
  - capabilities
  - performance profiles
  - planner

Core runtime thesis:
“Understand the machine, then specialize execution for it.”

## 7. Original Kestrel Commitments That Must Remain True

The original Kestrel spec committed to more than tensor kernels. These requirements remain part of the top-level product, although the architecture underneath them has evolved.

Training / autodiff requirements:
- graph-based reverse-mode autodiff;
- forward pass builds graph as a side effect of ops;
- backward(loss) topologically orders the graph and traverses in reverse;
- gradients accumulate with += rather than assignment;
- explicit test for diamond graph y = x*x + x;
- finite-difference code retained as a gradient-check oracle rather than the training method.

Initial NN/operator requirements:
- matmul;
- bias add;
- ReLU;
- sigmoid;
- tanh;
- softmax;
- MSE;
- cross-entropy.

Initial optimizer requirements:
- SGD;
- SGD + momentum;
- Adam.

Initial demonstration / correctness target:
- MNIST 784 -> 256 -> 10;
- ReLU, softmax, cross-entropy;
- target accuracy from original spec: >=97% test accuracy;
- below ~95% should be treated as likely bug rather than “small model.”

Gradient checking:
- analytical gradients compared against central-difference numerical gradients;
- original target relative error <1e-4 for every op.

Backend agreement / performance:
- Scalar is the semantic/correctness reference;
- AVX2 and GPU backends must agree with reference within defined tolerances;
- matmul benchmarks across multiple sizes;
- CUDA benchmarked honestly against cuBLAS;
- GPU kernel time and transfer time measured separately;
- end-to-end GPU MNIST training should match CPU accuracy closely (original spec target within ~0.5%).

The evolved runtime must support these goals rather than replacing them.

## 8. Kestrel Architectural Evolution - What Changed

The original design was intentionally simpler:
- tensor contained raw float *data;
- tensor carried device tag DEV_CPU / DEV_CUDA;
- scalar/AVX2/CUDA were treated more like direct dispatch targets;
- AVX2 row padding was close to a universal tensor-layout assumption;
- a device arena mirroring host arena semantics was proposed.

These are now SUPERSEDED by the v2 architecture.

Why the redesign happened:
The project grew from “a neural-network library with multiple kernels” into a more serious heterogeneous ML execution system. The old model could not correctly describe:
- one physical GPU exposed through both Vulkan and CUDA;
- integrated GPU sharing system RAM;
- separate storage identity from tensor semantics;
- non-host memory that is not a float pointer;
- backend-specific layout requirements;
- multiple GPUs;
- planner decisions that include transfers, synchronization, repacking, and launch cost;
- per-device/per-backend capabilities and performance profiles.

Key replacements:
- DEV_CPU / DEV_CUDA -> Device + Backend Family + Backend Instance.
- float *data inside tensor -> storage object + tensor descriptor/view.
- universal AVX2 row padding -> backend-specific preference/requirement, not global tensor semantics.
- “GPU memory = device arena with host-arena interface” -> allocator/provider model per memory space; GPU allocators need not copy mark/pop semantics.

## 9. KES-001 - Arena Lifecycle and Failure Contract [MERGED]

Status: APPROVED / MERGED.
Suggested historical commit label: KES-001: freeze arena lifecycle and failure contract.

arena_t fields:
- uint8_t *data
- size_t capacity
- size_t offset

State model:
- ZERO: data=NULL, capacity=0, offset=0
- LIVE: data!=NULL, capacity>0, offset<=capacity
- INVALID: anything else

Caller zero-initializes before init. No initialized flag.

Frozen behavior:
- init(NULL) -> recoverable false;
- init accepts exact ZERO;
- init on LIVE/INVALID is programming misuse: assert in debug, false in release, object untouched;
- failed init is transactional;
- destroy(NULL/ZERO) no-op;
- destroy(LIVE) -> exact ZERO;
- destroy(INVALID) -> programming violation; safe no-op in release;
- allocation valid only on LIVE;
- allocation validates size, alignment, power-of-two, alignment<=64, overflow;
- offset changes only after successful allocation;
- mark = current offset;
- pop_to_mark accepts mark <= current offset;
- equal mark is a no-op;
- stale/future mark > current must never move offset forward; assert debug, ignore release;
- reset(LIVE) -> offset=0 while preserving backing storage;
- get_mark invalid misuse: release returns 0 defensively, but 0 is also a legitimate mark;
- assertions are diagnostics, not the only safety layer;
- arena base/allocation alignment = 64 bytes;
- this alignment does not imply tensor row alignment.

Lifetime model:
- no individual free;
- lifetime ends only through pop/reset/destroy;
- ordinary arena-backed tensor views do not need refcounting because base + views die together when backing allocation is invalidated;
- reset/pop do not zero memory by default;
- optional debug poisoning can exist later;
- a subsystem may pop/reset only an arena region whose affected allocations it exclusively controls;
- runtime-private scratch can use marks;
- runtime must never secretly pop developer-controlled arena allocations.

## 10. KES-002 - Heterogeneous Runtime Architecture Freeze [ACCEPTED]

Status: architectural freeze accepted; it blocks and guides subsequent runtime/tensor/backend work.

Kestrel runtime thesis:
A heterogeneous ML execution runtime in C that discovers available compute hardware and memory topology, understands backend capabilities and storage representations, and plans tensor operations for efficient execution across available devices.

Seven core concepts:
1. DEVICE - physical compute hardware.
2. BACKEND / BACKEND FAMILY - execution implementation/API.
3. BACKEND INSTANCE - one backend family attached to one physical device.
4. MEMORY SPACE - where bytes physically live.
5. ALLOCATOR / ARENA / POOL - mechanism managing a chunk in one memory space.
6. STORAGE - one concrete physical allocation.
7. TENSOR - logical interpretation/view over storage.
8. PLANNER - chooses a legal and efficient execution strategy. (Planner is often listed separately from the seven physical/data concepts.)

Examples:
- Scalar + CPU
- AVX2 + CPU
- Vulkan + Intel iGPU
- Vulkan + NVIDIA GPU
- CUDA + NVIDIA GPU

Planner eventually chooses backend INSTANCES, not generic labels such as “GPU.”

Capability belongs substantially to device+backend pair. Performance profile answers “how well”; capability answers “can it.”

## 11. Memory Spaces, Allocators, and Storage - Frozen Architecture

Memory space:
- describes where bytes physically live;
- examples: system/shared RAM, NVIDIA-local VRAM;
- identity/classification/capacity/device relationships belong here;
- physical accessibility is NOT the same thing as backend resource compatibility.

Allocator:
- manages bytes inside exactly one memory space;
- different allocators/providers may target the same physical memory space;
- examples:
  - system RAM -> host arena;
  - system RAM -> Vulkan host-visible pool;
  - NVIDIA VRAM -> CUDA allocator/pool;
  - NVIDIA VRAM -> Vulkan allocator/pool.

Allocator responsibility:
- bytes;
- alignment;
- allocation mechanics;
- provider-specific native details;
- lifetime mechanics.

Allocator must NOT know:
- tensor shape;
- dtype;
- strides;
- operation;
- planner;
- backend-specific tensor-layout meaning.

Avoid “allocation domain” as a public first-class term; Daniel found it confusing. Provider/mechanism is okay in prose.

KES-001 arena remains a concrete host allocator. GPU allocators do not have to imitate mark/pop.

storage_t conceptual contract:
- byte_size;
- memory_space;
- allocator/provider;
- opaque allocation token.

Opaque token rule:
The provider interprets native handles/pointers. Core storage must NOT become a union of host_ptr / cuda_ptr / VkBuffer / etc.

Storage does NOT know:
- tensor shape;
- dtype;
- strides;
- list of tensors referencing it;
- backend operation;
- planner.

Relationship is one-way: tensor -> storage.

No storage offset field for tensor views; tensor owns logical offset_bytes. Provider-internal allocator suballocation details stay provider-internal unless future requirements prove otherwise.

## 12. Tensor and Layout Model - Frozen Architecture

Conceptual tensor descriptor:
- ndim
- shape[]
- stride_bytes[]
- offset_bytes
- dtype
- storage

MAX_DIMS = 6 retained.

Tensor must not know:
- device directly;
- backend directly;
- native pointer type;
- Vulkan/CUDA handles;
- allocator internals;
- planner decision.

Shape is logical only. Padding never changes logical shape.

Views:
- transpose/slice/reverse/broadcast should share storage where representable;
- no payload copy merely to make a view;
- view semantics are logical metadata over storage.

Strides are BYTES, not elements.
Ordinary users should not manually calculate them; constructors/view/planner code does.

Address mapping:
byte_position = offset_bytes + sum(index[d] * stride_bytes[d])

Negative strides:
- supported for zero-copy reverse/flip;
- validation must compute both lowest and highest reachable byte positions;
- example FP32 [a b c d e], offset=16, stride=-4 -> e,d,c,b,a;
- an offset/shape/stride combination that reaches before storage byte 0 is invalid.

Zero strides:
- supported for broadcasting;
- means advancing a logical index does not move in storage;
- introduces aliasing: many logical coordinates may reference the same physical bytes;
- write semantics must account for this aliasing.

Contiguity:
Global Kestrel definition = meaningful tensor values tightly packed in normal row-major order with no gaps.
- no padding;
- no reversal;
- no broadcast overlap;
- no permutation;
- size-1 dimensions are ignored for contiguity checking because they never advance.

Non-contiguous does not mean invalid or necessarily slow.
Backend-preferred layout is separate from global contiguity.

Logical count vs physical region:
For shape=[2,5], element-strides conceptually [8,1]:
- logical element count = 10;
- physical positions touched = 0..4 and 8..12;
- highest touched = 12;
- minimum backing region = 13 positions;
- a full two padded rows might allocate 16 positions.
Keep logical count, reachable byte range, and actual allocation size distinct.

## 13. Rejected / Superseded Kestrel Runtime Designs

Do not reintroduce these without a new ADR:
- universal AVX2 row padding;
- raw float *data inside tensor;
- device stored directly in tensor;
- backend-aware allocator that understands tensor layout;
- storage tracking the list of tensors that reference it;
- runtime blindly using marks in a shared developer arena;
- default zeroing of invalidated arena memory;
- DEV_CPU / DEV_CUDA as a unified “device” enum;
- CUDA_MEMORY or VULKAN_MEMORY as fundamental physical memory-space types;
- treating backend family alone as an executable target.

## 14. KES-003 - Core Dtype and Numeric Semantics [MERGED]

Status: APPROVED / MERGED.

Purpose:
Give Kestrel a global numerical type system that defines what values are, how much storage they use, how types combine, how accumulation widens, and what global overflow semantics later kernels must obey.

Core dtype set:
- BOOL
- FP16, BF16, FP32, FP64
- INT8, UINT8
- INT16, UINT16
- INT32, UINT32
- INT64, UINT64
- INVALID sentinel
- COUNT terminal sentinel (not a dtype)

kestrel_core.h owns primitive dtype identity (kestrel_dtype_t).
dtype.h / dtype.c own behavior.

INVALID = 0 for safe zero-initialized state.
COUNT follows the last real dtype automatically; do not hard-code stale numeric values.

Classification helpers approved as tiny static inline functions:
- is_valid
- is_float
- is_integer
- is_bool
- is_signed_integer
- is_unsigned_integer

Do not expose ambiguous is_signed() because FP types can represent negative values but signed/unsigned classification is intentionally integer-specific.

Size mapping:
- BOOL 1
- FP16/BF16 2
- FP32 4
- FP64 8
- INT8/UINT8 1
- INT16/UINT16 2
- INT32/UINT32 4
- INT64/UINT64 8

Do not duplicate integer width metadata; integer bit width can be derived from dtype_size * 8.

## 15. Frozen Dtype Promotion Rules

PROMOTION answers: what common dtype should operands use?
It does not guarantee the arithmetic result cannot overflow.

BOOL:
- does not participate in numeric promotion;
- BOOL+BOOL numeric promotion invalid;
- BOOL + numeric invalid;
- logical operations/comparisons/masks use BOOL;
- conversion to/from numeric should be explicit where defined later.

Float + float:
- FP16 + FP16 -> FP16
- BF16 + BF16 -> BF16
- FP32 + FP32 -> FP32
- FP64 + FP64 -> FP64
- FP16 + BF16 -> FP32
- FP16/BF16 + FP32 -> FP32
- any mixed float pair involving FP64 -> FP64

Integer-only promotion uses width + signedness.
Same signedness -> wider same-signedness type.

Mixed signed/unsigned:
- if signed width > unsigned width -> signed type;
- equal width -> next wider signed type if one exists;
  - INT8+UINT8 -> INT16
  - INT16+UINT16 -> INT32
  - INT32+UINT32 -> INT64
- unsigned wider -> choose a signed type wider than the unsigned type if available;
  - INT8+UINT16 -> INT32
  - INT16+UINT32 -> INT64
- if no wider signed type exists -> implicit promotion INVALID;
  - INT64+UINT64 -> INVALID
  - INT32+UINT64 -> INVALID
  - INT16+UINT64 -> INVALID
  - INT8+UINT64 -> INVALID

Integer + floating-point policy:
Choose the smallest supported float that can exactly represent the full integer input domain while being at least as capable as the existing float operand.

8-bit integer + float:
- INT8/UINT8 + FP16 -> FP16
- + BF16 -> BF16
- + FP32 -> FP32
- + FP64 -> FP64

16-bit integer + float:
- + FP16/BF16 -> FP32
- + FP32 -> FP32
- + FP64 -> FP64

32-bit integer + float:
- + FP16/BF16/FP32 -> FP64
- + FP64 -> FP64

64-bit integer + any supported float -> INVALID implicit promotion because FP64 cannot exactly represent the full INT64/UINT64 domain.
Explicit cast required.

Promotion invariant:
promote(A,B) == promote(B,A) for all legal pairs; invalidity must also be symmetric.

## 16. Frozen Accumulation and Overflow Rules

ACCUMULATION answers: what dtype should repeated arithmetic build intermediate totals into?
It is separate from promotion.

Floating accumulation:
- FP16 -> FP32
- BF16 -> FP32
- FP32 -> FP32
- FP64 -> FP64

Signed integer accumulation:
- INT8 -> INT32
- INT16 -> INT64
- INT32 -> INT64
- INT64 -> INT64

Unsigned integer accumulation:
- UINT8 -> UINT32
- UINT16 -> UINT64
- UINT32 -> UINT64
- UINT64 -> UINT64

BOOL has no generic numeric accumulation dtype.

Mixed-input accumulation rule:
1. promote operands;
2. take accumulation dtype of the promoted dtype.
Example: INT8+UINT8 -> INT16 promotion -> INT64 accumulation.

Default output rule discussed/frozen for accumulation-heavy operations:
- ordinary arithmetic output = promoted dtype;
- accumulation-heavy op default output = accumulation dtype;
- explicit downcast if a narrower output is desired.

OVERFLOW is a third separate concept.

Floating overflow:
- IEEE-style +/-Inf and NaN semantics where appropriate;
- no default saturation.

Integer overflow:
- deterministic N-bit modular wraparound;
- do NOT rely on C signed-overflow undefined behavior;
- examples: INT8 127+1 -> -128, UINT8 255+1 -> 0.

Accumulator overflow follows the accumulator dtype’s normal overflow semantics.
Widening reduces risk; it never guarantees overflow is impossible.
Saturating arithmetic, if needed for quantization later, must be explicit specialized semantics rather than default arithmetic.

## 17. KES-003 Implementation Review Outcome and Tests

Daniel implemented KES-003 rather than receiving completed code.

Notable review history:
- COUNT was initially hard-coded incorrectly and collided with INT32; corrected to automatic terminal sentinel.
- size_t header dependency was corrected.
- several switch defaults originally evaluated false without returning; corrected.
- is_signed/is_unsigned renamed to integer-specific names.
- validity was moved into the static-inline classification layer (approved).
- separate integer-width metadata was removed because dtype_size already provides width.
- promotion initially checked equality before validity/BOOL and could incorrectly return BOOL/COUNT for equal invalid inputs; corrected by validating/rejecting BOOL before same-type fast path.

Promotion algorithm approved:
- validate inputs;
- reject BOOL;
- same-type fast path;
- normalize operand ordering via swap:
  - float first for float+integer;
  - signed first for signed+unsigned;
- float/float branch;
- float/integer branch by integer byte size;
- integer/integer branch by size and signedness.

Test strategy approved:
- validity including positive/negative garbage enum values;
- complete size coverage;
- complete category coverage;
- 13x13 real-dtype promotion symmetry matrix (169 pairs);
- explicit float promotion matrix;
- integer promotion rule boundaries;
- mixed int/float width classes;
- complete accumulation mapping;
- invalid/BOOL/COUNT cases.

Final verdict: KES-003 APPROVED / MERGED.

## 18. KES-004 Runtime Topology - Design Freeze [CURRENT / PRE-IMPLEMENTATION]

Current active architecture topic. KES-004 has NOT yet been issued as the final implementation ticket in this conversation. Daniel asked for a full conceptual explanation first.

Purpose:
Represent the physical compute resources and memory topology Kestrel sees at runtime without mixing hardware identity with execution APIs.

KES-004 answers:
- What physical devices exist?
- What backend implementations are available?
- Which backend instance targets which device?
- What physical memory spaces exist?
- How are those objects related?

KES-004 does NOT answer:
- Can this backend run matmul?
- Does it support BF16?
- Does it support negative strides?
- How fast is it?
- Which backend should be chosen?
Those belong to capability, performance, and planner layers.

Core mnemonic:
Topology = map of the machine.
Capability = what is legal.
Performance = what it costs.
Planner = what we choose.
Execution = actually run it.

## 19. KES-004 Device Model - Frozen Decisions

DEVICE = actual physical compute hardware.
Examples:
- Intel Core i5 CPU
- Intel Iris Xe GPU
- NVIDIA T4 GPU

CUDA, Vulkan, AVX2, Scalar are NOT devices.

Device classes:
- CPU
- GPU
- ACCELERATOR
- UNKNOWN

Deliberate decision:
Do NOT create separate fundamental IGPU / DGPU classes.
Both integrated and discrete GPUs are class GPU. Their meaningful differences emerge from memory topology:
- iGPU may share system RAM;
- dGPU may have device-local VRAM.
This is more future-proof for unified/heterogeneous memory systems.

Device runtime-local ID:
- stable only for the lifetime of one Kestrel runtime instance;
- not guaranteed to persist across process restarts;
- persistent profiling later needs stronger hardware identity.

Device metadata may include:
- runtime ID;
- broad device class;
- human-readable name;
- vendor;
- trustworthy hardware identity data when available.

Do NOT put in the device object:
- supports_matmul;
- supports_FP16;
- backend-specific limits;
- performance numbers;
- planner decisions.
Those belong elsewhere.

CPU representation for v2:
Treat the host CPU as one Kestrel physical device, not one device per core. Cores/SMT/cache/NUMA are backend/internal/future topology sophistication.

## 20. KES-004 Backend Family and Backend Instance

Backend family = execution implementation/API.
Frozen families:
- SCALAR
- AVX2
- VULKAN
- CUDA

Meanings:
- Scalar = portable CPU reference implementation;
- AVX2 = x86 SIMD CPU implementation;
- Vulkan = cross-vendor GPU compute backend;
- CUDA = NVIDIA-specific GPU compute backend.

Backend instance = one backend family attached to exactly one physical device.
Conceptual fields:
- runtime-local backend_instance_id;
- backend_family;
- device_id.

Examples:
- Scalar + CPU
- AVX2 + CPU
- Vulkan + Intel Iris Xe
- Vulkan + NVIDIA T4
- CUDA + NVIDIA T4

Multiple instances of the same backend family are valid:
- Vulkan + GPU A
- Vulkan + GPU B
- CUDA + GPU 0
- CUDA + GPU 1

Multiple backend families can target the same device:
- one NVIDIA GPU may expose both Vulkan and CUDA instances.

Planner later chooses a backend INSTANCE, not “GPU” and not just “CUDA.”

Scalar baseline:
Every supported Kestrel machine should have Scalar + host CPU unless initialization fundamentally fails. Scalar is the semantic/correctness reference, not merely a slow fallback.

AVX2 discovery:
Only create AVX2 + CPU if runtime CPU capability supports it. “Compiled on x86” does not imply AVX2 is available.

## 21. KES-004 Memory-Space Model

Memory space = where bytes physically live.
It is NOT an allocator.

Examples:
- system RAM;
- NVIDIA T4 VRAM;
- RTX 4090 VRAM.

Memory-space kinds:
- SYSTEM
- DEVICE_LOCAL
- UNKNOWN

Do not create CUDA_MEMORY or VULKAN_MEMORY as physical kinds. CUDA/Vulkan are APIs/backends, not places where bytes physically live.

Memory spaces are instances:
Two GPUs with 8 GB and 24 GB VRAM are two separate DEVICE_LOCAL memory spaces, not one 32 GB pool.

Device <-> memory-space physical relationship:
- LOCAL: physically belongs/is local to the device (e.g. T4 -> T4 VRAM);
- SHARED: physically shared with device (e.g. CPU -> system RAM; iGPU -> system RAM);
- REMOTE: exists elsewhere from device perspective (e.g. T4 -> system RAM, CPU -> T4 VRAM).

Important:
REMOTE does not mean “backend can never access it.” It only describes physical topology. Backend capability later decides allocation, direct execution, transfer, import/export, etc.

Physical topology and backend/resource compatibility are separate concepts.

## 22. KES-004 Topology Registry, Ownership, and IDs

Conceptual authoritative registry:
runtime_topology
- devices[]
- backend_instances[]
- memory_spaces[]
- relationships[]

Runtime topology owns all discovered topology objects.
Users/subsystems do not independently free devices/memory spaces/backend instances.

Lifecycle:
- topology_init -> discover/build registry;
- topology_destroy -> release topology metadata.

Reason:
Central ownership prevents dangling relationships such as a backend instance referring to a destroyed device descriptor.

Persistent relationships inside topology should use runtime-local IDs rather than exposed raw pointers as identity.
Example:
backend_instance.device_id
rather than making device_t* itself the public semantic identity.

Benefits:
- easier debugging/introspection;
- stable if arrays move internally;
- easier later serialization/profile keys;
- avoids memory address being mistaken for identity.

Internal code may still resolve IDs efficiently to objects.

## 23. KES-004 Discovery and Deduplication Principles

Conceptual discovery pipeline:
1. initialize topology;
2. discover host CPU;
3. create Scalar+CPU;
4. detect AVX2; if legal, create AVX2+CPU;
5. discover Vulkan physical devices; create Vulkan backend instances;
6. discover CUDA devices when CUDA support is built/available; create CUDA backend instances.

Optional backend failure is recoverable.
Example: no CUDA runtime/device must not prevent Scalar/AVX2/Vulkan from operating.

Compile-time vs runtime distinction:
Compile-time knowledge: Kestrel build may contain Scalar/AVX2/Vulkan/CUDA backend families.
Runtime knowledge: actual CPU, AVX2 availability, GPUs, memory capacities, backend instances, hardware identities.
“CUDA backend compiled” != “CUDA device exists.”

Physical-device deduplication across APIs:
If Vulkan discovers NVIDIA T4 and CUDA discovers the same NVIDIA T4, Kestrel should ideally represent:
- one physical device;
- two backend instances.

BUT:
- do not merge devices merely because names match;
- strong/trustworthy identity -> deduplicate;
- ambiguous identity -> keep separate rather than falsely merging.

Conservative duplication is safer than false identity.

## 24. KES-004 Boundary Between Topology, Capability, Performance, Planner

This is one of the most important Kestrel architectural boundaries.

TOPOLOGY answers:
- what exists?
- where is physical memory?
- how are physical resources related?
- which backend instance targets which physical device?

CAPABILITY answers:
- what operations can this backend instance execute?
- which dtypes?
- which layouts/stride conditions?
- which storage/providers?
- what hard alignment/resource constraints?

PERFORMANCE PROFILE answers:
- how fast is this backend instance for specific operations/sizes/dtypes/layouts?
- transfer bandwidth/latency;
- allocation cost;
- synchronization cost;
- layout conversion cost;
- launch/dispatch overhead.

PLANNER answers:
- among legal plans, what should we choose for this workload?

Do not encode capability/performance/planner data into KES-004 topology structures.

## 25. KES-004 Introspection Goal

Topology must be inspectable by developers and later by benchmarking/planner tooling.

Example laptop topology output:
Devices:
- [0] Intel CPU - CPU
- [1] Intel Iris Xe - GPU

Memory Spaces:
- [0] System RAM - SYSTEM - capacity if known

Backend Instances:
- [0] Scalar -> Device 0
- [1] AVX2 -> Device 0
- [2] Vulkan -> Device 1

Example NVIDIA workstation:
Devices:
- [0] CPU
- [1] NVIDIA T4

Memory:
- [0] System RAM
- [1] T4 VRAM

Backends:
- [0] Scalar -> CPU
- [1] AVX2 -> CPU
- [2] Vulkan -> T4
- [3] CUDA -> T4

This is not cosmetic; the planner, benchmarker, debugging tools, and advanced users need one authoritative machine model.

## 26. KES-004 Explicitly Out of Scope

Do not implement these inside KES-004:
- tensor allocation;
- storage_t implementation;
- CUDA malloc;
- Vulkan buffers;
- matmul kernels;
- kernel dispatch;
- planner decisions;
- performance benchmarks/profiles;
- operation capability matrices;
- tensor migration;
- automatic transfers;
- autodiff.

These systems depend on topology; they are not topology itself.

## 27. KES-004 Core Invariants to Preserve

1. Device = physical compute hardware.
2. Backend family = execution implementation/API.
3. Backend instance binds exactly one backend family to exactly one device.
4. Multiple backend instances may target one physical device.
5. One backend family may have multiple instances.
6. CUDA is never a device.
7. AVX2 is never a device.
8. Memory space models physical memory topology, not allocator/provider identity.
9. Multiple physical spaces of the same kind may exist.
10. Runtime topology owns topology-object lifetimes.
11. Runtime-local IDs identify topology objects.
12. Runtime IDs are not persistent machine IDs.
13. Device deduplication requires trustworthy identity.
14. Ambiguous devices are not merged by name alone.
15. Topology does not encode operator support.
16. Topology does not encode performance.
17. Topology does not make planner decisions.
18. Optional backend discovery failure is recoverable.
19. Scalar + host CPU is the baseline backend instance.
20. Backend-storage compatibility is separate from physical topology.

## 28. Planner and Performance Architecture

Planner principle:
Correctness and Kestrel semantics are hard constraints. Optimize only among legal plans.

Target total cost model:
- compute;
- transfer;
- allocation;
- layout conversion/repacking;
- synchronization;
- launch/dispatch overhead;
- memory pressure where relevant.

Do not optimize isolated kernel time if total end-to-end execution is worse.

Example:
A GPU kernel may be faster than AVX2, but if host->GPU transfer + dispatch + synchronization dominate a small workload, CPU may be cheaper overall.

Performance profiles should be measured on the actual machine rather than relying only on generic assumptions such as “GPU always faster.”

Benchmark/profile methodology direction:
- representative problem sizes;
- warmups;
- repeated measurements;
- robust summary statistic (median was recommended; percentiles where useful);
- asynchronous GPU work must be synchronized/timed correctly;
- separate submission overhead from completed execution latency;
- measure transfers both directions;
- measure allocation, synchronization, layout conversion, kernel latency/throughput;
- persist profiles keyed by relevant hardware/backend/driver/Kestrel kernel/version context;
- invalidate/rebuild when relevant context changes.

Planner should eventually explain decisions:
- chosen backend instance;
- legal alternatives;
- estimated component costs;
- why one plan won.

There should be one authoritative capability/performance model used both by planner and advanced-user introspection.

## 29. Expert Control Model

Kestrel should support multiple control levels:

Automatic default:
- planner chooses among legal strategies.

Constrained:
- examples: CPU only, no CUDA, prefer backend X, keep data in memory Y where possible.

Forced/expert:
- example: force AVX2+CPU backend instance if legal.
- if impossible, reject the request rather than silently violating semantics.

This preserves automation without making the system opaque or un-debuggable.

## 30. Implementation Roadmap - Current Core

Dependency-ordered roadmap discussed:

PHASE I - CORE FOUNDATIONS
- KES-003 Dtype Core / Numeric Semantics - MERGED
- KES-004 Runtime Topology - NEXT
- KES-005 Allocator + Storage Core

PHASE II - TENSOR CORE
- KES-006 Tensor Descriptor + Validation
- KES-007 Views, Strides, Broadcasting + Contiguity

PHASE III - EXECUTION CONTRACT
- KES-008 Backend Capability + Introspection
- KES-009 Scalar Backend
- KES-010 AVX2 Backend

PHASE IV - GPU EXECUTION
- KES-011 Vulkan Backend + Iris Xe
- KES-012 CUDA Backend + remote NVIDIA

PHASE V - PERFORMANCE INTELLIGENCE
- KES-013 Benchmarking + Performance Profiles
- KES-014 Planner + Cost Model
- KES-015 End-to-End Heterogeneous Execution

Later detailed engineering-spec work extended the roadmap beyond KES-015 to cover the neural-network product requirements (autodiff, training, etc.). The exact numbering KES-016..KES-020 was introduced in the detailed spec artifact; if continuing, consult the latest generated Kestrel Engineering Specification v2 for exact ticket names before issuing new tickets.

Important: implementation is sequential because of dependency, not because foundational decisions are being deferred.

## 31. Neural-Network / Training Direction After Runtime Core

Kestrel must train real models, not stop at runtime infrastructure.

Minimum original NN stack:
- reverse-mode autodiff;
- ops: matmul, bias add, ReLU, sigmoid, tanh, softmax, MSE, cross-entropy;
- optimizers: SGD, momentum, Adam;
- MNIST loader and training;
- gradient checking;
- CPU/GPU backend agreement;
- end-to-end training benchmarks.

Future capability ladder discussed:

Level 1 - basic numerical learning
- linear regression;
- logistic regression.

Level 2 - dense neural networks
- MLPs;
- MNIST classifier;
- tabular classifiers;
- regression networks;
- dense autoencoders.

Level 3 - convolutional networks
Add operators such as:
- Conv2D;
- MaxPool;
- AveragePool;
- padding;
- flatten;
- BatchNorm.
Possible demos:
- LeNet;
- CIFAR-10 CNN;
- image classifiers;
- convolutional autoencoders.

Level 4 - sequence models
Potential additions:
- Embedding;
- Gather;
- RNN cell;
- LSTM;
- GRU;
- sequence batching/masking.
Possible demos:
- character language model;
- text classifier;
- time-series model.

Level 5 - transformers
Potential primitives:
- batched matmul;
- LayerNorm;
- Softmax;
- Embedding/Gather;
- Transpose/Reshape;
- masking;
- attention;
- GELU.
Potential demo:
- tiny GPT-style transformer (e.g. low-million to tens-of-millions parameter scale, not hyperscale).

Canonical future demo suite proposed:
- linear/logistic regression;
- MNIST MLP;
- MNIST autoencoder;
- CIFAR-10 CNN;
- character-level LSTM;
- tiny transformer language model;
- mixed-precision training benchmark.

Future training-system capabilities discussed:
- serialization/checkpoints;
- train/eval mode;
- weight initialization;
- gradient clipping;
- learning-rate schedules;
- dropout;
- batch normalization;
- mixed precision;
- gradient accumulation;
- mini-batching;
- dataloader abstraction;
- random seeds/reproducibility.

Do not interpret all of these as current core-v2 requirements unless the latest roadmap explicitly promotes them. They are documented future direction.

## 32. Mixed Precision - Strategic Fit

Kestrel is unusually well positioned for future mixed-precision training because dtype architecture already distinguishes FP16/BF16/FP32/FP64 and separates promotion from accumulation.

Conceptual mixed-precision direction:
- FP16/BF16 weights or activations where useful;
- FP32 accumulation;
- potentially FP32 gradients/master weights depending on training policy;
- capability checks per backend instance;
- planner aware of whether AVX2/Vulkan/CUDA truly supports requested dtype efficiently.

This is a FUTURE direction, not yet implemented.

## 33. Kestrel Benchmark Philosophy

Daniel wants benchmark claims to be honest, reproducible, and attributable to mechanisms.

Original benchmark philosophy:
- establish baseline before optimization;
- benchmark matmul across N = 128, 256, 512, 1024, 2048 (from original spec);
- report GFLOP/s and percent of theoretical peak where appropriate;
- when memory hierarchy optimizations are introduced, measure cache effects (e.g. perf cache misses/references) rather than claiming accidental speedup;
- AVX2 should be compared against blocked scalar baseline;
- GPU should be compared against external reference (cuBLAS for CUDA matmul), not only the project’s own naive kernel;
- time host<->device transfer separately from GPU kernels;
- report limitations honestly (e.g. bandwidth-bound regimes).

Evolved runtime benchmark requirements:
- include transfer, sync, layout conversion, allocation, launch/dispatch overhead;
- planner optimization is based on total plan cost, not isolated kernel speed;
- same authoritative measurements should support both planner decisions and user introspection.

Never manufacture “wins.” A result such as “38% of cuBLAS” can be credible and interesting if measured correctly.

## 34. Kestrel vs PyTorch - Positioning

Kestrel should not try to beat PyTorch at being PyTorch.

PyTorch:
- full Python-first ML framework;
- huge ecosystem/operator coverage;
- production-grade breadth.

Kestrel:
- C-first neural-network library with a compact, explicit heterogeneous runtime;
- emphasizes hardware-aware planning, memory/storage topology, backend capability, performance transparency, and understanding the machinery underneath training.

Strong project question:
Can a compact C neural-network runtime understand CPU SIMD, integrated GPU, discrete GPU, memory movement, layout costs, and backend behavior well enough to choose efficient execution strategies automatically while still supporting end-to-end training?

PyTorch/cuBLAS/etc. are baselines/references where useful, not targets to clone feature-for-feature.

## 35. Daniel’s Hardware and GPU Development Plan

Local machine context:
- ASUS ExpertBook B1;
- 11th-gen Intel Core i5;
- 16 GB RAM;
- Intel Iris Xe integrated GPU;
- no local NVIDIA discrete GPU.
A prior note recorded “26 GB storage,” likely a typo/abbreviation; do not rely on that number without Daniel confirming.

Local development targets:
- Scalar CPU;
- AVX2 CPU;
- Vulkan on Intel Iris Xe;
- core runtime/planner/tests.

CUDA remote-development plan:
- Lightning AI preferred initially;
- Kaggle/Colab fallback;
- same Git repo, no notebook-specific fork.

Previously observed Lightning free-GPU budget snapshot (historical, may change):
- T4 ~27 free hours;
- L40S ~4;
- A100 ~3;
- H100 ~3;
- H200 ~2;
- CPU free.

Recommendation was to use T4 for ordinary CUDA development/correctness/debugging, and preserve expensive accelerators for later performance/profile/planner experiments.

Conditional tests:
- CPU tests always;
- Vulkan tests when device/backend exists;
- CUDA tests when CUDA exists;
- unavailable optional backend should SKIP cleanly, not fail entire suite.

## 36. OpenGL Decision

Daniel asked whether OpenGL should be used.

Decision/direction:
- OpenGL is primarily graphics; compute shaders exist but it is not the preferred Kestrel compute foundation.
- Vulkan is graphics + first-class explicit general compute and is a better cross-vendor GPU backend for this architecture.
- CUDA remains NVIDIA-specific compute.

Current backend family:
CPU: Scalar, AVX2
GPU: Vulkan cross-vendor, CUDA NVIDIA-specific

Do not add OpenGL as a core backend without a specific future reason.

## 37. Ghost VM - Portfolio Project

Ghost VM is Daniel’s separate systems project. It is not a Kestrel dependency.

Existing code/history:
- stack VM;
- typed objects;
- bytecode emitters;
- packet parser;
- batch execution;
- considered pre-hardened / needs serious correctness work.

Planned phases:
Phase 0 - Harden
- fuzz parser;
- ASan/UBSan;
- fix memory-safety and lifetime bugs;
- clean diagnostics.

Phase 1 - Protocol v2 + TCP
- explicit endian-safe serialization;
- versioned packets;
- length validation;
- real client/server sockets;
- integrity story clear (FNV as corruption-only or keyed alternative).

Phase 2 - Compiler/language
- Pratt parser / single-pass compiler;
- jumps, loops, functions, locals/globals;
- call frames;
- validate jump targets before execution because bytecode is untrusted network input.

Phase 3 - Runtime performance
- NaN boxing (fallback to tagged value if time stalls);
- precise mark-sweep GC;
- string interning;
- computed goto with switch fallback;
- benchmark each optimization separately.

Phase 4 - Bytecode optimizer
- constant folding;
- peephole optimization;
- superinstructions where justified;
- explicitly skip dead-code elimination unless language structure makes it meaningful.

Stretch:
- closures/upvalues.

Cut:
- modules;
- second register-based VM backend.

Ghost’s distinguishing story:
Treat bytecode as untrusted network input and validate aggressively before execution.

## 38. Raptor - Portfolio Project

Raptor is Daniel’s future build-system project.

Stage 0:
- work through nob playlist;
- use it to build real projects;
- keep a friction log;
- derive features from real pain rather than copying a feature checklist.

Core thesis:
- nob is imperative: user manually writes staleness checks and effectively schedules the build;
- Raptor is declarative: user declares targets/dependencies/commands and scheduler determines ordering, staleness, parallelism, caching.

Phase 1 - graph
- declarative C API;
- action_t;
- graph edges via outputs->inputs;
- three-color DFS topo sort;
- cycle detection that names the cycle;
- serial executor.

Phase 2 - correctness
- SHA-256 content hashing;
- action hashing including command/env/input digests/output paths;
- depfile parsing;
- own open-addressing hash table;
- prove correctness in cases where mtime systems rebuild unnecessarily or silently fail to rebuild.

Phase 3 - parallelism
- pthreads/Win32 thread pool;
- Chase-Lev work-stealing deque with C11 atomics;
- critical-path scheduling;
- ThreadSanitizer clean.

Phase 4 - cache/ergonomics/self-host
- content-addressed output store;
- compile_commands.json;
- sanitizer presets;
- diagnostics;
- test/benchmark targets;
- self-host Raptor building Raptor.

Raptor is intended to build Kestrel and Ghost during development, but is not a Kestrel runtime dependency.

## 39. Talon - Portfolio Project

Talon is Daniel’s future container-orchestrator project in Go.

Target identity:
A container orchestrator with its own OCI-compliant runtime underneath - no Docker/runc dependency for the core runtime story - using Linux namespaces, cgroups v2, pivot_root-style filesystem isolation, networking, and eventually a Raft-replicated control plane.

Stage 0:
- work through Tim Boring’s orchestrator book honestly;
- understand manager/worker split, state machines, scheduling, storage;
- keep friction log around opaque Docker behavior and task transitions.

Phase 1 - harden book code
- tests;
- API fuzzing;
- structured logging;
- graceful shutdown;
- race/lint/vet cleanliness.

Phase 2 - own runtime (key differentiator)
- namespaces;
- cgroups v2;
- pivot_root/rootfs;
- OCI bundle config;
- registry/image layers + overlayfs;
- swap out Docker dependency behind interface.

Phase 3 - networking
- veth pairs;
- bridge;
- IPAM;
- NAT/port mapping;
- cross-host networking (VXLAN or WireGuard direction).

Phase 4 - consensus
- Raft / multi-manager control plane;
- leader failover;
- partition behavior;
- chaos testing.

Phase 5 - isolation
- seccomp-bpf;
- capability dropping;
- user namespaces;
- read-only rootfs by default;
- no-new-privileges.

Stretch:
- CRIU checkpoint/restore;
- CRI shim.

Cut:
- web UI / Helm-like product features.

## 40. Portfolio and Career Direction

Daniel wants a portfolio containing:
- Kestrel;
- Ghost VM;
- Raptor;
- Talon;
plus an LLVM open-source contribution.

Historical target plan discussed for 2026-2027:
- Kestrel complete by end of 2026;
- LLVM contribution attempted/ideally merged by end of 2026;
- Ghost VM major phases by late Feb / complete around March 2027;
- Raptor as primary Apr-May 2027 after background Stage 0;
- Talon Stage 0 should begin before implementation because Talon is schedule risk;
- June 30, 2027 portfolio freeze with all four projects defensible, benchmarked, documented, CI-backed.

Treat exact dates as planning targets, not immutable facts; if resuming much later, ask Daniel whether timeline is still current.

OSS target changed from Mesa to LLVM because LLVM better aligns with compilers/runtime/performance/Texyn direction.

## 41. Learning Plan

Courses Daniel considered via Coursera access:
1. Machine Learning: Theory and Hands-on Practice with Python - University of Colorado Boulder.
2. Mathematics for Machine Learning - Imperial College London.
3. Foundations of Data Structures and Algorithms - University of Colorado Boulder.

Also:
- LLVM learning/contribution;
- long Hinton/deep-learning-evolution course/video (~27h) for research literacy.

Prioritized direction discussed:
1. Kestrel;
2. LLVM;
3. Math for ML;
4. ML theory;
5. DSA as less urgent/background.

Scheduling preference:
- one main focus per day;
- suggested Kestrel ~3 days/week minimum;
- LLVM 1 day;
- one Coursera track at a time (Math for ML first);
- flexible day;
- rest day.

Do not run every goal at full intensity simultaneously. Daniel explicitly expressed concern about overload. The right response is prioritization and finite commitments, not adding more parallel obligations.

## 42. Documentation / Artifact History

Important artifacts created during this conversation or earlier context:

- Arena Architecture Documentation (writing block ID historically recorded as 64183).
- KES-001 Arena Implementation Contract (writing block ID historically recorded as 37416).
- Kestrel Architecture Specification v2 (first version; later corrected/redesigned because it over-centered the runtime).
- Kestrel Architecture Specification v2 - Redesigned: restores neural-network library/training identity and places heterogeneous runtime underneath it.
- Kestrel Engineering Specification v2: very detailed engineering spec, phased implementation, passing criteria, benchmarking, CI/release gates; described as ~65 pages when created.
- Kestrel Future Direction Roadmap: future model capability ladder (dense -> CNN -> recurrent -> transformer), mixed precision, broader training features.

If a future assistant needs exact content from these files, retrieve the latest artifacts rather than relying only on this handoff summary.

Do not assume a sandbox path remains valid forever without checking the active runtime.

## 43. Review and Explanation Preferences

Daniel likes:
- diagrams;
- small concrete examples;
- definitions before abstraction;
- “why” before code;
- explicit architecture boundaries;
- exact pass/fail criteria;
- understanding mechanism rather than memorizing API.

Daniel dislikes:
- treating him as already expert in low-level GPU/system concepts;
- standards jargon without intuition;
- assistant writing ticket implementations for him;
- “we can decide later” for known foundational choices;
- repeated/redundant review feedback delivered across many turns;
- needless abstractions;
- project framing drift (especially Kestrel being described as “just a runtime”).

When he asks “explain everything we concluded before I implement,” do a comprehensive conceptual walkthrough first. Only then issue the implementation ticket.

## 44. Current State / Resume Here

As of the end of this conversation:

Kestrel status:
- KES-001 Arena lifecycle/failure contract: MERGED.
- KES-002 Heterogeneous runtime architecture: FROZEN / accepted architecture.
- KES-003 Core dtype/numeric semantics: MERGED with strong test coverage.
- KES-004 Runtime Topology: conceptual design freeze explained in depth; FINAL IMPLEMENTATION TICKET HAS NOT YET BEEN ISSUED in this chat.

Immediate next action:
1. Confirm Daniel is satisfied with the KES-004 conceptual model.
2. Convert the frozen KES-004 design into the formal “Texyn Labs - Kestrel Engineering” implementation contract using the standard ticket format.
3. Keep scope strictly to topology: device/backend-instance/memory-space registry, IDs, ownership, discovery model, introspection, relationships.
4. Do not leak KES-005 storage/allocator implementation, KES-008 capabilities, KES-013 performance, or KES-014 planner into KES-004.
5. Daniel implements.
6. Review as PR with explicit verdict.

The final implementation ticket should include:
- Project, Company, Engineer, Area, Type, Priority, Status, Depends on, Blocks;
- Context and objective;
- exact object/enum/ID contracts;
- lifecycle/ownership;
- discovery behavior;
- optional-backend failure behavior;
- deduplication rule;
- introspection requirements;
- tests/acceptance criteria;
- out-of-scope;
- downstream impact;
- engineering note;
- completion checklist.

Do not reopen KES-003 numerical policy while implementing KES-004 unless a real cross-cutting contradiction is discovered.

Most important mental model to preserve:
Kestrel is a C neural-network library that will train and run ML models. Its runtime is becoming sophisticated so the library can understand the machine, then specialize execution for it.

