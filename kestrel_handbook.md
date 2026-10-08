# Kestrel: Engineering History, Architecture, Current State & Future Direction

Third edition • Canonical engineering checkpoint 8 October 2026 • Daniel Ademoye

This book connects Kestrel's earliest C learning experiments to its current migration checkpoint and its long-term training-system design. It is intended for an engineer who needs to understand the reasons behind the abstractions, the exact code that exists, the contracts that remain to be implemented and the evidence required to finish the work.

Arena and dtype foundations are implemented and frozen. KES-004 topology is active, with representation and owner lifecycle implemented. The tensor code is functional legacy code. The next engineering slice is topology-builder observation storage and registration. Core-v2 training and the post-v2 model ladder remain planned.

Kestrel is currently Daniel Ademoye's personal engineering project. Its work may inform Texyn Systems research and future systems, but this handbook does not imply that every Kestrel component is currently a Texyn product.

## Authority and reading guide

Accepted newer KES-004 decisions govern topology semantics. The inspected working tree governs implementation claims. The core-v2 specification supplies acceptance criteria; the future roadmap supplies planned expansion. A frozen contract can remain unimplemented. A historical program can work without conforming to a later contract. Review verdict MERGED does not independently prove a remote Git merge.

Read the orientation for the canonical architecture, Part I for its causal history, Part II for current code and evidence, Parts III–IV for detailed contracts, Part V for core-v2 completion, and Part VI for the future product. The decision register is the cross-cutting reference. Source details and fingerprints follow in an appendix; the final chapter is the exact resume point.

| Label | Meaning |
| --- | --- |
| IMPLEMENTED | Present in the inspected code, within the stated boundary |
| FROZEN | Accepted contract; implementation can remain pending |
| ACTIVE | Current engineering work |
| PLANNED | Future requirement or directional work |
| LEGACY | Working earlier architecture awaiting migration |
| SUPERSEDED | Replaced rule, retained only for history |
| OPEN | Unresolved detail or unverified assertion |

The source register retains S1–S12 and adds H1–H10 for early programs/notes and P1–P6 for the six supplied LinkedIn posts. No publication dates or missing screenshots are invented. Earlier claims that the posts were unavailable are replaced by this edition's actual intake record.

## Status at a glance

| Ticket or subsystem | State | Evidence and limit |
| --- | --- | --- |
| KES-001 Arena | IMPLEMENTED / FROZEN / MERGED review verdict | Reviewed as merged in conversation; substantive arena code and tests exist |
| KES-002 Architecture | FROZEN | ADR and architecture record; not a claim that runtime components exist |
| KES-003 Dtype | IMPLEMENTED / FROZEN / MERGED review verdict | Size, category, promotion, accumulation queries exist; arithmetic kernels do not |
| KES-004 Topology | ACTIVE / PARTIALLY IMPLEMENTED | Current vocabulary and owner lifecycle only; full acceptance is pending |
| Legacy tensor | LEGACY / IMPLEMENTED | FP32 arena payload, element strides, device tag |
| KES-005 Storage | PLANNED | No current storage implementation in inspected source tree |
| KES-006/007 Tensor redesign | PLANNED | Frozen descriptor/view direction awaits storage |
| KES-008–015 Runtime execution | PLANNED | Capability, backends, profiles, planner, integration |
| KES-016–020 Training and release | PLANNED | Autodiff, operators, optimizers, MNIST, evidence gates |

“Merged” here records the engineering-review verdict in the source conversation. It is not proof of a remote pull-request merge. The inspected local Git tree has uncommitted changes and untracked documentation/tests. That distinction prevents a new engineer from assuming the entire checkpoint can be reconstructed from HEAD alone.

# Orientation — The Current Architecture

> **CURRENT CONTRACT** Kestrel is a compact C neural-network system whose runtime makes numerical meaning, layout, ownership, hardware resources, legal execution and measured costs inspectable. The intended architecture is broader than the code currently implemented. Read status labels independently from architectural descriptions.

## The conceptual stack

```text
Neural-network API
        ↓
Reverse-mode autodiff
        ↓
Tensor + operator semantics
        ↓
Heterogeneous runtime
```

The neural-network layer expresses models, parameters, losses and training. Autodiff records dependencies and computes gradients through semantic operators. Tensor and operator semantics define values, dtype rules, shape, layout and views. The heterogeneous runtime supplies resources and execution strategies without changing those meanings. This is the accepted product structure; only the foundation components listed in the current-state chapter exist now. [S1, S2, S4]

| Runtime concern | Its question | What it must not silently decide |
| --- | --- | --- |
| Topology | What resources and structural relations exist? | Operator legality or transfer speed |
| Storage and providers | Where do bytes live and who owns them? | Tensor shape or operator semantics |
| Capability | Is this execution legal? | Which legal choice is fastest |
| Backends | How does an operator execute here? | New promotion or layout semantics |
| Performance profiles | What does execution cost on this machine? | Permission to choose an illegal operation |
| Planner | Which legal strategy should Kestrel choose? | Kernel implementation details |
| Execution | Perform the selected plan | Undeclared substitution for forced choices |

```text
Memory Space
     ↓
Provider / Allocator
     ↓
Storage
     ↓
Tensor view
```

This is a responsibility diagram, not ownership in both directions. Tensor views refer to storage; storage does not maintain a list of tensor owners. Providers allocate inside a memory space. Multiple providers may allocate in the same space while producing incompatible native resources. A memory-space ID does not grant a backend permission to consume every token allocated there.

## The four topology entities

```text
Backend Instance ── targets exactly one ──> Execution Target
                                               │
                BACKED_BY (zero or many)        ├──> Physical Device
                SUBTARGET_OF (direct parent)   ├──> Execution Target
                structural association         └──> Memory Space
```

A Physical Device is a root physical compute resource at execution-relevant granularity. An Execution Target is a selectable compute destination: a whole device, a partition, a nested subtarget or a representable aggregate. A Backend Instance binds one backend family to exactly one target. A Memory Space identifies a domain in which bytes may reside. Each arrow is typed; the direct-parent arrow never originates at a Backend Instance. [S5–S9]

## Distinctions that prevent architectural drift

Topology is not capability: an association between a target and memory space does not prove that a particular operation can consume a particular allocation. Capability is not preference: a required alignment is a legality constraint, while a faster layout can be only a preference. Capability is not performance: a legal kernel can be slow. Performance is not planning: measurements describe costs, while planning combines legal operations, movement, layout conversions and user constraints. Planning is not execution: selecting a strategy and performing it have different failure and ownership responsibilities.

Tensor is not storage: multiple views can interpret the same allocation. Memory space is not allocator: a domain may admit several resource providers. Physical device is not execution target: partitions and aggregates break one-to-one correspondence. Execution target is not backend instance: several API or implementation paths may execute on the same proven target. These distinctions govern the rest of the book; historical names do not override them.

## Engineering Philosophy — Later Is Now

Kestrel follows a principle Daniel calls **“Later is now.”** When an accepted future capability already invalidates a foundational assumption being made today, its architectural consequence belongs in the design now. The dependent capability can remain scheduled for its later ticket.

This separates designing a durable seam from implementing the feature that will use it. The principle applies especially to identity, ownership, cardinality, lifecycle, public ABI, tensor semantics, storage representation and topology relations. It does not freeze unresolved ABI guarantees or authorize speculative capabilities.

```text
Known future capability
        ↓
Does it invalidate a foundational assumption?

NO  → defer it
YES → design the lasting seam now;
      implement the capability in its later ticket
```

| Known capability | Architectural seam now | Implementation later |
| --- | --- | --- |
| NVIDIA MIG | Physical Device differs from Execution Target; explicit backing, target identity and memory-domain cardinality | NVML/MIG discovery, runtime integration and execution support |
| Level Zero nested subdevices | Direct execution hierarchy through SUBTARGET_OF | Level Zero discovery adapter and backend work |
| SR-IOV / virtualized targets | Zero known physical backing is representable; a visible parent is not mandatory | Host/guest discovery and SR-IOV integration |
| Multiple dtypes | Dtype-independent storage and signed byte strides; no universal float pointer | The complete set of dtype-specific kernels |
| Vulkan device groups | BACKED_BY is many-valued rather than one embedded physical ID | Aggregate-target and multi-GPU execution |

“Later is now” is a rule against knowingly building foundations that contradict accepted future requirements. It is **not permission for scope creep**: represent the proven distinction now, keep adapters, kernels and runtime integration in their owning tickets, and leave unresolved policies OPEN. The existing KES-004 implementation boundary and exact next checkpoint are unchanged.

# Part I The engineering history

> **HISTORICAL** This is an ordered engineering history, not a reconstructed publication calendar. Six LinkedIn posts are available: five in the repeated pasted-text attachment and the sixth in the conversation. Their exact publication dates and screenshots are not available. Source labels H1–H10 identify the early code and notes; P1–P6 identify the posts. Reported runs remain attributed reports, distinct from the current verification results.

## Curiosity before architecture

Daniel began with a question about what `model.fit()` actually does. A framework makes training easy to invoke but hides prediction, loss evaluation, differentiation, parameter updates and memory management behind that call. The first experiment made those operations small enough to inspect. There was no initial requirement to represent GPU partitions, reconcile hardware identities or compare backend execution plans. Those concerns arose later because the earlier representations stopped explaining the system he wanted to build. [P1, P6]

That distinction explains the project's character. Kestrel's architecture is a record of lessons from working programs. A more general representation repeatedly replaced a successful but narrower one. The linear regressor could learn a line; the fixed XOR program could learn XOR; the single-header engine could train a configurable dense network. Their limitations became visible only when Daniel asked the next question. Calling those stages failed implementations would erase the evidence that caused the redesigns.

The posts also capture how the work felt. The XOR account describes late-night experimentation and the satisfaction of seeing the truth table learned. The generalized-network account describes the change from programming one thing to representing a class of things, along with doubt, taking a walk and returning to the problem. These are Daniel's supplied accounts, not inferred biography. The engineering consequence was persistence with a question until the representation, rather than just one output, made sense. [P2, P3]

## An ordered map of the changes

| Stage | Working idea | Assumption exposed | Next design |
| --- | --- | --- | --- |
| One-weight regression | Predict by multiplying input and weight | Every line passes through the origin | Add a bias |
| Affine regression | MSE and numerical slopes update two parameters | A tiny parameter space is representative | Try classification and more parameters |
| OR | Sigmoid over two inputs and a bias | One affine boundary is sufficient | XOR and hidden units |
| Fixed XOR | Two hidden neurons and an output | Hard-coded names can scale | Architecture arrays and matrices |
| General dense networks | Layer dimensions determine weights and activations | Repeated perturbation is affordable | Reverse-mode differentiation |
| Matrix and arena work | Views and shared allocation lifetimes | One row stride and simple bump allocation suffice | General strides and defensive lifetimes |
| Scalar autodiff | Dependency graph and local derivatives | One graph node per scalar is economical | Tensor-level operations |
| N-dimensional tensors | Shapes and strides describe views | Float elements and a CPU/CUDA tag suffice | Dtype, storage and topology |
| Dtype and storage | Numerical rules above backends; opaque allocations | A device means one hardware/API pair | Separate physical roots and execution paths |
| KES-004 research | Explicit topology entities and evidence | Every target has one visible physical parent | Zero/many backing relations and reconciliation |

The sequence is conceptual. Matrix/arena work and learning about differentiation informed each other; the available material does not establish a dated, strictly non-overlapping development schedule.

## Linear regression with one weight

The zero-bias C program uses six rows whose outputs equal twice their inputs. Its model is just input multiplied by a scalar weight. Mean squared error averages the squared residual over the six rows. To estimate a gradient, the program evaluates loss at the current weight and at a nearby weight, divides the loss difference by the perturbation, and moves the weight in the opposite direction. Random initialization makes the starting point explicit. [H6]

Conceptually, this is the smallest visible training loop: a model defines a family of predictions, a loss evaluates a candidate, a numerical derivative indicates local sensitivity, and a learning rate controls the update. The supplied source uses 499 loop iterations with an epsilon and learning rate of 0.001. These are properties of that file, not general optimization prescriptions or a newly measured convergence claim.

What worked was the complete connection between a parameter and a changing loss. What the model assumed was equally important: a line through the origin can explain the data. Real affine relationships can require a nonzero intercept. Adding a bias therefore changed both the model and the gradient calculation: learning was no longer one unknown scalar but a vector of parameters. This was the first instance of the recurring Kestrel pattern—generalization makes a hidden assumption visible.

## Two parameters make the training loop explicit

The bias-enabled regressor predicts an affine function with weight and bias. The supplied `neuron.c` contains seven exact affine examples corresponding to output = 500 × input + 1000. It starts both parameters randomly, caches the current cost, estimates each derivative by perturbing its parameter, and applies both updates after computing those derivatives. This gives both derivatives the same base parameter point. The source runs 40,000 iterations. [H4]

The first LinkedIn post describes a related exercise using noisy workstation/electricity-cost data and roughly 6,000 iterations. Those details are valuable historical evidence, but they do not describe the supplied file literally. The handbook preserves both accounts rather than rewriting one to fit the other. The historical learning note also mixes example framing; the verified mathematical relationship in the file is the safer description of that implementation. [P1, H4, H5]

> **HISTORICAL** P1 reports the public learning exercise. H4 records a concrete surviving implementation. The difference between noisy data and exact affine data, and between 6,000 and 40,000 iterations, remains visible. No publication date or missing intermediate revision is inferred.

The two-parameter experiment explains what an optimizer is actually changing. Weight changes how strongly input contributes; bias shifts the prediction independently of input. A finite difference does not reveal a symbolic derivative. It samples the loss near a particular parameter vector. Choosing epsilon too small can amplify roundoff; choosing it too large can distort the local approximation. The learning rate is a separate quantity controlling how far the update moves.

The early notes use teaching analogies about descending a slope. Those analogies are useful if their limits remain clear. Gradient magnitude measures local sensitivity, not guaranteed distance from the optimum. A squared loss for a nonlinear neural network is not globally a simple U-shaped bowl. Even for regression, noisy observations need not permit zero error. These qualifications become more important as the project leaves the two-parameter case. [H5, H1]

## OR turns regression machinery toward classification

The next experiment uses two inputs, two weights and a bias. A sigmoid maps the affine output to a value between zero and one; the program interprets values at or above 0.5 as the positive class. The OR truth table can be separated by a single affine boundary, so this architecture can represent the task. The supplied `gates.c` uses float parameters, sigmoid and loss calculations, with double-valued epsilon/rate variables, MSE, finite differences, epsilon 0.1, learning rate 0.1, and 320,000 iterations with periodic printing. All three derivatives are computed before the parameters are updated. [H7, H2, H3]

The important success was not merely a printed truth table. Daniel could reuse the same optimization mechanism with a different model and interpretation of output. Prediction, error and parameter updates remained visible, while a nonlinear activation entered the model. The experiment connected a numerical output to a decision rule without hiding either behind a classifier API.

The limitation was representational. A single sigmoid applied to an affine score still has one affine decision boundary at a fixed threshold. Adding iterations cannot make that boundary solve XOR. This is a different kind of failure from a poor learning rate: the desired function lies outside the chosen model family. The lesson led directly to hidden units.

The notes are preserved as learning material, with their approximations identified. Sigmoid is not the only differentiable activation, and gradient descent does not require sigmoid—linear regression already demonstrates that. A hard threshold is unsuitable for ordinary derivative-based learning because its derivative is zero away from its discontinuity. Sigmoid can also saturate, and a sigmoid output is not automatically a calibrated probability. These distinctions keep the historical explanation useful without turning early shorthand into a current numerical contract.

## XOR makes representation the central problem

XOR labels the two mixed-input cases positive and the two equal-input cases negative. No single affine separator satisfies all four labels. The first small neural network introduces two hidden sigmoid units and one sigmoid output: two inputs, two hidden units and one output, with six weights and three biases. The hidden layer changes the features seen by the output unit. [P2, H2, H8]

This stage made a neural network concrete as a composition rather than a mysterious learning object. Each hidden unit computes its own affine score and sigmoid. The output consumes those two results, applies another affine map and sigmoid, and the same MSE-based learning loop evaluates the four cases. The public post reports successful XOR learning. That report belongs to the historical experiment; it is not evidence that the present modular runtime can train.

The source `full_gates.c` packs the nine parameters into one array and uses macros to name their roles. This reduced repeated parameter-specific code and anticipated a later data-driven network representation. It uses epsilon 0.001, learning rate 0.1, an upper bound of 320,000 iterations and an early stop when cost falls below 0.001. It also prints hidden activations, making the internal representation something Daniel could inspect rather than only checking the final answer.

One implementation detail deserves explicit preservation. This file updates each parameter immediately while using a baseline cost cached before the parameter loop. Later perturbation evaluations can therefore include earlier updates from the same loop. It is not a clean numerical gradient evaluated entirely at one fixed parameter vector. The later generalized engine separates gradient estimation from parameter updates. Recording this difference explains an actual improvement rather than idealizing every historical program as the final algorithm. [H8, H9]

The historical notes sometimes describe hidden units as logic gates. That is an intuition, not a guaranteed interpretation of learned weights. In particular, exact AND and NAND alone cannot distinguish every XOR-required case. The useful architectural conclusion is that learned nonlinear intermediate features make the function representable. No fixed semantic label is assigned to a hidden unit merely because the overall network learns XOR.

## From nine named parameters to a family of networks

Once a fixed network worked, the next question was how to represent arbitrary dense architectures without rewriting the forward pass for every shape. In the single-header engine, an architecture array specifies the number of units per layer. The implementation allocates arrays of weight matrices, bias matrices and activation matrices according to those dimensions. For a layer with n inputs and m outputs, its weight matrix is n by m, its bias is one by m, and its activation buffer is one by m. The layer count derives from the architecture length. [P3, H9]

This is the transition from an experiment to a small engine. The forward loop follows the representation rather than a handwritten chain of neuron variables. The same engine can instantiate the historical `{2, 2, 1}` network while allowing other dense shapes. Cost evaluation still walks examples individually; this representation is not yet a batched N-dimensional tensor engine.

The XOR driver stores each training row as two inputs followed by one target. A matrix row stride of three lets the input view expose two columns and the output view expose one column beginning at the third value, both over the same interleaved data. This small arrangement already separates logical interpretation from physical placement. A view does not need to own a separately packed buffer. [H10]

The generalized numerical-gradient routine also improves the update discipline. It caches the baseline loss, perturbs one weight or bias, evaluates, restores that parameter, and stores the estimated derivative in a separate gradient network. A later learning pass applies the derivatives. The driver allocates the current, gradient and best-known networks. Best-model tracking copies parameters rather than treating the current training state as automatically the best result.

The supplied XOR driver uses a large startup arena, one million iterations and epsilon/learning rate 0.1. The legacy README reports cost near 0.000026, predictions around 0.005/0.995 and roughly 4.2 seconds at `-O2`. Those are historical measurements with random initialization and their original workload conditions; they are not a benchmark of core v2 or a reproducibility guarantee. [H10, S12]

## Matrices expose memory layout and lifetime

The early matrix abstraction contains rows, columns, a row stride and a float pointer. Addressing is row × stride + column. This can express gaps between rows and interleaved row views, but it assumes adjacent columns are adjacent floats. Transpose breaks that assumption: after exchanging row and column roles, advancing one logical column may require jumping over several physical values. A single row stride does not represent both independent increments. [H9, P5]

Memory management developed alongside representation. The early `Chunk_memory` bump allocator serves many startup allocations from one backing block. A common lifetime simplifies teardown and avoids per-object heap management in the training path. The early allocator is intentionally simple; its offset-plus-size check lacks the later overflow-safe arithmetic and alignment contract. It should not be mistaken for KES-001 merely because both allocate from an arena. [H9, S9]

The arena redesign turns an implementation habit into a contract. ZERO, LIVE and INVALID states distinguish legal initialization and destruction from misuse. Allocation validates alignment and bounds before changing the offset. A failed request leaves existing allocations intact. Marks and resets allow controlled rollback, but only for the owner of the affected lifetime region. These decisions matter for views: a descriptor can survive in C after its bytes have been invalidated, so ownership discipline is part of correctness.

The fifth post reports 67 arena and 55 tensor tests, with AddressSanitizer and UndefinedBehaviorSanitizer runs described as clean at that point. The current inspected suites report different assertion totals. Both records are retained with their scope and time distinguished; this edition does not retroactively attach that earlier sanitizer report to the current source snapshot. [P5, S9]

## Finite differences reach their scaling limit

Numerical differentiation is attractive because it requires only a working loss function. Its weakness appears when each parameter perturbation requires another evaluation over the training data. With P parameters, N examples and forward cost F, one gradient estimate costs approximately (P + 1) × N forward evaluations, or O(P × N × F). The growth is not inherently exponential in P, despite informal wording in the historical backpropagation note. [H1, H9]

For the nine-parameter XOR model and four examples, the gradient estimate needs forty forward evaluations when it reuses one baseline cost. The generalized training loop then evaluates cost again after applying the update, adding four more forward evaluations for that iteration. This distinction keeps an explanatory gradient count from becoming an incorrect count of the whole training loop.

The fourth post applies the same argument to a 784→256→10 MNIST network. Including biases gives 203,530 parameters. On 60,000 examples, one baseline plus one evaluation per parameter gives 12,211,860,000 example-level forward evaluations for one numerical-gradient estimate. The post's approximately twelve-billion figure captures the scale. This is arithmetic from the described workload, not a measured training runtime. The model is small by modern standards, yet the method is already impractical. [P4]

The lesson is causal: faster allocation or a cleaner matrix API cannot remove the extra loss evaluation per parameter. The differentiation algorithm must change. Finite differences remain valuable as a small-test oracle because they are independent of handwritten backward rules, but they cease to be the intended training method.

## A scalar reverse-mode prototype makes the chain rule executable

Daniel next explored reverse-mode autodiff in Python. The fourth post describes a roughly 120-line scalar engine and a small MLP training example. Scalar values record dependencies and local backward behavior. A topological traversal orders the dependency graph, and the backward pass propagates contributions from outputs toward inputs. Shared dependencies require addition of gradient contributions, not overwriting. [P4]

The intellectual step was to reuse the work represented by the forward graph. Reverse mode evaluates local derivative rules along that graph instead of perturbing every parameter independently. Its cost still depends on graph size and operator work; saying that it uses a bounded number of traversals does not make training constant-time.

What worked was an inspectable implementation of the chain rule. The next limitation was graph granularity. If every scalar addition and multiplication becomes a separate graph object, a modest dense network and batch can generate many objects. The post describes millions of nodes and hundreds of megabytes as scaling concerns; no independent memory benchmark for that prototype is supplied. Tensor-level graph nodes offer a way to express one matmul as one semantic operation with a matrix-level backward rule.

The post also discusses translating Python closures into C through function pointers plus captured state. This is a proposed implementation direction, not evidence of a completed C autodiff runtime. Likewise, the proposed Python oracle and a 1e-6 comparison expectation in that narrative must not replace the later core-v2 FP32 gradient-check gate of relative error below 1e-4. The prototype, the intended oracle and KES-016 are distinct milestones.

## N-dimensional tensors make views explicit

The next representation carries rank, a shape array, per-dimension strides, logical count, a data pointer and a device field. Rather than defining a matrix as the universal object, it describes an N-dimensional view. The current legacy implementation supports up to six dimensions. Shape and strides are metadata; the payload remains a float allocation from the arena. [P5, S9]

The fifth post gives a particularly useful example. A contiguous shape `{2, 3}` has element strides `{3, 1}`. A transpose view changes the shape to `{3, 2}` and strides to `{1, 3}`, while sharing the same six floats. This makes the earlier matrix assumption visible: the old column increment of one is no longer valid. A transpose is a change in interpretation, not intrinsically a copy.

This working tensor implementation is a successful legacy stage. Its tests cover the behavior it actually promises. The later v2 descriptor requires more expressive addressing, dtype and storage ownership, but those requirements do not make the old transpose incorrect. They change the contract the next implementation must satisfy.

Aliasing is the consequence that follows from views. Two descriptors may refer to overlapping bytes. Resetting the arena invalidates all views into the discarded region. A future broadcast view may map many logical coordinates to one physical element. Layout and lifetime therefore cannot be treated as incidental optimization details: they affect the meaning and safety of operations.

## Dtype reveals the limits of a float pointer

The sixth post begins from the feeling that the tensor foundation was finally solid, then describes pulling it apart again. A tensor that is always an array of floats has one numerical world. Supporting Scalar, AVX2, Vulkan and CUDA requires agreement about what happens when types meet, how reductions accumulate and how conversions behave. Otherwise identical operator calls could mean different computations on different backends. [P6]

KES-003 establishes shared numerical policy. Byte size is only one part. Promotion chooses a common operand domain; accumulation chooses an intermediate domain for repeated arithmetic; overflow determines behavior at the domain boundary. A backend implements these rules instead of inventing them. The implementation and its review history are detailed in the numerical-contract chapter.

Dtype also changes addressing. An element stride is meaningful only together with the size of that element. The old float-based expression embeds a multiplication by the host C type's size. The target descriptor instead states byte displacements explicitly and separately records dtype. Element-based strides are mathematically usable when combined with dtype, but they are not the accepted universal physical-address contract for heterogeneous storage. Signed byte strides can describe offsets consistently across element widths and opaque allocations.

## Storage separates interpretation from allocation

A host float pointer cannot universally describe a CUDA allocation, a Vulkan buffer and memory binding, or a mapping whose availability changes with synchronization. Putting every native representation into a central union would force the tensor layer to know each backend's resource vocabulary. The storage design moves those facts behind a provider-owned opaque allocation token. [P6, S2, S4]

A tensor then describes shape, signed byte strides, byte offset and dtype over storage. Storage records capacity, memory-space identity, provider identity and the token. The provider understands allocation and release. This separation preserves the useful view idea from the early interleaved matrix example while removing the assumption that every view can be addressed by ordinary host pointer arithmetic.

This is a frozen architectural direction with implementation still planned in KES-005 onward. The sixth post uses language describing what the system is becoming; it does not establish that provider/storage code already exists in the inspected modular repository.

## The device tag becomes a topology problem

The old `DEV_CPU`/`DEV_CUDA` tag mixes a hardware class with an execution API. A GPU is hardware; CUDA and Vulkan can be different paths to it. Separating physical devices from backend instances initially seemed sufficient. That was an important improvement, but research into partitions, nested targets and virtualized hardware revealed another distinction: the selectable destination for execution need not be a whole physical device. [P6, S5, S6]

MIG can expose several targets backed by one GPU. Level Zero can expose nested subdevices. A guest may expose a target whose physical parent cannot be identified. A device group can represent an aggregate backed by multiple physical roots. These cases caused Execution Target to become a first-class entity and physical backing to become an explicit relation. This was a further refinement of the sixth post's hardware/API separation, not a contradiction in the learning story.

> **SUPERSEDED** The sixth post's simplified physical-device/backend description records the public explanation at that stage. The canonical rule now is Backend Instance → Execution Target, with Execution Target → Physical Device through explicit BACKED_BY relations. The post is historical evidence, not the final topology schema.

## Current Kestrel and the productive gap

The sixth post observes that the old Kestrel can still train XOR while the new one cannot. That is the migration truth, not a joke to edit out of the engineering record. The old engine has an end-to-end training path with narrow assumptions. The new tree has stronger arena and dtype foundations and the beginning of a topology registry, but storage, the v2 tensor descriptor, kernels, planner and training graph remain ahead. [P6, S9]

The same post says there was no repository for that post yet. That is a publication-context statement, not evidence that the local files inspected for this handbook do not exist. Similarly, the phrase “building the runtime topology registry” describes active work; it does not certify discovery or reconciliation code.

The next step is therefore deliberately bounded: store and register topology observations and their evidence. That work continues the original learning method. Daniel is still making a hidden layer explicit—now the distinction between what an API reports and what the runtime can responsibly claim exists. The detailed topology chapter supplies the contract; the final checkpoint specifies where implementation resumes.

# Part II Current code and migration evidence

## Repository checkpoint and evidence

Inspection date: 8 October 2026. Local repository: `/home/kernelghost/Downloads/dev_unpacked/experiments_and_code/c/projects/ML_in_C/Kestrel`. HEAD is `28bea76`, titled “added kestrel numerical types and started kestrel topology builder work.” Earlier recorded commits include `459a5ab` arena lifecycle/failure freeze, `a8fe5d6` directory structure, and `c87ba0c` N-dimensional tensors and hardened arena. [S9]

Modified tracked files include KES-004 discussions, `include/kestrel_core.h`, `include/kestrel_topology.h`, `src/arena.c`, and `tests/test_tensor.c`. Untracked files include the major DOCX/Markdown records, amended topology ticket and tests ticket, `tests/test_topology.c`, Makefile, build outputs, and auxiliary files. A complete manifest and hashes appear in the source register.

| File group | Actual role | Migration implication |
| --- | --- | --- |
| `include/arena.h`, `src/arena.c` | Host arena lifecycle and allocation | Reusable host foundation; not a universal GPU allocator |
| `include/dtype.h`, `src/dtype.c` | Numerical identity queries and rules | Central policy for later operators/backends |
| `include/kestrel_core.h` | Constants, dtype/status identities, assertions | Still contains legacy `device_t`; header says pending removal |
| `include/kestrel_topology.h` | Public records, typed IDs, identity values, opaque owners | No registration or finalize declarations yet |
| `src/kestrel_topology.c` | `reserved` owner structs, create/destroy functions | Owning arrays and canonical snapshot contents absent |
| `include/tensor.h`, `src/tensor.c` | Direct FP32 arena-backed tensor helpers | Replace through KES-005/006/007; do not infer v2 completion |
| `tests/test_topology.c` | Lifecycle and representation tests | Does not prove reconciliation or graph validity |
| Makefile | Arena/tensor test targets only | Dtype/topology suites exist but are outside its default test target |
| README | Earlier single-header engine description | Historical architecture and measurements; stale current build description |

## Verification performed for this edition

The source and tests were copied into an isolated working directory and compiled with C11, strict warnings, `-g -O1`, in debug and `NDEBUG` release modes. All eight runs exited successfully and compilation produced no diagnostics. The original repository was not modified.

| Suite | Debug assertions passed | Release assertions passed | Interpretation |
| --- | --- | --- | --- |
| Arena | 77/77 | 82/82 | Existing lifecycle, allocation, and release-defense tests pass |
| Legacy tensor | 79/79 | 79/79 | Existing FP32 descriptor behavior passes its own tests |
| Dtype | 161/161 | 161/161 | Current query/rule tests pass |
| Topology Slice 1 | 58/58 | 58/58 | Public representation and opaque-owner lifecycle pass |

These are assertion totals reported by each suite, not counts of independent test cases. No GPU training, discovery, reconciliation, sanitizer gate, benchmark gate, or v2 acceptance claim follows from these results. Forced allocation-failure injection is not established by the Slice 1 tests.

The 13 current headers, implementations and tests supplied with the posts match the previously inspected and tested working-tree files byte for byte. The test results above are therefore retained evidence for those bytes, not a claim that unimplemented runtime features were tested. Full source fingerprints appear in the appendix.

## KES-004 implemented and absent features

| Implemented now | Not implemented yet |
| --- | --- |
| Public topology vocabulary; strong final ID wrappers; identity representation | Observation arrays; observation registration; identity-claim registration |
| Opaque builder and topology types; builder create/destroy | Locator registration; relationship-evidence registration; backend observation registration |
| Runtime-topology empty-state destroy; Slice 1 representation/lifecycle tests | Comparator policies; reconciliation; canonicalization; graph validation |
| No live snapshot creation path yet | Transactional finalization; immutable snapshot contents; discovery adapters; query/introspection API |

The topology owner structs currently contain reserved placeholder state. Header representation is real implementation progress, but tests of stack-constructed public records are not tests of a populated registry. Legacy tensor/device_t code remains present because KES-005–007 have not replaced that path.

# Part III Numerical memory and execution contracts

> CURRENT CONTRACT — This part synthesizes S1, S2 and S4 with the inspected source. Arena and dtype query behavior are implemented; storage, the v2 tensor descriptor, capability, planning and training remain future implementation under their owning tickets.

## Product boundaries

### Non-goals for the baseline

- PyTorch API compatibility or a large Python ecosystem.

- Distributed multi-node data/model parallel training.

- Full compiler stack, ahead-of-time graph compiler, or whole-program fusion system.

- Every possible dtype, operator, neural-network layer, or optimizer.

- Implicit saturation arithmetic.

- Automatic lossy casts that conceal information loss.

- Forcing every allocator to expose host-arena mark/pop semantics.

- Pretending GPU execution is always faster than CPU execution.

- Benchmark results without methodology, baselines, or machine context.

### Design principles

| Principle | Engineering meaning |
| --- | --- |
| Semantics before speed | Correctness and globally defined behavior are hard constraints. |
| Separate identity from mechanism | Device, backend, memory space, allocator, storage, and tensor are distinct concepts. |
| One source of truth | Do not duplicate derivable facts such as integer width when dtype byte size already defines it. |
| Capability ≠ preference | A backend must distinguish what it can do from what it would prefer to do. |
| Measured planning | Performance profiles are measured per machine/backend context; planner policy does not hard-code “GPU wins.” |
| Explicit loss | Lossy/narrowing conversions require explicit action. |
| Transactional failure | Initialization/allocation failures must not leave partially mutated state where the contract says otherwise. |
| Evidence closes tickets | Tests, benchmark tables, sanitizer logs, and README artifacts are part of implementation, not optional documentation. |

## Shared invariants and module boundaries

- Backends MUST NOT redefine dtype promotion, accumulation, tensor addressing, view semantics, or global contiguity.

- Tensor shape is logical; padding and backend-preferred row pitch MUST NOT mutate logical shape.

- Tensor strides are expressed in bytes.

- A tensor may reference storage; storage MUST NOT maintain reverse ownership of tensors in the baseline architecture.

- Memory-space accessibility and backend/storage compatibility are distinct questions.

- An unavailable backend is not a fatal test-suite failure; backend-specific tests MUST skip cleanly when prerequisites are absent.

- Runtime-private scratch may use internal lifetime rollback only when the runtime exclusively owns all affected allocations.

- The runtime MUST NOT secretly pop/reset a developer-controlled shared arena.

- Promotion is symmetric for every legal pair.

- Accumulation is separate from promotion; widening reduces numerical risk but does not promise overflow impossibility.

- Integer arithmetic semantics are deterministic fixed-width modular wraparound; C signed-overflow UB MUST NOT leak into public semantics.

- Planner legality precedes cost optimization. An invalid plan is never chosen even if measured faster.

- Forced expert execution MUST either honor the requested legal backend instance or fail clearly; it MUST NOT silently substitute another backend.

- Benchmark kernel time MUST NOT hide host↔device transfer or synchronization cost.

### Boundary rules

- `kestrel_core.h` owns primitive cross-cutting identities such as dtype enums; behavior lives in subsystem modules.

- Dtype code does not know backend details.

- Storage code does not know tensor shape/strides/operators.

- Tensor descriptors do not contain backend-native handles.

- Backend adapters may interpret provider-native storage tokens only through their compatibility/provider contracts.

- Planner reads capability/performance data; it does not become the implementation location for kernels.

- Autodiff records semantic dependency and invokes operators/backward rules; it does not own hardware-specific scheduling policy.

## KES-001 Arena lifecycle and review history

> IMPLEMENTED / FROZEN — The early bump allocator made shared lifetimes convenient. KES-001 made those lifetimes and failure cases explicit. Its review verdict is APPROVED / MERGED; the local checkpoint and test results are separate evidence.

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

- arena base alignment is 64 bytes; each allocation meets its validated requested alignment up to 64 bytes;

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

The important review change was treating assertions as diagnostics rather than the entire safety mechanism. Debug misuse should be visible, while release code must still avoid advancing an invalid offset or freeing an invalid state. Capacity rounding and alignment arithmetic must be overflow-safe before any state is committed. A mark contains an offset rather than an ownership generation: after reset/reuse, a numerically admissible old mark cannot by itself prove lifetime validity. Callers remain responsible for mark scope and exclusive ownership. Optional poisoning is a future debugging aid, not default zeroing or a substitute for ownership.

## KES-003 Numerical meaning above backends

> IMPLEMENTED / FROZEN — The dtype layer answers classification, size, promotion and accumulation queries. The modular arithmetic and conversion policies constrain later kernels; query implementation does not mean those kernels or all conversions exist.

### Core dtype set

| Category | Dtypes |
| --- | --- |
| Boolean | BOOL |
| Floating | FP16, BF16, FP32, FP64 |
| Signed integer | INT8, INT16, INT32, INT64 |
| Unsigned integer | UINT8, UINT16, UINT32, UINT64 |
| Sentinels | INVALID=0, COUNT terminal sentinel |

### Byte size

| Dtype(s) | Bytes |
| --- | --- |
| BOOL, INT8, UINT8 | 1 |
| FP16, BF16, INT16, UINT16 | 2 |
| FP32, INT32, UINT32 | 4 |
| FP64, INT64, UINT64 | 8 |
| INVALID, COUNT, invalid enum | 0 / invalid query result per API contract |

Integer bit width is derived from byte size × 8. A separate width source of truth is not required.

### Promotion

| Pair class | Rule |
| --- | --- |
| Same float | unchanged |
| FP16 + BF16 | FP32 |
| FP16/BF16 + FP32 | FP32 |
| Any float + FP64 | FP64 |
| Same integer signedness | wider same-signedness type |
| Mixed signed/unsigned; signed wider | signed type |
| Mixed; equal width | next wider signed type |
| Mixed; unsigned wider | signed type wider than unsigned if one exists |
| No wider signed type | INVALID implicit promotion |
| BOOL numeric pair | INVALID |

### Integer + float promotion

| Integer width | With FP16/BF16 | With FP32 | With FP64 |
| --- | --- | --- | --- |
| 8-bit | existing float | FP32 | FP64 |
| 16-bit | FP32 | FP32 | FP64 |
| 32-bit | FP64 | FP64 | FP64 |
| 64-bit | INVALID | INVALID | INVALID |

Rationale: choose the smallest supported floating type that can exactly represent the full integer operand domain while not narrowing the existing floating operand.

### Accumulation

| Input dtype | Accumulator |
| --- | --- |
| FP16 | FP32 |
| BF16 | FP32 |
| FP32 | FP32 |
| FP64 | FP64 |
| INT8 | INT32 |
| INT16 | INT64 |
| INT32 | INT64 |
| INT64 | INT64 |
| UINT8 | UINT32 |
| UINT16 | UINT64 |
| UINT32 | UINT64 |
| UINT64 | UINT64 |
| BOOL | INVALID / no generic numeric accumulation |

For mixed-input accumulation-heavy operations: promote operands first, then take the accumulator dtype of the promoted dtype.

### Overflow

- Floating overflow follows IEEE-style infinity/NaN behavior where appropriate.

- Integer arithmetic uses deterministic N-bit modular wraparound.

- No normal implicit saturation.

- Later backend implementations must avoid relying on undefined C signed overflow.

- Accumulator overflow obeys the accumulator dtype’s normal overflow semantics.

The default distinction retained in S4 is ordinary arithmetic output in the promoted dtype and accumulation-heavy output in the accumulation dtype, with an explicit downcast when a narrower result is wanted. BOOL is for logical operations, comparisons and masks; it does not enter generic numeric promotion or accumulation. Mixed inputs promote first, then select the accumulator. For example INT8 plus UINT8 promotes to INT16, whose accumulation type is INT64.

Promotion asks which common domain represents the input types; accumulation asks where repeated intermediate arithmetic is performed; overflow defines behavior at the selected domain boundary. They cannot be collapsed into one rule. Widening reduces risk without making overflow impossible. INT8 modular 127 + 1 becomes −128; UINT8 255 + 1 becomes 0. Future C kernels must implement this without invoking signed-overflow undefined behavior. Saturation, if later needed for quantization, is explicit specialized behavior.

## How the dtype review established the contract

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

The review also made classification names precise: signed/unsigned queries are integer-specific even though floating types represent negative numbers. INVALID is zero; COUNT follows the real enum values and is never a dtype. Deriving bit width from byte size avoids a second table that can drift. Validity and BOOL rejection must precede the equality shortcut, because equal invalid values do not become valid by being equal.

## Storage providers and ownership

Memory space:

- describes where bytes physically live;

- examples: system/shared RAM, NVIDIA-local VRAM;

- identity, classification, capacity and execution-target associations belong here;

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

Opaque token rule: The provider interprets native handles/pointers. Core storage must NOT become a union of host_ptr / cuda_ptr / VkBuffer / etc.

Storage does NOT know:

- tensor shape;

- dtype;

- strides;

- list of tensors referencing it;

- backend operation;

- planner.

Relationship is one-way: tensor -> storage.

No storage offset field for tensor views; tensor owns logical offset_bytes. Provider-internal allocator suballocation details stay provider-internal unless future requirements prove otherwise.

## Tensor migration field by field

> **IMPLEMENTED / LEGACY** The present `tensor.h` and `tensor.c` are functional FP32 tensor code with arena-backed allocation and view-based transpose. Their passing tests prove that contract. They do not prove the v2 storage-based descriptor.

| Field or concern | Working legacy representation | Target v2 representation | Why it changes |
| --- | --- | --- | --- |
| Payload | `float *data` | Reference to storage | Native resource representation belongs to the provider |
| Rank | `ndim` | `ndim` | Logical dimensionality remains necessary |
| Shape | `shape[6]` | Logical `shape[6]` | Padding never changes logical extents |
| Strides | Unsigned element increments | Signed `stride_bytes[]` | Explicit byte addressing supports element widths and reversal |
| Origin | Pointer to the view's data | `offset_bytes` from storage origin | A view origin need not be a host-dereferenceable pointer |
| Count | Cached logical element count | Logical count derived/validated from shape | Count cannot stand in for reachable byte extent |
| Numerical type | Implicit FP32 | `dtype` | Central numerical rules apply across providers/backends |
| Device | `device_t` | No direct device tag | Placement is described through storage and runtime topology |

The target descriptor is conceptual; this chapter does not invent its future public constructor names or ABI. Storage owns byte capacity, while a tensor owns interpretation and offset. The legacy device enum remains present in `kestrel_core.h`, marked pending removal. New runtime code must not extend that enum as the new topology model. [S1, S2, S4, S9]

### Addressing and complete reachability

For each dimension, a legal logical index contributes its index multiplied by the signed byte stride. The starting byte is the tensor's offset from the storage origin plus the sum of those contributions. An element occupies its full dtype width beginning at that byte; validating only the first byte is insufficient.

To validate positive extents, compute each dimension's end displacement `(shape[d] - 1) × stride_bytes[d]`. Negative displacements contribute to the minimum; positive displacements contribute to the maximum. Add the tensor offset to each bound, then include the complete element width in the upper bound. Every multiplication, addition and conversion must be checked for overflow. The resulting range must lie inside storage capacity. Empty-shape policy and exact public error reporting must follow the accepted descriptor ticket rather than being inferred from this formula.

A five-element FP32 reverse view can begin at byte offset 16 with stride −4. Its minimum displacement is −16 and maximum is zero, so the covered bytes are 0 through 19. Starting the same view at offset zero would reach before storage and must be rejected. A zero stride contributes no displacement even when the logical dimension grows: broadcasting increases logical positions without creating independent physical elements.

| Example | Logical count | Reachable byte extent | Possible allocation capacity |
| --- | --- | --- | --- |
| FP32 shape [2,5], strides [32,4] | 10 | Bytes 0 through 51, including the gap between rows | 64 bytes for two padded rows |
| FP32 shape [5], offset 16, stride −4 | 5 | Bytes 0 through 19 | At least 20 bytes |
| FP32 broadcast shape [5], stride 0 | 5 | One 4-byte element | May be much larger |

The first example contains ten values, a 52-byte bounding range and a possible 64-byte allocation. None of those quantities can replace the others. The bounding interval may also include holes that the view never touches. This is why `count × dtype_size` alone cannot validate a general view. [S4]

### Aliasing and view operations

Transpose permutes shape and stride metadata. Slicing changes offset and extents. Reversal negates appropriate strides and moves the origin. Broadcasting introduces zero strides. When representable, these operations share storage rather than copying payload solely to change interpretation. A later operator may still materialize a view because a backend requires a layout; that execution decision does not change the view's mathematical semantics.

Aliasing includes both overlap between separate views and repeated physical locations inside one broadcast view. A write rule must explicitly account for overlap; treating each logical coordinate as independent can overwrite or race on the same bytes. The descriptor contract makes such cases representable, while later operator/capability rules determine legal mutation and materialization. No blanket promise that every in-place operation is safe follows from valid addressing.

### Contiguity and preferred layouts

Global contiguity means meaningful values packed in ordinary row-major order without gaps, reversal, broadcast overlap or permutation that changes the order. Size-one dimensions are ignored for stride constraints because their indices never advance. A backend may prefer aligned row pitch, blocking or tiling, but that preference must not redefine contiguity or alter logical shape.

The legacy contiguity helper checks every stride strictly; the target semantics explicitly handle singleton dimensions. This is a migration difference to test, not an excuse to claim the older helper already implements the newer rule. Base alignment of 64 bytes also does not imply that every row begins at a 64-byte boundary. Universal SIMD padding was rejected because it would make one backend's preference a global storage tax and confuse logical count with physical layout.

## Capability backends and planner

> PLANNED — A capability record is associated with a backend instance and its execution target, with physical backing obtained through explicit relations where relevant. Capability reports legal operator/dtype/layout/provider combinations, hard requirements and availability. Preferences are separately identified; the planner must never mistake a preference for proof of legality.

- Supported operators.

- Supported input/output dtype combinations.

- Supported stride/layout classes and hard constraints.

- Compatible storage providers / memory spaces.

- Execution requirements such as alignment/resource usage.

- Preferences such as preferred layout, tile size class, or work size.

- Availability and initialization status.

Capability answers “can this execute legally?” Performance profiles answer “how well does it execute on this machine?”

### Planner hard constraints

- Operation semantics are valid.

- Backend instance supports the operation and dtype combination.

- Storage/provider compatibility is satisfied or a legal transfer/conversion plan exists.

- Layout/stride requirements are satisfied or a legal repack/materialization step exists.

- User constraints/forced backend requests are respected.

- Memory capacity / allocation feasibility is satisfied.

### Cost model

```text
total_plan_cost =
    compute_cost
  + transfer_cost
  + allocation_cost
  + layout_conversion_cost
  + synchronization_cost
  + launch_or_dispatch_overhead
  + memory_pressure_penalty (where relevant)
```

### User control

| Mode | Behavior |
| --- | --- |
| Automatic | Planner chooses among legal plans. |
| Constrained | User can say CPU-only, no CUDA, prefer backend X, keep storage in memory space Y, etc. |
| Forced / expert | User names a backend instance. Kestrel executes there only if legal; otherwise returns a clear error. |

## Autodiff operators and the training contract

### Reverse-mode autodiff

- Forward operator calls record graph relationships when required.

- Backward starts at a scalar loss (or explicit output gradient for non-scalar outputs).

- Topological order is produced by DFS/post-order or an equivalent algorithm with identical dependency semantics.

- Backward traverses in reverse topological order.

- Gradient contributions accumulate; they are never blindly assigned when multiple paths contribute.

- The diamond graph y = x*x + x is the canonical gradient accumulation regression test.

- Finite differences remain available as a gradient-check oracle; they are not the training method.

### Required operator set

| Class | Operators |
| --- | --- |
| Linear algebra | matmul, bias add |
| Activations | ReLU, sigmoid, tanh |
| Probability / loss support | softmax, cross-entropy |
| Regression loss | MSE |
| Core utility | elementwise add/mul, reductions required by backward rules and losses |

### Optimizers

- SGD

- SGD + momentum

- Adam

### Training loop

```text
for batch in data:
    prediction = model.forward(batch.x)
    loss = loss_fn(prediction, batch.y)
    zero_grad(parameters)
    backward(loss)
    optimizer.step(parameters)
```

### Training correctness

- Gradient checks: central-difference oracle, h=1e-4 for canonical FP32 gradient checks unless an operator-specific rationale is documented.

- Per-op analytical-vs-numerical relative error target < 1e-4 for the baseline FP32 operator suite.

- MNIST 784→256→10 with ReLU, softmax, cross-entropy reaches ≥97% test accuracy on the CPU reference implementation.

- GPU end-to-end training must match CPU test accuracy within 0.5 percentage points for the same declared model, preprocessing, seed policy, and epoch/batch configuration.

The earlier scalar Python prototype is historical evidence of learning the differentiation mechanism. It does not fulfill KES-016. The current modular tree has no training graph, required operator suite or optimizer implementation. The detailed roadmap below separates the graph machinery, backward rules, parameter updates and end-to-end proof.

# Part IV KES-004 The definitive topology contract

> **CURRENT CONTRACT / ACTIVE** KES-004 defines resources and structural relationships. Public vocabulary and opaque-owner lifecycle are implemented. The construction pipeline described below is accepted direction awaiting implementation; it must not be read as a description of a functioning discovery registry. Sources S5–S8 establish the design and S9 establishes the current code boundary.

## Entity meaning and physical granularity

A Physical Device is a root physical compute resource identifiable as a whole at the granularity relevant to execution. Packaging alone does not choose the boundary. A board, socket connector, silicon die, chiplet, PCI function or CPU core is not automatically one Physical Device. A dual-GPU board can contain two roots; a multi-die GPU can remain one root when its execution-relevant identity is one device. A partition is represented as an Execution Target rather than promoted to a new physical root simply because an API can select it.

An Execution Target is the selectable compute destination exposed to the runtime. It can represent an entire physical device, a partition, a nested subdevice, a virtualized target with hidden parentage, or an aggregate. Its record contains ID, compute class and name. It deliberately has no embedded single `physical_device_id`. Backing relations express what is known without forcing a false one-to-one model.

A Backend Instance binds one backend family to exactly one Execution Target. Multiple instances can target the same canonical target when evidence establishes that the API observations refer to that target. Current family vocabulary is Scalar, AVX2, Vulkan and CUDA, plus INVALID. Level Zero informs the model without becoming an implemented backend family in this header. A family name alone is not a runnable destination.

A Memory Space is a relevant domain in which bytes may reside. SYSTEM and DEVICE_LOCAL classify domains; UNKNOWN is a valid absence of classification. INVALID denotes unusable/uninitialized semantic state. Two DEVICE_LOCAL spaces can be different domains. A partition's memory can be device-local without requiring a new fundamental kind. Caches, registers, PCI BARs or every API heap do not automatically become memory spaces merely because they have size or physical existence.

## Records and strong IDs

| Record | Current public facts | Deliberate omission |
| --- | --- | --- |
| Physical Device | Strong ID, compute class, name, vendor | Execution API identity embedded as the device itself |
| Execution Target | Strong ID, compute class, name | One required physical parent field |
| Backend Instance | Strong ID, backend family, target ID | Direct physical-device binding |
| Memory Space | Strong ID, kind, name, capacity-known flag, capacity bytes | Provider token, tensor meaning or performance claims |
| Physical-execution relation | Target ID and physical-device ID | Implicit uniqueness of physical backing |
| Execution hierarchy relation | Child target ID and direct parent target ID | Backend or physical IDs in the hierarchy |
| Execution-memory relation | Target ID and memory-space ID | LOCAL/SHARED/REMOTE semantics |

Final IDs are distinct wrapper types with uint32-valued payloads. Zero is invalid for each ID class. This prevents ordinary accidental interchange of physical, execution, memory and backend IDs in C interfaces. It does not eliminate the need to validate numeric values and references at finalization. IDs are runtime-local identity, not persistent vendor UUIDs or a promise of cross-snapshot stability.

Compute classes contain INVALID, CPU, GPU, ACCELERATOR and UNKNOWN. UNKNOWN permits a valid entity whose class is not known; zero-initialized INVALID is not an alternative spelling for uncertainty. Memory kinds follow the same distinction. Names have capacity 128 and vendor names capacity 64 in the current header. Exact future ABI stability is OPEN; these capacities are observed representation, not a promise never to change them.

For memory capacity, `capacity_known == false` requires `capacity_bytes == 0`. A known quantity and an unknown quantity must not collapse into one interpretation. The record describes the domain, not current free bytes or the allocation feasibility of every provider. Later capability and allocation logic must supply those separate answers.

## Three relations with separate invariants

```text
Execution Target ── BACKED_BY ──> Physical Device
Execution Target ── SUBTARGET_OF ──> Execution Target
Execution Target <── structural association ──> Memory Space
```

BACKED_BY permits zero, one or multiple known physical roots. Zero means the current environment cannot establish backing, not that execution is physically unsupported. Multiple backing edges allow an aggregate target. Duplicate target/root pairs are invalid in the final normalized relation set. Sharing a root does not establish that two targets are the same entity, nor that one is the other's execution parent.

SUBTARGET_OF expresses direct execution subdivision. Each child has at most one direct parent. A target cannot parent itself and the graph must be acyclic. Transitive ancestry is derived from direct edges, not separately asserted as additional direct parents. The relation is not a package-containment graph, an allocation-ownership graph or a physical-device hierarchy.

Execution-memory association is many-to-many and structural only. It does not imply legal access, locality, sharing, compatibility, zero-copy, synchronization freedom, bandwidth or latency. In particular, “associated with SYSTEM memory” does not mean every Vulkan object created in that system is valid for every backend. Those questions require capability and provider contracts; their costs require measurement.

> **SUPERSEDED** Device ↔ MemorySpace and generic LOCAL/SHARED/REMOTE labels mixed structure with capability and performance. The replacement is target–memory association with no such inference. The older ticket's overview graphic also misplaced SUBTARGET_OF beneath Backend Instance; only target-to-target hierarchy is canonical.

## Hardware cases that establish the required expressiveness

### CPU and dual-socket systems

A CPU execution target can have Scalar and usable AVX2 instances. Compiling AVX2 code is not sufficient evidence that the running machine can legally execute it; availability and initialization belong to the appropriate capability/backend path. System memory remains a separate domain. The topology does not encode a universal claim that all CPU accesses have equal cost.

A dual-socket machine can contain two physical CPU roots. The execution API may expose per-root targets, a process-wide aggregate target, or another supported granularity. The model can represent those observations and explicit backing without forcing sockets, cores and targets into the same type. NUMA-related costs and placement policy require later evidence and policy; a backing edge itself supplies neither. Exact discovery granularity is not invented here.

### Boards, dies and integrated systems

A dual-GPU board is not automatically one root because it is removable as one card. Conversely, chiplets or multiple dies need not become separate roots if the exposed execution-relevant hardware identity is a single GPU. The rule follows identity and execution semantics, not visual packaging.

An integrated CPU/GPU SoC may contain separately identifiable CPU and GPU roots whose targets associate with one system/shared memory domain. Shared physical RAM does not erase distinct compute identities. It also does not guarantee interchangeable API resources, coherent access at every moment, no copies or no synchronization. A provider may still need a particular mapping or resource usage contract.

### Ordinary cross-API GPUs and independent GPUs

When CUDA and Vulkan observe a GPU, comparable identity evidence may establish one canonical target with separate backend instances. Enumeration alone cannot establish that result. A matching name, vendor and capacity is insufficient; an independent machine can contain two identical GPUs with the same descriptive properties. The registry must preserve them as distinct unless valid evidence proves sameness.

Physical reconciliation and execution reconciliation remain separate. Two observations can refer to the same physical root yet different execution partitions. Conversely, a backend observation can attach to a canonical target only after its target reference has been resolved. A native handle or API ordinal is useful to the discovery source but is not the final canonical identity.

### NVIDIA MIG and memory domains

MIG establishes the clearest failure of one-GPU/one-target reasoning. Several independently selectable execution targets can be backed by one physical GPU. Their physical locator can be the same PCI function while their execution identities differ. Merging them based on BDF would destroy the partition structure the runtime needs to preserve.

MIG Compute Instances can share a GPU Instance memory domain. The corresponding structural model therefore permits several targets to associate with one Memory Space. It does not create a new memory space merely because there is a new compute target, and it does not infer legal simultaneous access or interoperation from the shared association.

Graphics-capable MIG research strengthens the need for target-specific evidence across APIs. The appearance of graphics/API visibility is not itself proof that two observations denote the same target. It neither requires a new fundamental compute class nor authorizes undocumented UUID equivalence. This is a model requirement motivated by project research, not a claim that a graphics-capable MIG adapter has been implemented or tested in Kestrel.

### Intel Level Zero nested subdevices

A root target can have a child subdevice that itself has children. Keeping only a flat target list would discard direct execution ancestry. SUBTARGET_OF preserves that hierarchy while BACKED_BY can independently connect the targets to the root physical resource. Only direct parents are stored; a grandchild's ancestry can be traversed without assigning it two direct parents.

Nested subdevices also show why one relation cannot serve every purpose. Physical backing can be shared across several hierarchy levels. Sharing that root is not evidence of direct parentage; the discovery API must supply the hierarchy relation. Level Zero is a stress case for the schema, not a current execution backend implementation.

### SR-IOV and partial visibility

On a host, discovery may provide evidence connecting a virtual function or exposed execution target with underlying physical hardware. The registry records only supported relationships, without equating every PCI function with a root Physical Device. In a guest, the physical parent may be hidden. A useful target must remain representable with zero known BACKED_BY edges.

Inventing a synthetic “known parent” would convert uncertainty into false identity. UNKNOWN classification and absent evidence are legitimate states. Exact adapter policies remain future work, including which observations a given host or guest API can provide.

### Device groups and aggregates

A Vulkan device group motivates a selectable aggregate target with several backing roots. The representation permits this without claiming that distributed execution, partitioned kernels or a multi-GPU planner already exists. Independent GPUs without a reported aggregate remain separate targets; mere coexistence does not create a group.

The schema's generality is a boundary-preservation measure. Later capability can say whether an operation is legal on an aggregate, and later planning can choose a strategy. Neither answer is encoded by BACKED_BY multiplicity alone.

## Discovery produces observations rather than truth by enumeration

```text
Discovery sources
      ↓
Physical / Execution / Memory observations
      ↓
Typed identity claims + locators + relationship evidence
      ↓
Reconciliation
      ↓
Canonical entities and observation-to-final-ID maps
      ↓
Normalized relationships and retained metadata
      ↓
Validation
      ↓
Immutable RuntimeTopology
```

Sources include system/hardware discovery, management APIs and execution APIs. A management source can observe hardware or relationships without providing an execution backend. Registration stores what was observed; it must not prematurely turn every observation into a permanent entity. Physical, execution and memory observations need separate temporary ID types, distinct from final runtime IDs.

Multiple observations may map to one canonical entity after reconciliation. A physical observation cannot be merged into an execution observation simply because both describe aspects of one GPU. Their types answer different questions. Backend observations refer to the execution observations they can use and are later rewritten against canonical targets.

| Evidence category | Question answered | Example and constraint |
| --- | --- | --- |
| Identity claim | Which entity is this within a namespace? | Target UUID under a documented namespace and comparison policy |
| Physical locator | Which physical location is associated with this observation? | PCI BDF can correlate physical hardware but cannot generally identify a MIG target |
| Explicit relationship evidence | Which structural relation did the source report? | MIG backing or a direct Level Zero parent relationship |

Two MIG execution observations can carry different target identities and the same BDF. That is consistent evidence of different targets associated with one physical location. It is not an identity conflict. Reconciliation answers sameness; association answers backing or hierarchy. Keeping these operations separate prevents a locator from silently becoming a universal merge key.

## Identity representation and comparison policy

> **IMPLEMENTED** The current public identity value contains `identity_kind`, `uint8_t length`, and `uint8_t bytes[64]`. This representation exists in `kestrel_topology.h`. The older report's candidate wording is SUPERSEDED at the representation level.

The current vocabulary includes PCI_BDF, LINUX_DRM_DEVICE, NVIDIA_GPU_UUID, CUDA_DEVICE_UUID, VULKAN_DEVICE_UUID, NVIDIA_MIG_UUID and LEVEL_ZERO_DEVICE_UUID. These are namespace labels, not a complete legal comparison table. Equal byte lengths or UUID-shaped values do not make two namespaces interchangeable.

> **OPEN** Exact per-kind encodings, legal namespace-comparison rules, concrete memory identity namespaces and future ABI guarantees remain unresolved. The fixed buffer does not settle any of these policies. No undocumented CUDA/Vulkan/Level Zero equivalence is asserted in this handbook.

Names, vendor/model strings, capacities, API enumeration order, CUDA ordinals and NVML indexes are diagnostic or source-local facts, not sufficient identity. Discovery order must not decide semantic truth. There is no “first backend wins” rule for choosing between contradictory authoritative claims.

| Outcome | Evidence meaning | Required action |
| --- | --- | --- |
| MATCH | Comparable evidence establishes one entity | Associate observations with one canonical entity |
| DISTINCT | Comparable evidence establishes different entities | Keep entities separate |
| INDETERMINATE | Evidence proves neither identity nor distinction | Do not merge |
| CONFLICT | Authoritative evidence disagrees | Finalization must fail until the conflict is explicitly resolved |

A legal comparator policy must state which kinds can be compared in which context. A generic byte equality helper cannot supply that policy. Memory observations retain an identity seam so future sources can reconcile one domain, but invented memory namespace constants would create false certainty. Introduce them only when real discovery semantics justify them.

## Metadata that survives construction

Final core records remain compact. Selected normalized identity metadata lives in separate tables keyed by physical, execution or memory IDs. It can support later introspection and traceability without embedding every vendor field into every record. The public metadata types exist now; populated immutable tables do not.

Raw observations, source-local handles, temporary observation IDs, redundant evidence and resolver scratch state end with construction. Canonical entities, normalized relationships and selected normalized identity metadata survive. Native handles such as VkPhysicalDevice values, CUDA ordinals or management handles must not masquerade as durable canonical topology metadata. Later backend ownership of operational resources is a separate contract.

## Opaque owners and transactional finalization

Amendment A replaces public by-value owners and public wrappers containing `impl*` with true incomplete struct types owned through pointers. NULL is the empty owner state. Visible record structs are still public data vocabulary; opacity applies to owning arrays, capacities, indexes and construction machinery.

```text
NULL builder -- create --> LIVE mutable builder
LIVE builder -- finalize succeeds --> NULL builder + immutable topology
LIVE builder -- finalize fails --> valid LIVE builder + NULL topology
LIVE owner   -- destroy --> NULL owner
```

Current create rejects a null output-pointer location with INVALID_ARGUMENT and an already-live output with INVALID_STATE, preserving the existing pointer. Allocation failure returns OUT_OF_MEMORY without committing an owner. Successful create assigns the new builder. Destroy accepts null outer pointers and null owners, frees a live owner and writes NULL. Runtime-topology destroy is tested in the empty state; no finalize path currently constructs a live snapshot.

Future finalization performs every fallible allocation, comparison, normalization and validation before ownership transfer. A failure leaves a valid, destroyable builder and an unchanged/null topology output. The commit stage must not half-transfer arrays. Success consumes the builder and publishes an immutable snapshot. This extends the arena's transactional-failure discipline to a graph of owned records.

Validation must establish valid nonzero IDs, existing references, valid enum values, correctly targeted backend instances, deduplicated canonical relationships, unique direct parents, no self-parenting or cycles, consistent memory capacity metadata and coherent retained identities. Conflicts must not be downgraded to arbitrary selection. Relationship normalization rewrites temporary observation references through the canonical maps before graph checks.

Read-only queries must expose physical devices, targets, backend instances, memory spaces, relations and retained identity metadata without mutable internal arrays. Exact public function names are OPEN. Immutability permits a later snapshot-publication design, but hotplug behavior, generations and cross-snapshot identity lifetime are not frozen by this chapter.

## The implementation boundary and completion gate

Slice 1 supplies the record vocabulary, final ID wrappers, identity representation, opaque types, builder create/destroy and empty-topology destroy. Its owner implementations currently contain reserved placeholder state. Its tests intentionally cover representation and lifecycle. They do not store observations or demonstrate reconciliation, allocation-growth rollback, finalized graph validation or introspection.

KES-004 is complete only when observation/evidence registration, explicit comparison policy, reconciliation, canonical relation construction, validation, transactional finalize and immutable queries work together and pass their tests. Synthetic scenarios must exercise CPU-only systems, shared-memory CPU/GPU systems, independent GPUs, multiple backends, partitions, hierarchy, unknown backing, aggregates and conflicts. Available real topology dumps supplement rather than replace synthetic tests.

Topology discovery latency is informative startup evidence, not a fabricated speed target. Unavailable APIs must fail or skip gracefully under the agreed policy. Tensor allocation, kernels, cost optimization and provider mechanics remain outside KES-004. Finishing a broader schema does not silently complete KES-005.

# Part V Completing core v2

> PLANNED except ACTIVE KES-004 — This is the completion sequence, not a list of already available modules. S1 supplies the detailed phase deliverables and acceptance criteria. Its earlier flat-device wording and NEXT status for KES-004 are replaced here by the accepted topology model. Backend phases share foundations; their order is an engineering rollout sequence, not a claim that CUDA technically requires Vulkan.

## Why each stage depends on the earlier work

Topology identifies resources. Storage makes allocation identity explicit. Tensor descriptors interpret storage safely. Views and broadcasting establish layout/alias semantics. Capability determines legal execution. Scalar supplies a semantic reference; optimized backends preserve it. Profiles measure costs. The planner filters legality before estimating cost. Heterogeneous execution integrates movement and materialization. Autodiff and neural-network operators build on that coherent path. Optimizers train, MNIST proves the product, and release validation proves reproducibility and honest performance. [S1]

| Ticket | State | Dependency and acceptance focus |
| --- | --- | --- |
| KES-004 Topology | ACTIVE | Foundations → canonical, validated, queryable immutable topology |
| KES-005 Storage | PLANNED | Topology → provider-agnostic allocation, ownership and transactional failure |
| KES-006 Tensor | PLANNED | Dtype/storage → descriptor and complete byte reachability validation |
| KES-007 Views | PLANNED | Tensor → transpose/slice/reverse/broadcast and alias-aware contiguity |
| KES-008 Capability | PLANNED | Topology/storage/layout → authoritative legal dtype/op/layout/provider combinations |
| KES-009 Scalar | PLANNED | Tensor/capability → correct reference operator behavior |
| KES-010 AVX2 | PLANNED | Scalar/capability → numerical agreement, tails and measured CPU gains |
| KES-011 Vulkan | PLANNED | Runtime foundations/Scalar → cross-vendor conformance and measured execution |
| KES-012 CUDA | PLANNED | Runtime foundations/Scalar/NVIDIA environment → transfers, tiled GEMM, reference comparison |
| KES-013 Profiles | PLANNED | Available backends → machine-specific reusable measurements |
| KES-014 Planner | PLANNED | Capability/profiles/multiple implementations → legal explainable cost-based choices |
| KES-015 Integration | PLANNED | KES-004–014 → coherent execution, transfers and layout conversion |
| KES-016 Autodiff | PLANNED | Stable execution → reverse graph traversal and accumulated gradients |
| KES-017 NN ops | PLANNED | Autodiff/runtime → required forward and analytical backward operators |
| KES-018 Training | PLANNED | Graph/operators → SGD, momentum, Adam and explicit loop |
| KES-019 MNIST | PLANNED | Training primitives → canonical CPU/GPU quality gates |
| KES-020 Release | PLANNED | All prior phases → performance, sanitizer and reproducibility evidence |

The detailed phases below retain deliverables, contracts, tests, pass/fail criteria, evidence, risks and exclusions. This is a dependency sequence, not a release calendar. KES-004 semantics are defined in the preceding topology chapter.

## The dependency behind every transition

| Transition | Engineering reason |
| --- | --- |
| Topology → storage | Allocation identity needs stable memory-space and resource references. |
| Storage → tensor descriptor | A view needs a capacity and owner against which byte reachability can be validated. |
| Descriptor → views/layout | Transpose, slicing, reversal and broadcast transform an already valid addressing contract. |
| Views/layout → capability | Legality must describe the actual stride and provider combinations an operator may receive. |
| Capability → Scalar | The reference backend needs an authoritative declaration of supported semantics and constraints. |
| Scalar → AVX2 | Optimization needs a correct oracle, including tails and non-ideal shapes. |
| AVX2 → Vulkan | The rollout moves from CPU optimization to resource/dispatch/synchronization integration; Vulkan shares core prerequisites, not AVX2 internals. |
| Vulkan → CUDA | The second GPU path tests that provider and capability boundaries generalize; CUDA is not implemented through Vulkan. |
| Backends → performance profiles | Cost estimates must come from measured legal implementations and movement paths. |
| Profiles → planner | Choosing among legal strategies requires costs that include more than kernel time. |
| Planner → heterogeneous integration | A chosen strategy must become a correct sequence of allocation, movement, conversion and execution. |
| Integration → autodiff | Forward values and gradient tensors need reliable lifetime and operator execution. |
| Autodiff → NN operators | Each semantic operator needs a correct backward rule attached to the graph. |
| NN operators → optimizers/training | Parameter updates depend on trustworthy gradients and explicit optimizer state. |
| Training → MNIST | A fixed end-to-end workload exposes integration errors that isolated tests miss. |
| MNIST → final validation | Correct learning is necessary before performance, portability and reproducibility can close the release. |

## KES-004 completion evidence

The definitive topology chapter owns KES-004 semantics and its construction pipeline. Acceptance requires internally consistent queryable records; no legacy device_t in new runtime logic; several backend instances on a proven target; partial API availability handled gracefully; synthetic partition/hierarchy/aggregate/unknown-parent cases; and readable topology dumps. Record discovery startup latency without inventing an aggressive target. Capture the local CPU/Iris Xe environment and, where available, an NVIDIA environment. Allocations, tensors and kernels remain out of scope.

## Phase 2 — Allocator and Storage Core

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-005 |
| Purpose | Create the physical-allocation layer that tensors can reference without embedding host/GPU-native handles in tensor metadata. |
| Prerequisites | KES-004 topology. |
| Blocks | KES-006 tensor core and backend/storage compatibility. |

### Scope and deliverables

- Generic allocator/provider interface sufficient for host arena-backed storage and later GPU providers.

- `storage_t`-equivalent core record: byte_size, memory_space, provider identity, opaque token.

- Host storage provider backed by the merged arena where appropriate.

- Clear ownership and destroy/release path per provider.

- Storage compatibility query hooks needed by backend capability logic.

### Contracts to freeze

- Each allocation belongs to exactly one memory space/provider.

- Core storage does not know dtype, shape, strides, operation, graph, or planner.

- No core union of host/CUDA/Vulkan native handles.

- Provider token interpretation remains private.

- Host arena semantics are not imposed on GPU providers.

### Required tests

- Create/release host storage.

- Out-of-memory/failure leaves caller-visible state deterministic.

- Zero-byte and invalid alignment policy tests.

- Multiple storages from same provider and independent release/lifetime tests.

- Compatibility query negative cases.

### Pass / fail acceptance criteria

- Host storage can be created, written/read through the authorized host-provider path, and released without leaks.

- Storage metadata remains valid and provider-agnostic.

- ASan/UBSan-clean host storage tests.

- No tensor-specific fields added to storage.

### Benchmarking and measurements

- Allocation latency and throughput may be recorded but are not optimization gates yet. Later KES-013 will benchmark provider allocation formally.

### Required evidence and artifacts

- Provider contract header/docs, storage unit tests, sanitizer logs.

### Known risks and failure modes

- Leaking provider-native details into core.

- Double ownership between allocator and storage.

- Attempting to make all providers look like mark/pop arenas.

### Explicitly out of scope

- GPU providers, transfer engine, tensor views.

## Phase 3 — Tensor Descriptor and Validation

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-006 |
| Purpose | Implement the logical tensor descriptor over storage with robust byte-address validation. |
| Prerequisites | KES-003 dtype, KES-005 storage. |
| Blocks | KES-007 views; all operators/backends. |

### Scope and deliverables

- Tensor descriptor with ndim, shape, byte strides, byte offset, dtype, storage reference.

- Default dense row-major descriptor construction.

- Validation of ndim, shape extents, dtype validity, storage presence, and reachable byte range.

- Logical element-count helpers with overflow-safe arithmetic.

- Queries needed by operators and backends.

### Contracts to freeze

- MAX_DIMS=6 baseline.

- Byte strides only.

- Shape remains logical.

- Validation uses both minimum and maximum reachable byte displacement.

- No backend/device field in tensor.

### Required tests

- Contiguous FP32 tensors across 0D/1D/2D/... supported dimensions as contract permits.

- Invalid dtype and invalid storage rejection.

- Overflowing element-count/byte-size arithmetic rejection.

- Offset at exact last legal element boundary.

- Negative-stride-compatible validation primitives even if full view constructors land in KES-007.

### Pass / fail acceptance criteria

- Dense tensor construction produces mathematically correct byte strides.

- Every valid descriptor stays within storage bounds.

- Every tested underflow/overflow/reachability violation is rejected deterministically.

- No payload copy is required merely to construct a descriptor.

### Benchmarking and measurements

- Construction/validation microbenchmarks optional only; correctness dominates.

### Required evidence and artifacts

- Unit/property tests, documented address formula, examples for padded rows and offsets.

### Known risks and failure modes

- Using unsigned-only arithmetic that cannot reason about negative strides.

- Confusing logical count with physical span.

- Implicitly deriving tensor placement from storage without preserving separation.

### Explicitly out of scope

- Broadcast/view constructors, backend-preferred materialization, kernels.

## Phase 4 — Views, Strides, Broadcasting, and Contiguity

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-007 |
| Purpose | Provide zero-copy tensor transformations and global layout semantics required by operators and autodiff. |
| Prerequisites | KES-006 tensor descriptor. |
| Blocks | KES-008 capability layout reporting; operators/backends/autodiff. |

### Scope and deliverables

- Transpose/permutation views.

- Slice views with adjusted offset/strides.

- Reverse/flip using negative strides.

- Broadcast views using zero strides.

- Global contiguity query.

- Alias-awareness rules for zero-stride writes.

- Helpers for materialization requests without coupling to a backend implementation.

### Contracts to freeze

- Views share storage by default.

- Negative strides are legal when reachability remains in bounds.

- Zero stride means repeated logical coordinates may map to one physical element.

- Size-1 dimensions do not invalidate contiguity due solely to arbitrary stride.

- Backend-preferred layout does not redefine Kestrel contiguity.

### Required tests

- 2D transpose without copy.

- Positive-step slice, negative-step reverse, zero-stride broadcast.

- Nested view composition.

- Contiguity examples including size-1 dimensions, padded rows, transpose, reverse, broadcast.

- Reachability property tests under random valid shapes/strides within bounded test generation.

### Pass / fail acceptance criteria

- All canonical views address expected source elements exactly.

- No illegal out-of-bounds negative-stride view passes validation.

- Broadcast aliasing is detectable by semantics needed for later write/operator policy.

- Global contiguity results match the frozen definition.

### Benchmarking and measurements

- No hard performance gate; zero-copy view construction should be O(ndim) metadata work and independent of payload size.

### Required evidence and artifacts

- View tests, address-mapping examples, aliasing/contiguity documentation.

### Known risks and failure modes

- Accidentally materializing views.

- Assuming all non-contiguous tensors are slow/invalid.

- Allowing in-place writes to broadcast aliases without defined semantics.

### Explicitly out of scope

- Full materialization/repack engine, kernels.

## Phase 5 — Backend Capability and Introspection

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-008 |
| Purpose | Define one authoritative capability model used by planner and advanced users. |
| Prerequisites | KES-004 topology, KES-005 storage, KES-006/007 tensor semantics. |
| Blocks | KES-009..014. |

### Scope and deliverables

- Backend-instance capability structure/API.

- Operator support queries.

- Dtype-combination support queries.

- Layout/stride requirement and preference reporting.

- Storage/provider compatibility reporting.

- Human-readable capability dump.

- Clear distinction between hard requirement and preference.

### Contracts to freeze

- Capabilities attach substantially to backend instance, not only backend family.

- Capability is legality, not speed.

- Preferences cannot silently become hard constraints.

- Planner and introspection consume the same authoritative data.

### Required tests

- Synthetic capability matrices.

- Unsupported op/dtype/layout negative tests.

- Same physical GPU with different Vulkan vs CUDA capability records.

- Introspection consistency tests.

### Pass / fail acceptance criteria

- Every backend instance can answer legality queries without executing a kernel.

- Unsupported combinations fail before launch.

- Capability dumps contain enough information to explain planner legality decisions.

### Benchmarking and measurements

- No performance gates; capability queries should be lightweight and cacheable.

### Required evidence and artifacts

- Capability matrix docs and test snapshots.

### Known risks and failure modes

- Encoding expected performance into capability.

- Overgeneralizing family-level support across different devices/drivers.

- Duplicating planner rules inside backends.

### Explicitly out of scope

- Performance profile, cost model.

## Phase 6 — Scalar Reference Backend

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-009 |
| Purpose | Provide the semantic reference implementation for supported operators. |
| Prerequisites | Tensor core + capability model. |
| Blocks | KES-010 backend agreement; all NN operator validation. |

### Scope and deliverables

- Scalar backend instance for CPU.

- Reference kernels for foundational tensor operations needed by later NN/autodiff work.

- At minimum: copy/materialize, elementwise add/mul, reductions needed by losses, and matmul baseline.

- Correct handling or explicit rejection of supported non-contiguous layouts per capability declarations.

- Deterministic test/reference path used by other backend conformance tests.

### Contracts to freeze

- Scalar backend prioritizes correctness and clarity.

- It must obey global dtype/output/accumulation semantics.

- Unsupported dtype/operator combinations are declared, not guessed.

- No backend-specific semantic shortcuts.

### Required tests

- Per-op known-answer tests.

- Randomized comparison against simple trusted calculations where feasible.

- View/non-contiguous input tests for declared-supported cases.

- Overflow semantics tests for integer kernels once implemented.

### Pass / fail acceptance criteria

- Every declared-supported scalar op passes correctness tests.

- No undeclared silent fallback.

- ASan/UBSan clean.

- Matmul baseline benchmark committed for N=128..2048.

### Benchmarking and measurements

- Naive matmul establishes baseline GFLOP/s table.

- Blocked i-k-j optimization belongs either as a scalar optimization subphase or immediately before AVX2; preserve the original ≥5× N=1024 target over original naive when measured on the same environment.

### Required evidence and artifacts

- Reference output fixtures, benchmark baseline table, sanitizer results.

### Known risks and failure modes

- Optimizing so aggressively that reference readability/correctness suffers.

- Using C signed-overflow UB for integer semantics.

- Ignoring layout declarations.

### Explicitly out of scope

- AVX2 intrinsics, GPU APIs, autodiff graph.

## Phase 7 — AVX2 CPU Backend

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-010 |
| Purpose | Implement SIMD CPU execution while preserving Kestrel semantics and supporting tails/non-ideal shapes correctly. |
| Prerequisites | Scalar backend, capability model. |
| Blocks | KES-013 performance profile and planner comparisons. |

### Scope and deliverables

- AVX2 backend instance for compatible CPU.

- AVX2 matmul and priority elementwise kernels.

- Tail handling for dimensions not divisible by SIMD width.

- Backend-preferred alignment/layout reporting rather than universal tensor padding.

- Fallback/repack plan hooks where legal.

### Contracts to freeze

- AVX2 availability is runtime/compile-time checked as appropriate.

- No global requirement that tensor rows be padded to 8 floats.

- AVX2 cannot redefine contiguity.

- Capability record states hard alignment/layout needs separately from preferences.

### Required tests

- Random scalar-vs-AVX2 comparison across aligned and tail sizes.

- Non-multiple-of-8 dimensions.

- Unsupported dtype/layout rejection.

- Deterministic skip when AVX2 unavailable.

### Pass / fail acceptance criteria

- All declared-supported operations agree with Scalar within declared tolerance.

- Original FP32 backend agreement target 1e-5 for canonical tests.

- No divisibility assertion substitutes for correct tail handling.

- Performance benchmark includes blocked-scalar and AVX2 columns.

### Benchmarking and measurements

- Original target: ≥2× over blocked scalar matmul on the primary CPU environment at representative large size; if not achieved because of bandwidth or hardware limits, provide roofline/bandwidth evidence and require explicit review.

- Report GFLOP/s and % peak where peak calculation is defensible.

### Required evidence and artifacts

- Benchmark table, tail correctness tests, capability dump, roofline note if needed.

### Known risks and failure modes

- Hidden global padding assumptions.

- Alignment faults or undefined loads on tails.

- Benchmarking only favorable divisible sizes.

### Explicitly out of scope

- GPU execution, planner.

## Phase 8 — Vulkan GPU Backend

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-011 |
| Purpose | Add cross-vendor GPU compute and validate Kestrel on the local Intel Iris Xe path. |
| Prerequisites | Topology, storage/provider abstraction, tensor core, capability model, scalar reference. |
| Blocks | KES-013 profiling, KES-014 planner, heterogeneous execution. |

### Scope and deliverables

- Vulkan backend family/instance initialization and device binding.

- Vulkan-compatible storage/provider implementation.

- Upload/download or host-visible/shared-memory resource path as actually supported.

- Foundational GPU kernels needed for operator progression.

- Synchronization and resource-lifetime management.

- Capability reporting by actual device/driver support.

### Contracts to freeze

- Vulkan resource compatibility is distinct from generic memory-space accessibility.

- Backend-native handles remain outside core tensor/storage structs.

- Asynchronous execution must expose correct synchronization boundaries to measurement/planner code.

- No assumption that integrated shared physical memory eliminates all resource-management cost.

### Required tests

- Backend initialization/teardown.

- Buffer allocation and transfer correctness.

- Scalar-vs-Vulkan operator agreement.

- Non-contiguous support or explicit rejection according to capability.

- Repeated execution/resource lifetime stress.

### Pass / fail acceptance criteria

- Local Iris Xe can execute the declared Vulkan operator subset correctly.

- All declared-supported outputs agree with Scalar within operation-specific tolerance.

- Unavailable Vulkan environment skips cleanly.

- No leaked Vulkan resources in repeated stress runs as validated by available validation tooling.

### Benchmarking and measurements

- Report kernel/dispatch, synchronization, transfer/resource preparation, and end-to-end timings separately.

- Compare representative sizes against AVX2; no requirement that Vulkan always win.

### Required evidence and artifacts

- Local benchmark table, capability dump, validation-layer-clean run where applicable.

### Known risks and failure modes

- Treating shared-memory hardware as equivalent to zero-cost access.

- Measuring submission instead of completion.

- Leaking API-specific handles into core.

### Explicitly out of scope

- Full NN training completeness, CUDA-specific work.

## Phase 9 — CUDA Backend

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-012 |
| Purpose | Add NVIDIA-specific GPU execution, device-local storage, explicit transfers, and cuBLAS-referenced benchmarking. |
| Prerequisites | Same core/runtime foundations; remote NVIDIA environment. |
| Blocks | KES-013, KES-014, KES-015, final GPU training. |

### Scope and deliverables

- CUDA backend initialization and backend-instance registration.

- CUDA memory provider/storage integration.

- Host↔device transfer path.

- CUDA naive matmul kernel.

- Tiled shared-memory matmul kernel.

- Elementwise/reduction kernels needed by the supported operator set.

- cuBLAS benchmark/reference integration.

- Optional register-tiled matmul after required gates.

### Contracts to freeze

- CUDA is a backend family, not a device identity.

- CUDA storage/provider handles remain opaque to core.

- Transfers are first-class costs.

- Kernel timing synchronizes correctly.

- cuBLAS is a reference/benchmark, not silently substituted for Kestrel kernels unless a later explicit library-backend ADR allows it.

### Required tests

- Scalar-vs-CUDA correctness for each declared op.

- Naive/tiled matmul correctness at N=128..2048 and irregular sizes.

- Host↔device round-trip tests.

- Repeated allocate/execute/free stress.

- Unavailable CUDA skip behavior.

### Pass / fail acceptance criteria

- All required CUDA kernels agree with CPU reference within declared tolerance; original FP32 matmul tolerance target 1e-4.

- Naive and tiled kernels are benchmarked against cuBLAS at every canonical size.

- Transfer and kernel time are reported separately.

- No claim of beating cuBLAS is required for completion.

### Benchmarking and measurements

- Median kernel latency, GFLOP/s, Kestrel/cuBLAS ratio, H2D/D2H bandwidth/latency, end-to-end operation latency.

- T4 or equivalent is acceptable for development/correctness; more expensive GPUs are reserved for later profile/planner validation where useful.

### Required evidence and artifacts

- Benchmark tables, correctness logs, machine/driver/toolkit metadata.

### Known risks and failure modes

- Asynchronous timing errors.

- Benchmarking only kernels while ignoring dominant transfers.

- Tuning to one GPU then encoding those assumptions globally.

### Explicitly out of scope

- Autodiff/training completeness, distributed training.

## Phase 10 — Benchmarking and Performance Profiles

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-013 |
| Purpose | Turn measurements into an authoritative machine-specific performance model reusable by planner and expert introspection. |
| Prerequisites | At least Scalar+AVX2; Vulkan/CUDA profiles when available. |
| Blocks | KES-014 planner. |

### Scope and deliverables

- Benchmark harness for representative op/shape/dtype/layout cases.

- Warm-up/repetition/statistics implementation.

- Transfer benchmarks by direction and size.

- Allocation, synchronization, dispatch/launch, layout conversion benchmarks.

- Persisted performance profile format.

- Profile key/invalidation logic tied to relevant hardware, backend, driver/runtime, Kestrel/kernel version context.

- Human-readable profile dump.

### Contracts to freeze

- Profile data is descriptive measurement, not semantic capability.

- Async GPU execution is synchronized correctly for completed-time measurements.

- Profile invalidates when conditions that can materially change measurements change.

- Planner and introspection read the same profile.

### Required tests

- Profile save/load round trip.

- Invalidation on synthetic version/device changes.

- Repeated benchmark statistical sanity tests.

- No profile entry for unsupported operation/backend pair.

- Cold-start behavior when no profile exists.

### Pass / fail acceptance criteria

- Profiles can be generated, persisted, reloaded, and invalidated deterministically.

- Each recorded measurement includes sufficient context to reproduce it.

- No kernel-time metric accidentally includes transfer unless explicitly named end-to-end.

- Median and variability fields are present for representative cases.

### Benchmarking and measurements

- The complete benchmark protocol in this part applies.

- Benchmark suite should be runnable in a bounded “quick profile” mode and a more complete calibration mode.

### Required evidence and artifacts

- Profile files, benchmark command docs, sample profiles for local CPU/iGPU and remote NVIDIA.

### Known risks and failure modes

- Overfitting profile to exact shapes with no interpolation strategy.

- Stale profile reuse after code/driver changes.

- Excessive calibration cost.

### Explicitly out of scope

- Planner policy itself.

## Phase 11 — Planner and Cost Model

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-014 |
| Purpose | Choose legal execution strategies using topology, capability, storage/layout state, user constraints, and measured cost. |
| Prerequisites | KES-008 capability, KES-013 profiles, multiple backend implementations. |
| Blocks | KES-015 heterogeneous execution and training runtime integration. |

### Scope and deliverables

- Plan representation.

- Candidate enumeration across backend instances.

- Legality filter.

- Cost estimation components.

- Transfer/repack/allocation/synchronization steps in candidate plans.

- Automatic, constrained, and forced modes.

- Planner explanation output.

### Contracts to freeze

- Correctness and Kestrel semantics are hard constraints.

- Total-plan cost, not isolated kernel speed, drives automatic selection.

- Forced illegal request returns error instead of silent substitution.

- Performance model is measured where data exists and conservative where it does not.

### Required tests

- Synthetic cost-model scenarios.

- Small workload where AVX2 beats GPU due to launch/transfer overhead.

- Large workload where GPU wins.

- Already-resident GPU storage case vs host-resident case.

- Layout-conversion-required case.

- Forced backend legal/illegal cases.

- No-profile fallback behavior.

### Pass / fail acceptance criteria

- Planner never selects an illegal candidate.

- Explanation names chosen backend instance and major cost components.

- On stable representative benchmark cases, chosen plan is within 15% of best measured legal median latency unless documented secondary constraints explain otherwise.

- User constraints are honored exactly.

### Benchmarking and measurements

- Compare planner-selected latency against exhaustive measured legal candidates for a benchmark suite.

- Report prediction error per cost component where possible.

### Required evidence and artifacts

- Planner trace logs, benchmark comparison table, synthetic tests.

### Known risks and failure modes

- Optimizing kernel-only time.

- Circular dependence between planner and benchmark calibration.

- Ignoring storage residency and conversion amortization.

### Explicitly out of scope

- Autodiff/model semantics.

## Phase 12 — End-to-End Heterogeneous Execution

| Field | Value |
| --- | --- |
| Status | PLANNED |
| Ticket(s) | KES-015 |
| Purpose | Integrate storage, tensors, backends, transfers, layout materialization, and planner into a coherent operator-execution path. |
| Prerequisites | KES-004 through KES-014. |
| Blocks | KES-016+ neural-network training layers. |

### Scope and deliverables

- Operator execution entry point that obtains/validates plan and executes steps.

- Transfer/materialization orchestration.

- Backend launch and synchronization handling.

- Result storage/tensor construction.

- Execution tracing for debugging and benchmarks.

- Resident-storage reuse across sequences of operations where legal.

### Contracts to freeze

- Execution follows the plan exactly.

- Intermediate lifetime is runtime-private and cannot invalidate developer-owned allocations.

- No semantic differences from Scalar reference.

- Errors unwind resources deterministically.

### Required tests

- Multi-step operation chains across one backend.

- CPU→GPU→CPU planned sequence.

- Repack/materialize path.

- Failure injection for allocation/transfer/launch errors where feasible.

- Output equality against Scalar reference.

### Pass / fail acceptance criteria

- End-to-end operator chains execute on at least Scalar, AVX2, Vulkan (local) and CUDA (remote when available) according to declared capabilities.

- No resource/lifetime leaks in stress tests.

- Execution trace explains transfers and backend choices.

- Planner-selected plan produces correct output.

### Benchmarking and measurements

- End-to-end latency decomposed into transfer/repack/kernel/sync where applicable.

- Compare planned vs forced backend modes.

### Required evidence and artifacts

- Execution traces, stress results, benchmark breakdowns.

### Known risks and failure modes

- Hidden copies that make planner cost model wrong.

- Intermediate-lifetime bugs.

- Backend error paths leaking resources.

### Explicitly out of scope

- Autodiff graph and optimizer semantics.

## Phase 13 — Reverse-Mode Autodiff

| Field | Value |
| --- | --- |
| Status | PLANNED — v2 extension |
| Ticket(s) | KES-016 |
| Purpose | Implement the core training differentiation mechanism promised by the original Kestrel product specification. |
| Prerequisites | Tensor/operator execution path stable enough for forward values and gradient tensors. |
| Blocks | KES-017 operators/backward rules. |

### Scope and deliverables

- Autodiff node/graph representation.

- Operation identity + parent relationship recording.

- `requires_grad` semantics.

- Topological traversal.

- Backward traversal and gradient accumulation.

- Gradient zeroing/reset lifecycle.

- Finite-difference gradient-check harness retained as oracle.

### Contracts to freeze

- Forward operations build graph only when required.

- Backward traversal respects dependency order.

- Gradient contributions accumulate rather than overwrite.

- Graph lifetime does not outlive tensor/storage dependencies illegally.

- Autodiff remains backend-independent; backward operations execute through normal operator/runtime paths.

### Required tests

- Diamond graph `y = x*x + x` accumulation regression.

- Chain graph and branched graph.

- Reused parameter in multiple paths.

- No-grad tensors do not receive gradients.

- Gradient check harness infrastructure.

### Pass / fail acceptance criteria

- Diamond accumulation produces mathematically correct gradient.

- Backward on canonical graphs is deterministic and leak-free.

- Autodiff records do not embed backend-native state.

- Finite-difference harness can compare analytical gradients for later ops.

### Benchmarking and measurements

- Autodiff overhead benchmark: graph construction and backward scheduling cost measured separately from kernel execution for representative small graphs; informational, not a hard optimization gate.

### Required evidence and artifacts

- Graph diagrams/tests, sanitizer results, gradient accumulation proof test.

### Known risks and failure modes

- Gradient overwrite bug.

- Retaining temporaries longer than necessary.

- Embedding execution-backend assumptions into graph nodes.

### Explicitly out of scope

- Complete NN op backward rules, optimizers.

## Phase 14 — Neural-Network Operators and Backward Rules

| Field | Value |
| --- | --- |
| Status | PLANNED — v2 extension |
| Ticket(s) | KES-017 |
| Purpose | Implement the original required NN operator/loss set with analytical backward definitions and backend execution. |
| Prerequisites | KES-016 autodiff + runtime/operator path. |
| Blocks | KES-018 optimizers and training API. |

### Scope and deliverables

- Forward/backward for matmul.

- Bias add.

- ReLU, sigmoid, tanh.

- Softmax.

- MSE.

- Cross-entropy.

- Reductions/utilities required to express backward passes.

- Broadcast-aware gradient reduction where required.

### Contracts to freeze

- Output dtype follows promotion/accumulation rules.

- Backward semantics are mathematical and backend-independent.

- Broadcasted operands receive correctly reduced gradients over broadcast dimensions.

- Numerically stable softmax/cross-entropy implementation strategy must be documented before merge.

### Required tests

- Per-op known-answer forward tests.

- Per-op gradient checks using central differences.

- Broadcast-gradient cases.

- Backend agreement for every backend that declares the op supported.

- Extreme-value stability tests for sigmoid/softmax/cross-entropy.

### Pass / fail acceptance criteria

- Every required op passes gradient check with relative error < 1e-4 in the canonical FP32 test regime.

- Backend outputs/backward gradients agree with Scalar within declared tolerance.

- No NaN/Inf for canonical numerically stable test cases where finite results are expected.

### Benchmarking and measurements

- Forward and backward latency benchmarked separately for matmul and selected activations/losses.

- Operator benchmarks use the same shape/dtype methodology as runtime profiles.

### Required evidence and artifacts

- Gradient-check report table, backend conformance table, stability tests.

### Known risks and failure modes

- Unstable softmax/cross-entropy.

- Incorrect gradient reduction for broadcasts.

- Backward rules that mutate aliased broadcast views unsafely.

### Explicitly out of scope

- Optimizer state, data loader, full model API polish.

## Phase 15 — Optimizers and Training API

| Field | Value |
| --- | --- |
| Status | PLANNED — v2 extension |
| Ticket(s) | KES-018 |
| Purpose | Provide parameter update algorithms and a minimal explicit training interface. |
| Prerequisites | Autodiff + required NN ops. |
| Blocks | KES-019 MNIST end-to-end. |

### Scope and deliverables

- Parameter abstraction or documented tensor/graph convention for trainable values.

- Zero-grad behavior.

- SGD.

- SGD + momentum.

- Adam.

- Optimizer state allocation/lifetime.

- Minimal training-loop API sufficient for MNIST.

- Deterministic initialization/seed controls needed for reproducible tests.

### Contracts to freeze

- Optimizer math is backend-independent; tensor arithmetic executes through normal runtime.

- Optimizer state dtype/placement policy must be explicit.

- No hidden parameter copies that disconnect graph/optimizer state.

- Zero-grad semantics are explicit.

### Required tests

- One-step numerical update tests for each optimizer against hand-calculated expected values.

- Multiple-step momentum/Adam state tests.

- Zero-grad regression.

- CPU/GPU optimizer consistency on small models where backend support exists.

### Pass / fail acceptance criteria

- All optimizer known-answer tests pass.

- Optimizer state survives multiple steps without leaks or stale references.

- Minimal XOR/small synthetic training smoke test converges before MNIST integration.

### Benchmarking and measurements

- Optimizer-step latency may be measured; not a hard gate. Memory footprint of optimizer state should be reported for representative parameter counts.

### Required evidence and artifacts

- Known-answer optimizer fixtures, convergence smoke test, memory-state documentation.

### Known risks and failure modes

- Incorrect bias correction in Adam.

- Gradient/state placement causing unnecessary transfers.

- Implicit optimizer casts losing precision.

### Explicitly out of scope

- Large model zoo, mixed-precision training policy, distributed optimizer states.

## Phase 16 — MNIST End-to-End Correctness

| Field | Value |
| --- | --- |
| Status | PLANNED — v2 extension |
| Ticket(s) | KES-019 |
| Purpose | Prove that Kestrel is genuinely a neural-network training library rather than only a tensor/runtime project. |
| Prerequisites | All training primitives. |
| Blocks | KES-020 final performance validation/release. |

### Scope and deliverables

- MNIST IDX loader.

- 784→256→10 model configuration.

- ReLU hidden activation.

- Softmax + cross-entropy output/loss path.

- Training/evaluation loop.

- Model/optimizer seed and configuration recording.

- CPU reference training run.

- GPU training path where required backend op coverage exists.

### Contracts to freeze

- Data preprocessing and split policy are fixed/documented for comparable runs.

- The same model semantics are used across backends.

- Accuracy comparison must use identical evaluation procedure.

- Training correctness beats timing; no benchmark result is valid if accuracy gate fails.

### Required tests

- IDX parsing tests.

- Small-subset overfit test to catch learning bugs.

- Full training run.

- Reproducibility check across repeated runs within expected stochastic variance.

- CPU-vs-GPU final accuracy comparison.

### Pass / fail acceptance criteria

- CPU reference test accuracy ≥97%.

- GPU end-to-end training accuracy within 0.5 percentage points of CPU reference for declared canonical configuration.

- Loss decreases meaningfully during training and no silent NaN divergence occurs.

- All training configuration and final metrics are emitted/recorded.

### Benchmarking and measurements

- Report epoch time, total wall clock, examples/sec, final train/test accuracy, peak/observed memory where feasible.

- Report CPU and GPU training with same model/data configuration.

- Report transfer/runtime breakdown for GPU path where instrumentation is available.

### Required evidence and artifacts

- Committed training command/config, final metrics table, learning-curve data, machine specs.

### Known risks and failure modes

- Achieving accuracy by changing model/data configuration between backends.

- Comparing kernel speed while end-to-end training is transfer-bound.

- Gradient instability hidden by small smoke tests.

### Explicitly out of scope

- Convolutional models, distributed training, large datasets.

## Phase 17 — Performance Validation and v2 Release

| Field | Value |
| --- | --- |
| Status | PLANNED — v2 extension |
| Ticket(s) | KES-020 |
| Purpose | Close the project with reproducible benchmarks, planner validation, documentation, and release evidence. |
| Prerequisites | All prior phases. |
| Blocks | Kestrel v2 release. |

### Scope and deliverables

- Complete matmul benchmark table across Scalar/blocked/AVX2/Vulkan/CUDA/cuBLAS as available.

- End-to-end MNIST CPU/GPU training table.

- Planner-vs-best-legal comparison suite.

- Final capability/topology/profile examples.

- README architecture and limitations.

- CI/release matrix.

- No-new-feature stabilization pass.

### Contracts to freeze

- Benchmark methodology frozen for published results.

- All final numbers tied to git commit and machine metadata.

- Known limitations documented explicitly.

- No last-minute feature additions after release-candidate gate.

### Required tests

- Full regression suite.

- ASan/UBSan CPU/core pass.

- Backend conformance.

- Gradient checks.

- MNIST correctness.

- Reproducible benchmark smoke rerun.

### Pass / fail acceptance criteria

- All core-v2 release gates pass.

- Published benchmark tables contain no blank required columns for environments actually used.

- cuBLAS ratios and transfer times are present for CUDA measurements.

- Planner benchmark demonstrates legal choices and near-best measured behavior on representative cases.

- Repository can be built/tested from clean checkout using documented steps.

### Benchmarking and measurements

- All benchmarking standards in this part apply.

- Results are presented with limitations and without unsupported claims.

### Required evidence and artifacts

- Final README, architecture spec, engineering spec, ADR index, benchmark CSV/JSON if used, plots/tables, CI badge/results.

### Known risks and failure modes

- Cherry-picked favorable benchmark runs.

- Unrepeatable local-only environment assumptions.

- Publishing claims stronger than evidence.

### Explicitly out of scope

- New operators/features. These require a post-v2 roadmap.

## Release Gates and Final Acceptance

| Gate | Hard requirement |
| --- | --- |
| G1 — Core correctness | Arena, dtype, storage, tensor/view, capability, and planner core tests all pass. |
| G2 — Backend conformance | Every declared-supported backend operation agrees with Scalar/reference within documented tolerance. |
| G3 — Gradient correctness | Every required differentiable op passes central-difference gradient check at the defined tolerance. |
| G4 — Training correctness | Canonical MNIST CPU test accuracy ≥97%. |
| G5 — GPU training correctness | Canonical GPU training accuracy within 0.5 percentage points of CPU reference on the chosen GPU backend used for final training acceptance. |
| G6 — Sanitizers | CPU/core test suite is ASan/UBSan clean. |
| G7 — Performance evidence | Matmul and end-to-end training benchmark tables are committed with machine/software metadata. |
| G8 — Honest CUDA comparison | cuBLAS reference is measured for canonical CUDA FP32 matmul sizes; transfer time is separate. |
| G9 — Planner validation | Planner never selects illegal plans and is within 15% of best measured legal median on representative stable cases unless documented constraints explain otherwise. |
| G10 — Reproducibility | Clean checkout build/test/training/benchmark instructions are sufficient for a technically competent engineer to reproduce the primary results on compatible hardware. |

### Failure policy

A failed hard gate means the release is not complete. The correct responses are to fix the implementation, revise the architecture through an explicit ADR if the requirement was wrong, or document an environment-specific unsupported path. Silent waivers are not allowed.

## Baseline deferrals and scope boundaries

- FP8 and additional specialized/quantized dtypes.

- Explicit saturating arithmetic operators.

- Mixed-precision training policy and dynamic loss scaling.

- Convolution-heavy model suite and larger datasets.

- Graph compiler/fusion beyond simple planner/materialization decisions.

- Persistent GPU kernel autotuning database beyond baseline performance profile.

- Multi-GPU and distributed training.

- Additional CPU ISAs such as AVX-512/ARM NEON.

- Additional GPU APIs/backends.

- Model serialization/interchange formats beyond what is required for baseline demonstrations.

- Python bindings/high-level ecosystem.

## Required release evidence artifacts

- `bench_matmul` results for canonical sizes and available backends.

- CPU cache/memory evidence for blocked optimization phase where applicable.

- AVX2 speedup and backend agreement table.

- Vulkan kernel/dispatch/sync/end-to-end table.

- CUDA naive/tiled/cuBLAS comparison and H2D/D2H transfer table.

- Planner chosen-vs-best-legal results.

- MNIST CPU/GPU training time and accuracy table.

- Machine/compiler/driver metadata captured with every published benchmark set.

## Suggested Repository Evidence Layout

```text
docs/
  architecture_v2.docx / exported markdown as desired
  engineering_spec_v2.docx
  adr/
bench/
  bench_matmul.c
  bench_transfer.*
  bench_train.c
  results/
    <machine-profile>/
tests/
  test_arena.*
  test_dtype.*
  test_tensor.*
  test_views.*
  test_backends.*
  test_gradcheck.*
  test_training.*
profiles/
  <machine-profile>.json-or-equivalent
```

## Benchmark and validation standard

> PLANNED RELEASE REQUIREMENTS — The following measurements and numerical targets are frozen source requirements or historical optimization targets, not results achieved by this edition. Blank benchmark cells in the source are not zero-valued results.

Benchmarking principle; A benchmark is an engineering experiment, not a marketing number. Every result must state what was measured, on what machine, with which software versions, under which synchronization boundary, and against which baseline.

### Environment capture

- CPU model, core/thread count, relevant ISA support, nominal/base/turbo clocks where available.

- RAM capacity and relevant memory configuration where known.

- GPU model(s), driver version, CUDA toolkit/runtime version, Vulkan runtime/driver version.

- Compiler and version, optimization flags, debug/release mode, sanitizers disabled for performance runs.

- Kestrel git commit and benchmark configuration.

- OS/kernel version.

- Thermal/power mode if it materially affects repeatability.

### Statistical protocol (v2 engineering decision)

- Warm up each benchmark case until one-time initialization is excluded; default minimum is 5 warm-up iterations unless the benchmark is an end-to-end training run.

- Collect at least 20 timed samples for microbenchmarks; 30 is preferred when runtime is short enough.

- Report median as the primary latency statistic. Report p10/p90 or min/max when variance is material.

- For throughput, compute from work divided by measured completed-execution time; do not use asynchronous submission time as completed GPU execution time.

- GPU timings must synchronize at the boundary required to measure actual completion.

- Never mix transfer time into “kernel time.” Report transfer, kernel, synchronization, conversion, and end-to-end separately when each exists.

- Discarded outliers or changed methodology must be documented rather than silently removed.

### Matmul microbenchmark matrix

| Dimension | Required cases |
| --- | --- |
| Square N | 128, 256, 512, 1024, 2048 |
| Dtype | FP32 baseline; additional dtypes as backend support becomes available |
| CPU variants | Scalar naive/reference, blocked i-k-j, AVX2 |
| GPU variants | Vulkan implementation(s), CUDA naive, CUDA tiled/shared-memory, optional register-tiled |
| Reference | cuBLAS SGEMM for CUDA FP32 comparison |
| Metrics | median ms, GFLOP/s, % theoretical peak where defensible, transfer ms, end-to-end ms |

### Original CPU optimization targets

- Blocked/memory-hierarchy phase target: ≥5× over the original naive matmul at N=1024 on the same CPU/build configuration; if the target is missed, the phase may not claim completion without an ADR/spec revision grounded in measured evidence.

- AVX2 target: ≥2× over the blocked scalar version on the declared primary CPU benchmark environment. If memory bandwidth prevents this, the benchmark report must provide roofline/bandwidth evidence and the gate requires explicit review rather than silent waiver.

- Scalar and AVX2 outputs agree within 1e-5 for canonical random FP32 test matrices unless an operation-specific tolerance is documented.

### CUDA benchmarking

- Naive GPU kernel establishes GPU baseline.

- Tiled shared-memory implementation is the required optimized CUDA kernel.

- Register-tiled kernel is optional stretch unless promoted by ADR.

- Every CUDA kernel is checked against the CPU reference with tolerance appropriate to operation/dtype; original FP32 matmul target is 1e-4.

- cuBLAS comparison is required at every canonical matmul size. Report Kestrel/cuBLAS ratio; no requirement to beat cuBLAS.

- Host↔device transfer time is measured separately from kernel execution.

- End-to-end training benchmark reports CPU train time, GPU train time, final accuracy, and transfer/runtime breakdown where measurable.

### Vulkan benchmarking

- Benchmark the same operator/shape/dtype cases supported by the Vulkan backend.

- Report command submission/dispatch overhead separately when measurable.

- Integrated-GPU shared-memory paths must not be described as “zero-copy” unless the actual storage/provider path avoids payload transfer; resource creation/mapping/synchronization cost still counts.

- No hard rule says Vulkan must beat AVX2. The hard gate is correctness plus reproducible measured performance and accurate capability reporting.

### Planner validation benchmarks

- For each representative workload, measure every legal candidate plan offline.

- Compare planner-chosen plan cost/latency to the best measured legal candidate.

- For stable microbenchmark cases, planner should select a plan within 15% of the best measured median latency unless a documented secondary constraint (memory pressure, user constraint, one-time conversion amortization) explains the difference.

- Planner explanation output must identify the chosen backend instance and material cost components used in the decision.

### Required benchmark tables

| Required benchmark rows | Required measurement columns |
| --- | --- |
| Scalar naive; blocked scalar; AVX2; Vulkan; CUDA naive; CUDA tiled; cuBLAS reference | N=512, N=1024, N=2048; GFLOP/s; defensible % peak; transfer ms; end-to-end ms |

## Test and CI Standard

### Test layers

| Layer | Purpose |
| --- | --- |
| Unit | Small deterministic subsystem behavior: dtype, arena, tensor validation, promotion, capability queries. |
| Property/invariant | Symmetry, bounds, contiguity properties, view reachability, planner legality. |
| Backend conformance | Same semantic operation across Scalar/AVX2/Vulkan/CUDA within declared tolerance. |
| Gradient check | Analytical backward rules vs central finite difference oracle. |
| Integration | Storage + tensor + backend + planner + graph interactions. |
| End-to-end | MNIST training and final model accuracy. |
| Sanitizer | ASan/UBSan baseline for CPU code; backend-specific tooling where available. |
| Performance regression | Selected stable benchmark cases tracked against committed baselines without making CI timing overly flaky. |

### CI matrix

| Environment | Required behavior |
| --- | --- |
| CPU-only Linux | Build; arena/dtype/tensor/core tests; Scalar; AVX2 when host supports it; autodiff/training reference tests. |
| CPU without AVX2 or forced-disabled AVX2 | AVX2 tests skip cleanly; Scalar remains functional. |
| Vulkan-capable environment | Vulkan discovery, capability, correctness/conformance tests. |
| CUDA-capable NVIDIA environment | CUDA backend tests, transfer tests, selected cuBLAS comparison smoke benchmark. |
| Sanitizer build | CPU/core tests under ASan + UBSan; zero sanitizer findings. |

### Release-mode misuse and invalid-state tests

- Debug assertions are diagnostic aids, not the sole release-safety mechanism.

- Invalid enums, stale/future arena marks, invalid tensor reachability, unsupported backend requests, and impossible conversions return deterministic failure behavior.

- Tests must include both expected valid behavior and invalid boundary behavior.

# Part VI Beyond core v2

> **PLANNED** The future roadmap describes product direction and proof obligations. None of F1–F7 is implemented merely because its deliverables are detailed. Core-v2 release gates precede this ladder. Source S3 supplies the detailed phase requirements that follow this narrative.

## What mature Kestrel is trying to become

Mature Kestrel is a compact, inspectable C neural-network training system where an engineer can see how model semantics, dtype rules, tensor layouts, storage placement, hardware topology, backend capabilities, execution plans and measured costs connect all the way down to the machine.

That identity connects the future product to the first linear regressor. The original question was what `.fit()` hides. A mature system answers more of that question without requiring every user to reimplement the training loop. It provides useful models while retaining a route from a loss value to a gradient rule, from a view to its backing allocation, and from an operation to the legal candidates and measured costs behind its execution plan.

The model ladder is therefore an architectural validation program. More models are valuable, but each family also exposes a different kind of hidden assumption. A dense classifier alone cannot establish that a graph is general, that arbitrary layouts work, that state survives correctly across steps, or that low-precision training has coherent semantics.

```text
Classical differentiable models → Dense neural networks
             ↓
         Autoencoders → CNNs → Recurrent / sequence models
                                      ↓
                              Small transformers
                                      ↓
                            Mixed-precision training
                                      ↓
                       Whole-training planner optimization
```

### Hardening before expanding

F1 makes repeated training trustworthy: reusable loops, deterministic controls where promised, evaluation/inference behavior, checkpointing and reliable resource lifetimes. These are prerequisites for interpreting later model results. If a run changes because stale gradients or optimizer state survive incorrectly, a new model architecture cannot be evaluated honestly.

Classical linear and logistic regression remain useful demonstrations even after deep models appear. They connect the mature API to the earliest experiments and provide small workloads whose numerical behavior can be inspected directly. A larger neural-network system should not make those simple cases harder to explain.

### Autoencoders prove graph generality

An autoencoder reconstructs its input through an encoder and decoder. It uses training machinery for a purpose other than assigning a class label, so it tests whether the model, loss and graph composition are genuinely reusable. Reconstruction loss, bottleneck representations and decoder gradients expose classifier-specific assumptions that an MNIST MLP could leave untouched. F2 should make the architecture more general rather than special-case one additional example.

### CNNs prove layout and state discipline

Convolution and pooling place spatial dimensions and layout at the center of performance and correctness. Backward rules must agree with the forward operator's padding, stride and shape interpretation. Normalization also introduces mutable training state whose lifetime and evaluation behavior differ from ordinary immutable parameters. F3 tests the tensor/view abstraction, backend layout support and model state together.

The future roadmap can call for CIFAR-family work without inventing an accuracy target. A defensible gate needs a declared architecture, data processing, baseline and metric. Those details remain OPEN where the source does not freeze them. A historical ambition is not a license to put an unsupported percentage into the release criteria.

### Sequence models prove reuse across time

RNN and LSTM workloads reuse parameters across time and propagate gradients through an unrolled dependency structure. Embeddings and indexing, sequence masks, graph lifetime and backpropagation through time all become first-class concerns. F4 asks whether the graph can represent repeated use correctly, release intermediates at the right time and accumulate contributions rather than overwrite them.

This is a different pressure from making one matmul fast. A model may run every individual kernel correctly and still train incorrectly if parameter sharing, masking or recurrent-state boundaries are wrong. Sequence workloads force those boundaries into observable tests.

### Transformers connect views to planning

Small transformers combine batched matrix multiplication, transposes, attention masks, normalization, residual paths, embedding behavior and many short-lived intermediate views. They test the claim that metadata views remain meaningful throughout forward and backward execution and that backend restrictions are handled by explicit plans.

F5 should make those primitives and their validation inspectable. The roadmap does not establish a parameter count or large-language-model scale commitment. A small, reproducible demonstration with explainable attention and correct gradients is more relevant to the stated product identity than an unsupported scale claim.

### Mixed precision makes dtype policy operational

F6 is where the dtype architecture must do more than return correct enum values. Storage dtype, compute dtype, accumulation dtype, master parameter/state representation and conversion decisions interact across forward, backward and optimizer steps. Low-precision speed is useful only if quality is measured against an explicit higher-precision baseline.

The existing promotion and accumulation contracts provide the foundation, but they do not by themselves freeze a complete mixed-precision training policy. Exact quality tolerances, loss-scaling details and backend coverage must be specified and tested. No universal tolerance or speedup is invented here.

### Whole-training planning expands the optimization horizon

F7 moves from selecting a good isolated operator implementation to reasoning about repeated graph execution. Keeping useful data resident, reusing conversions and allocation plans, avoiding unnecessary synchronization and considering forward/backward/optimizer sequences can change total training cost. A locally fast kernel can lose once movement and repeated layout conversion are included.

This phase tests whether the separation between semantics, legal execution and measured cost is useful at training scale. It must preserve numerical correctness and user constraints while making planner decisions explainable. It does not establish automatic multi-device partitioning, dynamic compilation or a distributed system unless those separately scoped directions are accepted.

## A mature Kestrel walkthrough

The following is a PLANNED user journey, not a claim that these commands or APIs exist. An engineer clones the eventual project and begins with the smallest demonstration. Each step should preserve the ability to inspect what the system is doing while adding a new architectural challenge.

| Journey stage | What the engineer learns | What should be inspectable |
| --- | --- | --- |
| Linear regression | A loss changes because parameters follow gradients | Parameter dtype, MSE, gradient graph and optimizer updates |
| Logistic regression | A numerical score becomes a classification objective | Stable loss/activation semantics, labels and dtype conversions |
| MNIST MLP | The core training system works end to end | Matmul layouts, gradients, CPU/GPU comparisons and reproducible accuracy |
| Autoencoder | Graph composition is independent of classification | Encoder/decoder dependencies, reconstruction loss and shared storage/views |
| CNN | Spatial computation makes layout and state visible | Convolution/pooling shapes, normalization state and repack costs |
| LSTM | Parameters and state are reused across time | Recurrent state, masks, gradient accumulation and graph lifetime |
| Tiny transformer | Attention composes demanding primitives | Batched matmul, views/transposes, masks, normalization and attention intermediates |

At every stage, the engineer should be able to follow a value from its logical tensor shape and numerical semantics to the storage that backs it. The runtime should reveal available execution targets, the backend instances that can use them, and why a candidate operation is legal or rejected. The selected plan should explain compute cost, transfer cost, synchronization, conversion and allocation effects, and memory use under declared measurement conditions.

A useful inspection session might compare two legal strategies for one workload. One keeps data in a memory space and uses a compatible backend; another requires a transfer or materialization. The engineer sees those steps and the profile evidence behind the choice, then compares end-to-end training results with a baseline. This is a conceptual journey; exact introspection functions and the presentation interface remain to be designed.

Expert control remains important. An engineer can eventually constrain candidates or force a specific legal backend instance to isolate a problem. An illegal forced request should fail clearly instead of quietly changing the experiment. Automatic planning and manual investigation therefore share the same legality rules.

The future phase reference below retains the concrete deliverables, validation requirements, passing criteria, benchmark obligations, risks and exclusions. These details turn the narrative into work that can be reviewed. The narrative explains why the phases matter; the reference states what must be demonstrated.

## Future Phase F1 — Training System Hardening

Objective: Turn the core training loop into a reusable training platform before adding more model families.

What this unlocks: Reliable experiments, recoverable long-running training, reproducible comparisons, and a stable base for CNN/RNN/transformer work.

Prerequisites: Core v2 end-to-end dense training must already pass its release gates.

### Required deliverables

- Parameter initialization API: zeros, constant, uniform, normal, Xavier/Glorot, and He initialization where mathematically appropriate.

- Explicit train/eval mode propagation so future dropout and normalization layers can change behavior safely.

- Checkpoint format containing model parameters, optimizer state, global step/epoch, dtype metadata, and enough graph/model metadata to resume training.

- Model serialization separate from training checkpointing so inference-only artifacts can omit optimizer state.

- Deterministic random-number subsystem with explicit seed control and backend-consistent reproducibility expectations.

- Mini-batch dataset iterator / dataloader abstraction with shuffling and deterministic seeded order.

- Gradient clipping by value and by global norm.

- Learning-rate scheduler interface; minimum implementations: constant, step decay, cosine decay or equivalent simple schedule.

- Training metrics API for loss, accuracy/custom scalar metrics, step time, examples/sec, and backend/planner trace capture.

- Error policy for incompatible checkpoint versions and dtype/backend mismatches.

### Required validation

- Save/resume test: interrupt training, reload checkpoint, and prove that resumed training matches an uninterrupted reference within deterministic expectations.

- Seed reproducibility test on scalar CPU: same seed produces identical initialization, mini-batch order, and training trace.

- Train/eval test that verifies mode-dependent layers can receive mode state even before dropout is added.

- Gradient clipping unit tests with analytically predictable vectors.

- Serialization round-trip test for all supported trainable parameter dtypes.

### Passing criteria

- All training state needed to resume a supported model is preserved.

- Corrupt or incompatible checkpoint input fails loudly without partial model mutation.

- Reference deterministic training run can be reproduced on the same backend/build configuration.

- No training utility may bypass tensor ownership/storage rules or create a second parameter representation.

### Benchmark evidence

- Checkpoint write/read throughput and file size reported for at least one MLP.

- Training-loop overhead measured relative to raw forward/backward execution; framework overhead must be reported separately.

- Dataloader throughput measured independently from model compute.

### Risks / design cautions

- Checkpoint schema can accidentally become coupled to in-memory C struct layout; use explicit versioned fields instead.

- Reproducibility across different GPU architectures/drivers may not be bitwise; document deterministic scope instead of promising impossible guarantees.

### Explicitly out of scope

- Distributed data-parallel training.

- Cloud checkpoint stores.

- Large dataset streaming frameworks.

## Future Phase F2 — Dense Autoencoders and General Model Composition

Objective: Prove that Kestrel can train non-classification dense networks and compose encoder/decoder graphs cleanly.

What this unlocks: Reconstruction models, representation-learning demos, and a stronger proof that the graph/autodiff system is general rather than classification-specific.

Prerequisites: F1 training utilities plus stable dense training.

### Required deliverables

- Reusable sequential/module composition abstraction if the current API still requires manual graph wiring.

- Parameter enumeration and zero-grad utilities that work across nested model structures.

- Example dense autoencoder: 784 -> 256 -> 64 -> 256 -> 784.

- Optional tied-weight experiment only if the parameter/view semantics support it cleanly.

- Reconstruction visualization/export helper kept outside the core runtime.

### Required validation

- Autodiff tests where one parameter contributes to multiple graph paths.

- Autoencoder gradient checks on a tiny deterministic network.

- Serialization/resume test for encoder+decoder parameters.

- Backend agreement for one forward/backward mini-batch.

### Passing criteria

- Autoencoder training loss falls consistently from initialization on MNIST or an equivalent small image dataset.

- Reconstruction quality is qualitatively non-degenerate and quantitatively better than the untrained baseline.

- No model-specific exceptions are introduced into the planner, tensor, or autodiff core.

### Benchmark evidence

- Examples/sec and step latency compared with the MNIST classifier at similar parameter count.

- Peak memory usage reported for forward and forward+backward.

- CPU AVX2 vs GPU execution time separated into compute and transfer components.

### Explicitly out of scope

- Variational autoencoders unless random sampling/reparameterization is explicitly designed.

- Generative image quality metrics.

## Future Phase F3 — Convolutional Neural Networks

Objective: Add the operator family required for practical image models.

What this unlocks: LeNet-style networks, CIFAR-10 classifiers, convolutional autoencoders, and a major expansion beyond dense-only models.

Prerequisites: Stable tensor views/strides, mature autodiff, backend capability queries, F1 training utilities.

### Required deliverables

- Conv2D forward semantics covering NCHW or another explicitly frozen canonical layout; layout must be documented globally.

- Conv2D backward for input, weights, and optional bias.

- Padding and stride semantics; dilation can be deferred unless deliberately included.

- MaxPool2D and AveragePool2D forward/backward.

- Flatten/reshape path that remains view-based where legal.

- BatchNorm for training and inference, including running statistics and epsilon/momentum semantics.

- Optional dropout once train/eval mode exists.

- Scalar reference convolution first; optimized AVX2/Vulkan/CUDA paths only after correctness.

- Planner capability entries for convolution, pooling, normalization, and layout restrictions.

- Canonical CNN example for MNIST/LeNet and a CIFAR-10-class model.

### Required validation

- Numerical gradient check for Conv2D on tiny tensors.

- Finite-difference checks for pooling and BatchNorm where meaningful.

- Cross-backend agreement for forward and gradients.

- Padding/stride boundary cases, non-square kernels, odd dimensions, and tail handling.

- BatchNorm train/eval behavior and checkpoint persistence tests.

### Passing criteria

- Reference CNN trains end-to-end with learning behavior comparable to a matched reference implementation using the same architecture/hyperparameters.

- No backend may silently change convolution layout semantics.

- Optimized backend kernels must satisfy documented numerical tolerance against scalar reference.

- CIFAR-10-class demo must achieve a non-degenerate, repeatable accuracy substantially above chance; the exact release target should be frozen with the final model architecture rather than guessed now.

### Benchmark evidence

- Conv2D throughput across representative batch/channel/image/kernel sizes.

- AVX2 scalar-relative speedup.

- CUDA/Vulkan convolution kernel time plus transfer/sync/layout-conversion time.

- Training examples/sec and peak memory for the canonical CNN.

- Where practical, compare CUDA convolution against a trusted vendor/reference implementation; report ratio honestly.

### Risks / design cautions

- Convolution can tempt backend-specific tensor layout hacks; preferred layouts belong in backend capability/planner logic, not in global tensor semantics.

- BatchNorm state introduces mutable non-parameter state that must be included in serialization.

### Explicitly out of scope

- ResNet-scale architecture zoo.

- Winograd/FFT convolution as mandatory algorithms.

- Image augmentation framework beyond minimal demo preprocessing.

## Future Phase F4 — Sequence Models

Objective: Add first-class support for token/time-step sequences and recurrent training.

What this unlocks: Character language models, text classification, time-series prediction, and recurrent neural-network experiments.

Prerequisites: F1 utilities, mature autodiff, stable parameter/state handling.

### Required deliverables

- Embedding lookup operator with backward accumulation into embedding rows.

- Gather/index-select primitive with bounds checking and explicit integer index dtype policy.

- Sequence/batch masking primitives.

- Vanilla RNN cell as the simplest recurrent reference.

- LSTM cell with explicit gate equations and state semantics.

- GRU as a later same-phase deliverable if LSTM is stable.

- Unrolled reverse-mode differentiation through time using the existing graph model before considering specialized fused recurrent backward logic.

- Hidden-state initialization and optional state carry between batches.

- Gradient clipping integrated into canonical recurrent examples.

- Character-level language-model demo and at least one simple time-series or sequence-classification example.

### Required validation

- Gradient checks for embedding lookup and tiny recurrent cells.

- Repeated-index embedding test proving gradients accumulate rather than overwrite.

- Longer unroll stress test for graph lifetime and memory correctness.

- Masking tests proving padded sequence positions do not contribute to loss/gradients.

- Cross-backend forward/backward agreement on short sequences.

### Passing criteria

- Character model loss decreases materially below an untrained/uniform baseline.

- Recurrent state and optimizer checkpoint can resume training correctly.

- No graph-lifetime leaks across repeated sequence batches under sanitizer runs.

- Embedding duplicate-index gradients are mathematically correct.

### Benchmark evidence

- Tokens/sec versus sequence length and batch size.

- Forward/backward memory growth versus unroll length.

- Kernel-launch/dispatch overhead for many small recurrent operations measured explicitly.

- Planner trace demonstrating whether recurrent workloads remain CPU-favorable at small sizes and when GPU execution becomes beneficial.

### Explicitly out of scope

- Beam search as a training requirement.

- Large-vocabulary optimized softmax variants.

- Sequence-to-sequence attention before the transformer phase.

## Future Phase F5 — Transformer Primitives

Objective: Make a small transformer trainable using reusable tensor operators rather than a monolithic special-case kernel.

What this unlocks: Transformer encoders, tiny GPT-style autoregressive models, and modern attention-based experiments.

Prerequisites: Stable sequence/token infrastructure, robust broadcasting/views, mature matmul, softmax, autodiff, and training utilities.

### Required deliverables

- Batched matrix multiplication with complete backward support.

- LayerNorm forward/backward with numerically stable mean/variance calculation.

- GELU activation and backward semantics.

- Causal and padding masks compatible with softmax/attention.

- Scaled dot-product attention expressed first as composable primitive ops: QK^T, scaling, masking, softmax, and multiplication by V.

- Multi-head attention built using reshape/view/transpose/batched-matmul semantics.

- Token embedding plus positional representation; initial implementation may use learned positional embeddings.

- Transformer block: LayerNorm, attention, residual path, MLP/GELU, residual path.

- Autoregressive cross-entropy training example.

- Tiny GPT-style model sized for development hardware; exact parameter count should remain configurable rather than hard-coded.

### Required validation

- Gradient checks for LayerNorm, GELU, batched matmul, and small attention tensors.

- Mask correctness tests preventing future-token attention in causal mode.

- Multi-head reshape/transpose round-trip tests ensuring no accidental copy or stride corruption.

- Scalar-versus-optimized backend agreement for a complete transformer block.

- Checkpoint/resume training test for the tiny language model.

### Passing criteria

- Tiny transformer language-model loss improves repeatably on a small text corpus.

- Causal masking is proven by tests, not visual inspection.

- Complete forward/backward executes through the normal Kestrel graph and planner.

- No transformer-specific numerical rules bypass the dtype/promotion/accumulation system.

### Benchmark evidence

- Tokens/sec by context length, batch size, model width, and backend.

- Attention breakdown: QKV projection, QK^T, softmax/masking, AV product, output projection.

- Planner cost breakdown including layout conversions and transfers.

- CUDA matmul portions compared with cuBLAS where equivalent GEMM calls exist.

- Memory footprint versus context length, explicitly showing quadratic attention-memory behavior.

### Risks / design cautions

- Attention contains many views/transposes; poor stride handling can force hidden repacks and erase expected GPU gains.

- A fused attention kernel should only be introduced after the compositional implementation is correct and benchmarked.

### Explicitly out of scope

- FlashAttention in the first transformer implementation.

- Mixture-of-experts.

- Distributed multi-GPU transformer training.

- Large language models beyond development-scale demonstrations.

## Future Phase F6 — Mixed-Precision Training

Objective: Use Kestrel's dtype architecture to support lower-precision compute without silently degrading training correctness.

What this unlocks: Faster/lower-memory GPU training, deeper use of FP16/BF16 semantics, and richer planner decisions.

Prerequisites: FP16/BF16 backend arithmetic support, stable dtype promotion/accumulation, mature optimizer/checkpoint state.

### Required deliverables

- Explicit mixed-precision training policy rather than implicit global casts.

- FP16/BF16 model compute paths with FP32 accumulation where frozen by dtype semantics.

- FP32 master-parameter option for lower-precision trainable weights if required by optimizer stability.

- Static loss scaling first; dynamic loss scaling only after overflow detection semantics are designed.

- Optimizer-state dtype policy, especially for Adam moments.

- Planner capability queries distinguishing supported dtype from preferred/high-performance dtype.

- Checkpoint metadata preserving parameter, master-weight, gradient, and optimizer dtypes.

- Per-backend numerical-conformance tests for lower precision.

### Required validation

- FP16/BF16 forward comparisons against FP32 reference with documented tolerances.

- Training regression tests proving mixed-precision runs converge comparably to FP32 on at least MNIST MLP and one larger demo.

- Overflow/loss-scale tests using constructed large-gradient cases.

- Checkpoint round-trip across mixed-precision state.

- Backend fallback tests when a requested lower-precision operation is unsupported.

### Passing criteria

- Mixed-precision training must not silently change the globally frozen promotion/accumulation rules.

- Training quality remains within a frozen task-specific tolerance of FP32 reference on canonical demos.

- Unsupported lower-precision plans are rejected or explicitly converted; never executed with undocumented semantics.

### Benchmark evidence

- Step time, examples/sec or tokens/sec, peak memory, and energy/power only if measurement is available and trustworthy.

- FP32 vs FP16/BF16 comparison on the same backend and model.

- Break down gains by kernel speed, reduced transfer volume, and reduced memory footprint rather than reporting only end-to-end speedup.

### Explicitly out of scope

- Quantized training (INT8).

- FP8.

- Automatic mixed precision with a huge policy database.

## Future Phase F7 — Planner-Aware Whole-Training Optimization

Objective: Extend the planner from operation-level legality/cost reasoning toward repeated training-step optimization.

What this unlocks: Kestrel can optimize model execution as a repeated workload rather than choosing each operation in isolation.

Prerequisites: Stable models from prior phases, performance profiles, transfer-cost model, graph execution.

### Required deliverables

- Training-step graph capture or reusable execution plan where graph structure is stable across batches.

- Plan caching keyed by relevant shapes, dtypes, backend topology, and model state that affects execution legality.

- Transfer minimization across adjacent operations; planner should reason about keeping tensors resident where useful.

- Layout-conversion reuse/caching where correctness permits.

- Planner explanation output at graph/segment level: chosen backend, expected compute cost, transfer cost, conversion cost, and fallback reason.

- Optional operation fusion candidates identified from measurements, not assumed universally profitable.

- Memory-pressure-aware planning so a theoretically fast GPU plan can be rejected when workspace/residency is infeasible.

### Required validation

- Plan-cache invalidation tests when shape/dtype/backend/profile context changes.

- Correctness equivalence between uncached and cached plans.

- Forced-backend and constrained-execution tests continue to override automatic planning legally.

- Memory-pressure tests using deliberately constrained pools.

### Passing criteria

- Cached execution produces identical model semantics to fresh planning.

- Planner explanations are deterministic enough to debug and contain the dominant cost components.

- At least one canonical workload demonstrates a measured whole-step improvement from avoiding unnecessary transfers/repacking, not merely faster isolated kernels.

### Benchmark evidence

- Compare per-op greedy planning against whole-step cached planning on MLP, CNN, and transformer demos.

- Report compute, transfer, synchronization, conversion, and allocator components separately.

- Report planning overhead itself and amortization across repeated steps.

### Explicitly out of scope

- General compiler IR optimization framework.

- Distributed graph partitioning across machines.

- Speculative execution.

## Stretch directions after the model-family roadmap

The following are valid future directions, but they should not be allowed to delay the dense/CNN/RNN/transformer ladder above.

- Quantized inference: INT8/UINT8 weight and activation paths, explicit quantize/dequantize operators, calibration, and saturating conversion semantics.

- Quantization-aware training only after inference quantization semantics are mature.

- Sparse tensors and sparse matrix operations if a real target workload justifies them.

- Custom operator extension mechanism for user-defined kernels.

- Model exchange/import, potentially through a constrained format subset, once Kestrel model semantics are stable.

- Distributed training across multiple GPUs or hosts. This is a distinct systems project inside Kestrel and should be treated as such, not slipped into an earlier phase.

- Kernel fusion and graph-level compilation for frequently repeated subgraphs.

- Inference-focused optimizations such as constant folding, operator fusion, and static memory planning.

## Benchmarking standard for future model phases

Every phase must produce both correctness evidence and performance evidence. A fast incorrect model is a failed phase; a correct model with no measurement is incomplete performance work.

### Measurement protocol

- Record full machine identity: CPU, SIMD capability, GPU(s), driver/runtime versions, memory, compiler, optimization flags, Kestrel commit, and backend configuration.

- Perform warm-up iterations before timed repetitions.

- Use repeated measurements and report a robust statistic such as median; add percentile/spread where variability matters.

- For asynchronous GPU work, measure completed execution, not submission latency; synchronize or use backend-native timing correctly.

- Report host-device transfer separately from kernel execution.

- Report layout conversion/repacking separately where it is a significant cost.

- Do not compare different batch sizes, dtypes, model architectures, or accuracy levels as if they were equivalent performance runs.

- For end-to-end training, report time/step, examples/sec or tokens/sec, peak memory, final validation metric, and number of optimization steps/epochs.

### Reference comparisons

- Scalar Kestrel remains the correctness reference for individual operations.

- For CUDA GEMM-compatible operations, compare against cuBLAS rather than only against Kestrel's naive CUDA kernel.

- For end-to-end models, use a small trusted framework implementation with identical architecture, initialization policy where possible, optimizer, learning rate, batch size, and data preprocessing as a behavioral/performance reference.

- Reference frameworks are comparison baselines, not targets that Kestrel must beat. Report ratios and reasons honestly.

### Required benchmark tables

| Category | Required columns |
| --- | --- |
| Kernel | op, shape, dtype, backend, median time, throughput, reference time, ratio |
| GPU overhead | kernel, H→D, D→H, sync, repack/layout, total |
| Training | model, backend, dtype, batch/context, step time, throughput, peak memory, validation metric |
| Planner | model/op segment, candidate plans, estimated cost, selected plan, measured cost, error/ratio |
| Mixed precision | model, FP32 throughput/memory/metric, low-precision throughput/memory/metric, relative delta |

## Model capability release gates

| Gate | Kestrel must demonstrate |
| --- | --- |
| Dense Training Complete | MNIST MLP + regression/classification sanity tasks; gradient checks; optimizer correctness |
| General Dense Models | MNIST autoencoder and reusable model composition |
| CNN Ready | CNN forward/backward, CIFAR-10-class training, backend agreement |
| Sequence Ready | Embedding + recurrent model, masking, BPTT, character/time-series demo |
| Transformer Ready | Attention stack + tiny transformer LM trained end-to-end |
| Mixed Precision Ready | At least two canonical models train in lower precision within frozen quality tolerance |
| Planner-Optimized Training | Repeated training graph demonstrates measured benefit from topology-aware execution planning |

## Long-term non-goals

- Competing with PyTorch/TensorFlow on total operator count.

- Supporting every published neural-network architecture.

- Building a Python-first ecosystem before the C core is mature.

- Chasing benchmark wins by changing model quality, precision, or workload semantics.

- Hiding backend limitations from advanced users.

- Adding distributed training before single-node execution and model semantics are mature.

- Treating CUDA as the only meaningful GPU target; Vulkan remains part of Kestrel's cross-vendor heterogeneous-runtime thesis.

# Part VII Canonical decision register

This register is the cross-cutting index. A FROZEN rule may still be PLANNED in code. SUPERSEDED entries identify the discarded assumption and its immediate replacement; they are not additional current requirements. Ticket ownership is retained without inventing new ADR numbers.

**Engineering principle — Later is now.** The register records durable architectural consequences of known requirements, even when the dependent feature remains PLANNED. This is an engineering principle, not a new ticket decision or a change to implementation status.

```text
Known future requirement
        ↓
Does it invalidate a foundational assumption?
NO  → defer it
YES → design the seam now; implement the capability later
```

| Decision | Status | Current rule | Superseded rule | Why changed | Owner | Implementation |
| --- | --- | --- | --- | --- | --- | --- |
| Arena state | FROZEN | ZERO/LIVE/INVALID; checked transitions | Unspecified bump state | Safe failure and lifetime | KES-001 | IMPLEMENTED |
| Arena failure | FROZEN | Validate before committing offset/state | Assert-only safety | Release must remain defensive | KES-001 | IMPLEMENTED |
| Arena lifetime | FROZEN | No individual free; exclusive mark/pop/reset | Runtime resets shared caller arena | Views share invalidation boundary | KES-001 | IMPLEMENTED foundation |
| Alignment | FROZEN | 64-byte base guarantee; checked requested alignment | Universal padded rows | Allocation ≠ tensor layout | KES-001 / ADR-006 | Arena implemented; layouts planned |
| Numerical policy | FROZEN | Global promotion, accumulation, overflow | Backend chooses semantics | One numerical system | KES-003 / revised ADR-007 | Queries IMPLEMENTED |
| Dtype sentinels | FROZEN | INVALID=0; automatic terminal COUNT | Hard-coded collision; equality first | Reject invalid and BOOL pairs | KES-003 | IMPLEMENTED |
| Storage | FROZEN | Capacity, space, provider, opaque token | Universal float pointer or native union | Separate resource mechanisms | KES-005 / ADR-004 | PLANNED |
| Tensor views | FROZEN | Shape, dtype, signed byte strides, offset, storage | Element strides and direct device tag | Dtypes, negative/zero strides | KES-006/007 / ADR-005/006 | Legacy works; v2 PLANNED |
| Contiguity | FROZEN | Packed row-major; ignore singleton stride | Backend preferred layout defines contiguous | Stable logical meaning | KES-007 / ADR-006 | Target PLANNED |
| Hardware versus API | FROZEN | Physical root, target and backend distinct | DEV_CPU / DEV_CUDA runtime enum | API is not hardware | KES-004 / refined ADR-002 | Records IMPLEMENTED; legacy enum remains |
| Backend binding | FROZEN | Exactly one Execution Target | BackendInstance → PhysicalDevice | Partitions and aggregates | KES-004 | Record IMPLEMENTED |
| Physical backing | FROZEN | Explicit BACKED_BY, zero/many | Embedded single physical_device_id | Hidden parents and groups | KES-004 | Record IMPLEMENTED; validation planned |
| Execution hierarchy | FROZEN | Target direct parent; unique and acyclic | Flat list or backend hierarchy | Nested subdevices | KES-004 | Record IMPLEMENTED; validation planned |
| Memory relation | FROZEN | Target–memory structural association | Device–memory; one-to-one; LOCAL/SHARED/REMOTE | Sharing does not prove legal access | KES-004 / refined ADR-003 | Record IMPLEMENTED |
| Identity evidence | FROZEN | Typed claims, locators, explicit relations | Name/vendor/model/capacity or API ordinal | Avoid false cross-API merges | KES-004 | Registration PLANNED |
| PCI BDF | FROZEN | Physical locator/correlation | Universal target identity | MIG targets share function | KES-004 | Comparator PLANNED |
| Comparison | FROZEN / OPEN | Explicit legal namespace policy | Equal UUID-shaped bytes merge | Namespaces differ | KES-004 | Policy table OPEN |
| Outcomes | FROZEN | MATCH / DISTINCT / INDETERMINATE / CONFLICT | Guess merges; first source wins | Preserve uncertainty and reject conflict | KES-004 | PLANNED |
| Identity payload | IMPLEMENTED | kind, uint8 length, bytes[64] | Only a representation proposal | Header now defines it | KES-004 | Encodings / ABI OPEN |
| Topology ownership | FROZEN | True opaque pointer owners; NULL empty | Public by-value owner / impl* handle | Private ownership and lifecycle | KES-004 Amendment A | Lifecycle IMPLEMENTED |
| Finalization | FROZEN | Fallible work before transfer; immutable result | Partial transfer on failure | Transactional publication | KES-004 | PLANNED |
| Memory identity | OPEN | Retain seam; add justified namespaces | Invent namespace enum to fill gaps | Real discovery semantics needed | KES-004 | Metadata types only |
| Provider lifetime | FROZEN / OPEN | Provider-appropriate mechanics | All GPU allocators mimic arena | Native APIs have different lifetimes | KES-005 | Exact binding OPEN |
| Capability | FROZEN | Legality separate from preference and cost | Topology relation authorizes execution | Resource compatibility matters | KES-008 / ADR-008 | PLANNED |
| Planning | FROZEN | Legal candidates then measured total cost | GPU always wins; device-tag dispatch | Movement and sync can dominate | KES-013/014 / ADR-009 | PLANNED |
| Expert execution | FROZEN | Honor legal forced backend or fail | Silent fallback changes experiment | Reproducibility and control | KES-014 | PLANNED |
| Differentiation | PLANNED | Tensor reverse mode; finite differences oracle | Finite differences train large models | Parameter-scaled forward cost | KES-016 | Scalar prototype HISTORICAL |
| Snapshot evolution | OPEN | Immutable current snapshot; future policy needed | Assumed hotplug/generation guarantees | Lifecycle beyond current scope | KES-004 future ADR | Not implemented |

# Part VIII Source register and evidence

Sources were inspected for the 8 October 2026 checkpoint. Research findings from supplied records motivate architectural seams; they are not newly verified vendor-support claims or implemented adapter claims. S1 and S2 retain useful contracts but their flat topology has been superseded in canonical prose. Original source documents remain the historical record rather than being pasted repeatedly into this edition.

| ID | Source location | Authority and limitation |
| --- | --- | --- |
| S1 | Kestrel_Engineering_Specification_v2.docx | Older v2 baseline; topology/status overrides apply |
| S2 | Kestrel_Architecture_Specification_v2_Redesigned.docx | Architecture reference; topology overrides apply |
| S3 | Kestrel_Future_Direction_Roadmap.docx | Directional post-v2 roadmap |
| S4 | Daniel_Texyn_Kestrel_Handoff.md | Detailed continuity; older topology/status |
| S5 | documentation/KES-004/Kestrel_KES-004_Runtime_Topology_Architecture_Report.md | Research-driven semantic redesign; owner amendment applies |
| S6 | documentation/KES-004/KES-004 TICKET.md | Active implementation contract; owner amendment applies |
| S7 | documentation/KES-004/KES-004 AMENDMENT.md | True opaque owner replacement |
| S8 | documentation/KES-004/KES-004 TESTS TICKET.md | Slice 1 representation/lifecycle scope |
| S9 | include/, src/, tests/, Makefile and Git checkpoint | Implemented truth; working tree differs from HEAD |
| S10 | Review Kestrel Context, conversation 6ab7098a-2b24-83ea-bbdb-4d1cea96a97f | Five recent turns returned; historical origin/status account |
| S11 | /home/kernelghost/Downloads/dev_unpacked/Kestrel_State_Report.md | 5 October intermediate amendment candidate; superseded by accepted report/ticket/header |
| S12 | README.md | Legacy single-header engine, historical XOR measurements |

## Additional historical sources and supplied posts

| ID | Source relative to ML_in_C | Use |
| --- | --- | --- |
| H1 | BACKPROPAGATION.md | Historical code or learning note; interpreted against actual implementation |
| H2 | ML_Binary_Classification_and_XOR.md | Historical code or learning note; interpreted against actual implementation |
| H3 | ML double inputs.md | Historical code or learning note; interpreted against actual implementation |
| H4 | 01_single_weight/complete_bias_linear_regressor/neuron.c | Historical code or learning note; interpreted against actual implementation |
| H5 | 01_single_weight/MACHINE LEARNING IN C.md | Historical code or learning note; interpreted against actual implementation |
| H6 | 01_single_weight/zero_bias_linear_regressor/zero-bias_linear_regressor.c | Historical code or learning note; interpreted against actual implementation |
| H7 | 02_double_weight/gates.c | Historical code or learning note; interpreted against actual implementation |
| H8 | 02_double_weight/full_gates.c | Historical code or learning note; interpreted against actual implementation |
| H9 | Kestrel-main/kestrel.h | Historical code or learning note; interpreted against actual implementation |
| H10 | Kestrel-main/xor.c | Historical code or learning note; interpreted against actual implementation |

| ID | Post supplied | Evidence boundary |
| --- | --- | --- |
| P1 | Linear regression in C | No publication date supplied; run differs from surviving neuron.c |
| P2 | OR/XOR and first neural network | Historical success report, not current runtime test |
| P3 | Generalized network representation | Architecture and learning narrative |
| P4 | Finite-difference ceiling and Python autodiff | Prototype account and scaling estimates |
| P5 | Tensor views and arena rewrite | Earlier test/sanitizer report, distinct from current checks |
| P6 | Dtype, storage and runtime redesign | Full text supplied in conversation; later KES-004 target refinement governs |

Nine pasted-text attachments contain duplicate copies of P1–P5. Their content was read and their hashes match; they are not nine different posts. P6 is the final post supplied directly in the conversation. Exact post text is available for all six; publication dates and screenshots are not. The normalized working copy of P6 removes formatting escapes and an incidental LinkedIn safety-link wrapper without adding historical claims.

## Source reconciliation and historical limits

S10 was retrieved as a bounded conversation response containing five recent turns and attachment references, not the complete earlier chat. No missing earlier turn is claimed to have been read. The local source set and newly supplied posts fill the engineering-history gap, while accepted current instructions resolve the second-edition requirements. S11 is an older candidate checkpoint; it cannot override the later report, amendment, ticket and header.

The sixth post, earlier state reports, the former flat-device design, by-value-owner proposals, identity-payload proposal status and old README/test/performance reports remain deliberately historical. This edition quotes or paraphrases their contributions only where they explain evolution, and immediately states the current replacement where needed. Earlier informal mathematical simplifications are identified in Part I rather than promoted to contracts.

## Open decisions and implementation risks

| Open subject | Required next decision or evidence |
| --- | --- |
| Observation registration | Exact public API/error policy, typed temporary IDs, copied metadata and failure-ID policy |
| Identity | Per-kind encodings, namespace comparison matrix, real memory identity kinds, future ABI guarantees |
| Provider/storage | Exact provider binding, native allocation paths and lifetime representation in KES-005 |
| Views and mutation | Concrete operator alias/write rules and exact descriptor validation API |
| Snapshots | Hotplug, generations and cross-snapshot lifetime/identity semantics |
| Backends | Discovery adapters, availability/error policy and actual supported capability matrices |
| Future models | Declared model/configuration, CIFAR metrics, transformer size, mixed-precision tolerances |
| Performance | Measured profiles, planner results and benchmark evidence; no invented results |
| Release | No dates frozen by this handbook; completion follows evidence gates |

## Source fingerprints and exact Git checkpoint

| Repository-relative source | Bytes | SHA256 prefix |
| --- | --- | --- |
| documentation/KES-001/Implementations.md | 22025 | 6eddf33e5f47547e |
| documentation/KES-001/decisions.md | 25642 | 23adba10e7d0ead8 |
| documentation/KES-002/ADRs.md | 13668 | af84bb0b4270327d |
| documentation/KES-002/KES-002.md | 31845 | 0b659b31ae99f961 |
| documentation/KES-003/KES-003 DISCUSSIONS.md | 18342 | 81b286bb3d083e09 |
| documentation/KES-003/KES-003.md | 13804 | 99038a6918832a6d |
| documentation/KES-004/KES-004 AMENDMENT.md | 6069 | b171c8a565e83a52 |
| documentation/KES-004/KES-004 DISCUSSIONS.md | 18015 | eca47400ff750223 |
| documentation/KES-004/KES-004 TESTS TICKET.md | 6075 | 9a2c3e1d1a36514d |
| documentation/KES-004/KES-004 TICKET.md | 25396 | 05708a8c809befda |
| documentation/KES-004/KES-004.md | 14554 | d6f5dc055707af6a |
| documentation/KES-004/Kestrel_KES-004_Runtime_Topology_Architecture_Report.md | 35927 | f48d949b54efe4de |
| include/arena.h | 3296 | 8d9bbf4bb7c6abff |
| include/dtype.h | 2231 | dc0acf8ff7ea5121 |
| include/kestrel_core.h | 2463 | 4a97e287cc8d9a5d |
| include/kestrel_topology.h | 3799 | 5dd1716f4474e103 |
| include/tensor.h | 4761 | 309ad570ff5f9588 |
| src/arena.c | 7019 | 9cd670c662fb495e |
| src/dtype.c | 4906 | 9c182d1a93b8c927 |
| src/kestrel_topology.c | 1235 | fa69f14772b91539 |
| src/tensor.c | 4772 | 0b7f41c4189fae58 |
| tests/test_arena.c | 8518 | e1ce96e52b9304ac |
| tests/test_dtype.c | 21737 | 4871f2cdb2c405dc |
| tests/test_tensor.c | 11161 | 10467560de098b48 |
| tests/test_topology.c | 10594 | 1d07e9b6ba71da1e |

Fingerprints distinguish this working-tree snapshot from the commit alone; prefixes are compact identifiers, not a replacement for full revision control. The source path is recorded above so another engineer can locate the original records.

## Exact local Git state

Local branch: main. Full HEAD: 28bea7694c4275d717ef6cb21536e341a9cfbb77. Working-tree status follows exactly as inspected; M means modified tracked file and ?? means untracked. No remote merge or clean checkout is inferred.

| Status | Path |
| --- | --- |
|  M | documentation/KES-004/KES-004 DISCUSSIONS.md |
|  M | include/kestrel_core.h |
|  M | include/kestrel_topology.h |
|  M | src/arena.c |
|  M | tests/test_tensor.c |
| ?? | Daniel_Texyn_Kestrel_Handoff.docx |
| ?? | Daniel_Texyn_Kestrel_Handoff.md |
| ?? | Kestrel_Architecture_Specification_v2_Redesigned.docx |
| ?? | Kestrel_Engineering_Specification_v2.docx |
| ?? | Kestrel_Future_Direction_Roadmap.docx |
| ?? | Kestrel_KES-004_Runtime_Topology_Architecture_Report.docx |
| ?? | Kestrel_Opportunity_Outreach_Plan_v1.1.pdf |
| ?? | Makefile |
| ?? | cd |
| ?? | compile_commands.json |
| ?? | documentation/KES-004/KES-004 TESTS TICKET.md |
| ?? | documentation/KES-004/KES-004 TICKET.md |
| ?? | test_arena |
| ?? | test_tensor |
| ?? | tests/test_topology.c |

## Historical intake fingerprints

| Historical source | Bytes | SHA256 prefix |
| --- | --- | --- |
| H1 BACKPROPAGATION.md | 7745 | d3379da055f7d007 |
| H2 ML_Binary_Classification_and_XOR.md | 8928 | b01457467de046ae |
| H3 ML double inputs.md | 11103 | 3a2ce0e8122325b1 |
| H4 01_single_weight/complete_bias_linear_regressor/neuron.c | 3933 | 0e814e214c65bff8 |
| H5 01_single_weight/MACHINE LEARNING IN C.md | 5686 | ab7d6153f2d1f5d1 |
| H6 01_single_weight/zero_bias_linear_regressor/zero-bias_linear_regressor.c | 1797 | 464397c0d52c19d4 |
| H7 02_double_weight/gates.c | 3263 | 138959f25c6befaa |
| H8 02_double_weight/full_gates.c | 6960 | 58438d01dcc1b43f |
| H9 Kestrel-main/kestrel.h | 14323 | 093f84ddf06f1c33 |
| H10 Kestrel-main/xor.c | 1422 | dddee6ee5858bafb |

| Intake | SHA256 | Meaning |
| --- | --- | --- |
| P1–P5 repeated attachment | f2f6e1e934cbcc4b4890f6d8443610aae854af14673940704c7af9d97cbd40d9 | Nine matching copies |
| P6 normalized transcription | 6ee09b92d6dfa306bf53f4ae37f9ebfbf3b9db65c2ac4e7393b1b8ead6f53ea2 | Derived working text; original is the conversation message |

## Edition comparison and synchronization method

The first edition contained 126 rendered pages. The second expands the engineering history and mature-product narrative, consolidates repeated S1/S2/S4 architecture into one contract sequence, and integrates S5/S6/S7/S8 topology into one canonical chapter. Detailed core-v2 phase requirements and F1–F7 requirements are retained. The old source reproductions and redundant checklists are replaced by the decision register and source references. Both deliverables are generated from one substantive block stream; heading, paragraph, code and table content are checked against that stream. The automatic contents and pagination are presentation-only additions.

Third-edition checkpoint patch: added the “Later is now” engineering principle and its decision-register reminder; clarified Daniel Ademoye’s personal-project attribution and the relationship to Texyn Systems; standardized the requested headings and KES ticket spelling. The approved second-edition architecture, technical contracts, implementation statuses, roadmaps, OPEN decisions and resume point are retained. Canonical handbook checkpoint: 8 October 2026.

# Final Checkpoint — Where Implementation Resumes

> CURRENT CONTRACT — The exact next slice is KES-004 topology-builder observation storage and registration. Slice 1 public vocabulary and opaque-owner lifecycle are implemented and have the recorded approval verdict. Whole KES-004 remains ACTIVE.

Implement typed physical, execution and memory observation storage with strongly typed temporary IDs. Include backend observations where required by the agreed model, owned copied metadata, identity claims, physical locators and explicit relationship evidence. Arrays need count/capacity/storage bookkeeping, overflow-safe growth, cleanup and transactional allocation failure. Registration must preserve builder validity and existing content on failure, and must not prematurely canonicalize observations.

> OPEN — Exact public signatures and error-policy details must be decided before Daniel implements them. The accepted current ticket requires valid builder state on allocation failure but does not explicitly freeze the no-ID-consumption rule in the inspected text. Preserve “no ID consumption on failed registration” as the proposed checkpoint contract to confirm, rather than falsely reporting it as already implemented or unambiguously frozen.

```text
Observation storage / registration
        ↓
Identity comparator policy
        ↓
Reconciliation
        ↓
Relation canonicalization
        ↓
Graph and metadata validation
        ↓
Transactional finalize
        ↓
Immutable snapshot
        ↓
Read-only queries / introspection
        ↓
KES-004 complete
        ↓
KES-005 storage and providers
```

Review the observation contract first, then implement and test the bounded slice. Verify copied metadata ownership, invalid input behavior, overflow and allocation failures, typed references and destroy cleanup. Later slices establish comparison legality, conflict handling, normalized relations and immutable publication. Do not substitute new tensor allocation, kernels or planner work for this checkpoint. Daniel resumes at observation storage and registration.
