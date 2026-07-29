# Kestrel

A neural-network engine written from scratch in C — no frameworks, no BLAS, no third-party dependencies. Kestrel is a single header: the matrix math, the network representation, the forward pass, and the training loop are all implemented behind one `#include`. It depends only on the C standard library and `libm`.

Today Kestrel trains small networks with **finite-difference gradient descent** and validates on XOR. Its memory model is arena-based: every allocation happens once, up front, and the forward and training loops do **zero heap allocation**. That property — deterministic, fragmentation-free memory with no `malloc`/`free` in the hot path — is the design decision the rest of the engine is built around, and it's what points Kestrel at eventual edge and embedded inference.

This document is the full technical record: architecture, the memory model, the matrix and network representations, the training method, the design decisions behind each one, and — stated plainly — what is and isn't implemented yet.

---

## Status — what Kestrel is, and is not, today

Read this before the rest, because the README is a specification and it will not claim more than the code delivers.

**Implemented and working:**

- A bump-allocator arena: one up-front allocation, `O(1)` sub-allocation, whole-arena reset and free.
- A strided `matrix` type with zero-copy row views (`row_matricize`) and a naive `i·j·k` matmul.
- A feed-forward network of arbitrary depth/width, described by an `size_t[]` architecture array.
- A forward pass: per layer, matrix product → bias add → **sigmoid**.
- A cost function: **mean squared error**, accumulated in `double`.
- Training by **finite-difference gradient estimation** (`nn_fdiff`) + gradient descent, with best-model tracking.
- Validated on **XOR** (`{2, 2, 1}`): converges to ~`2.6e-5` cost; the full 1,000,000-iteration demo runs in ~4.2 s.

**Not implemented yet (see [Roadmap](#roadmap)):**

- **Backpropagation.** Kestrel does *not* compute analytic gradients yet. `nn_fdiff` estimates them by perturbation. Backprop (`nn_backprop`) is the next milestone and will be validated *against* the finite-difference gradients, which stay in the tree as the correctness oracle.
- ReLU / softmax / cross-entropy, and anything MNIST-scale.
- SIMD (AVX2/NEON) or cache-blocked matmul, a selectable compute backend, GPU, int8 quantization, or an MCU port.

Anything in this document describing a selectable backend, an accelerated matmul, or microcontroller deployment lives in the Roadmap section and is labelled as such. The engine as it stands is the finite-difference trainer described above.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Building and Running](#building-and-running)
3. [Verification](#verification)
4. [The Memory Model — Arena Allocator](#the-memory-model--arena-allocator)
5. [The Matrix Type](#the-matrix-type)
6. [The Network Representation](#the-network-representation)
7. [Training — Finite-Difference Gradient Descent](#training--finite-difference-gradient-descent)
8. [Design Decisions and Tradeoffs](#design-decisions-and-tradeoffs)
9. [Roadmap](#roadmap)
10. [File Structure](#file-structure)

---

## Architecture

Kestrel is four layers stacked on one contiguous block of memory. Everything above the arena is a *view into it* or a *loop over it* — no layer owns heap memory of its own.

```
┌──────────────────────────────────────────────────────────────┐
│  Training loop        gradient_descent()                       │
│  cost → estimate gradient → update → track best                │
│      nn_cost / nn_fdiff / nn_learn / NN_copy                   │
└───────────────────────────────┬────────────────────────────────┘
                                │  operates on
┌───────────────────────────────▼────────────────────────────────┐
│  Network              Neural_network                            │
│  weights[] · biases[] · inputs[]  (arrays of matrices)          │
│  forward pass: dot → +bias → sigmoid, per layer                 │
└───────────────────────────────┬────────────────────────────────┘
                                │  built from
┌───────────────────────────────▼────────────────────────────────┐
│  Matrix               matrix { rows, cols, stride, es }         │
│  strided 2-D view · MAT_POS · dotproduct · row_matricize        │
└───────────────────────────────┬────────────────────────────────┘
                                │  memory from
┌───────────────────────────────▼────────────────────────────────┐
│  Arena                Chunk_memory { data, offset, capacity }   │
│  one malloc up front · bump-allocate · reset · free             │
└─────────────────────────────────────────────────────────────────┘
```

Every `matrix.es`, every `weights[i]`, every `biases[i]`, every activation buffer is a pointer into the arena's single block. `nn_allocate` carves the whole network out of the arena in one pass at startup. After that, training touches only floats that already exist — no allocation happens again until the arena is freed.

---

## Building and Running

Kestrel is a single header in the stb style: declarations are always visible; the implementation is compiled into exactly **one** translation unit that defines `KESTREL_CODE` before including it.

```c
#define KESTREL_CODE
#include "kestrel.h"
```

The bundled demo (`kestrel.c`, or `examples/xor.c` in the split layout) is that translation unit. Build and run it:

```bash
gcc -O2 -Wall -Wextra kestrel.c -o kestrel -lm
./kestrel
```

`-lm` links `libm` for `expf` (used by the sigmoid). There are no other link-time dependencies.

To use Kestrel in your own program, include the header everywhere you need the API, and put `#define KESTREL_CODE` in front of the include in one `.c` file only. Defining it in more than one translation unit produces duplicate-symbol link errors — that single definition is the whole build system.

---

## Verification

Compiles clean under full warnings:

```bash
gcc -O2 -Wall -Wextra kestrel.c -o kestrel -lm   # zero diagnostics
```

The XOR demo (`{2, 2, 1}`, sigmoid, MSE, ε = 0.1, learning rate = 0.1, 1,000,000 iterations) trains from a random init and learns the function. A representative run:

```
iteration_no: 0        cost: 0.373062
iteration_no: 1000     cost: 0.250249
iteration_no: 2000     cost: 0.249669
...
iteration_no: 999000   cost: 0.000026

FINAL COST: 0.000026
BEST MODEL COST: 0.000026

train_input [0.000000  0.000000]   0.005445
train_input [0.000000  1.000000]   0.994877
train_input [1.000000  0.000000]   0.994858
train_input [1.000000  1.000000]   0.004544
```

Two things worth reading in that trace. The cost sits on a **plateau around 0.25** for the early iterations — that's the network outputting ≈0.5 for every input, the local flat spot XOR is famous for — and then breaks off it and collapses toward zero once the hidden units differentiate. And the final predictions are ~0.005 / 0.995 against targets of 0 / 1: the sigmoid output layer asymptotes toward but never reaches the extremes, which is expected and correct. Wall time for the full run is ~4.2 s (`-O2`); the init is random (`srand(time(0))`), so the exact trajectory varies between runs while the outcome does not.

---

## The Memory Model — Arena Allocator

This is the load-bearing design decision, so it comes first.

```c
typedef struct {
    size_t   offset;     // bytes handed out so far
    size_t   capacity;   // total size of the block
    uint8_t *data;       // the single underlying allocation
} Chunk_memory;
```

`arena_init` performs **one** `malloc` of `capacity` bytes. From then on, `custom_alloc` is a bump pointer: it returns `data + offset` and advances `offset` by the request size, or returns `NULL` if the request would exceed `capacity`. There is no per-object free list, no header per allocation, no fragmentation. Cleanup is whole-arena: `arena_reset` sets `offset = 0` (reusing the block); `custom_free` releases it.

```c
void *custom_alloc(size_t size, Chunk_memory *arena) {
    if (arena->offset + size > arena->capacity) return NULL;   // out of room
    uint8_t *ptr = arena->data + arena->offset;
    arena->offset += size;
    return ptr;
}
```

The consequence that matters: the entire network — every weight matrix, bias vector, and activation buffer — is allocated out of the arena in one pass inside `nn_allocate`, at startup. **The forward pass and the training loop never allocate.** They only read and write floats that already exist. Memory use is therefore fixed and knowable before the first iteration, and there is no allocator on the hot path to introduce jitter. That is the property that makes this design a candidate for deterministic and embedded inference later.

**Honest limitations of the current arena:**

- **Capacity is a fixed constant chosen up front.** The demo sets it to 100 MB (`.capacity = 100000000`) — vastly more than the XOR net needs, and an untuned placeholder rather than a computed size. A real deployment would size the arena to the model.
- **No individual free.** You cannot release one matrix; you reset or free the whole arena. That is the deliberate trade (see below), not an oversight.
- **No alignment beyond the natural `float` boundary.** `custom_alloc` hands back byte-exact offsets. Because every allocation here is a multiple of `sizeof(float)` and the base is `malloc`-aligned, every `matrix.es` is 4-byte aligned — fine for scalar float access, but **not** the 32-byte (AVX2) or 16-byte (NEON) alignment that vectorised loads want. Aligning arena allocations is a prerequisite for the SIMD roadmap, and is called out there.

---

## The Matrix Type

```c
typedef struct {
    size_t rows;
    size_t cols;
    size_t stride;   // elements per row in the *backing* store
    float *es;
} matrix;

#define MAT_POS(m, r, c) ((m).es[(r) * (m).stride + (c)])
```

The `stride` field is the important one. It decouples a matrix's logical shape (`rows × cols`) from its physical row spacing in memory. That single indirection buys **zero-copy views**:

- `row_matricize(m, i)` returns a `1 × cols` matrix whose `es` points *into* `m` at row `i` and whose `stride` is inherited from `m`. No data is copied — it's a window onto an existing row, the same idea as a slice.
- The XOR demo exploits stride to treat one flat, interleaved array as two matrices at once: with `stride = 3`, the inputs are the `2`-wide view starting at column 0 and the outputs are the `1`-wide view starting at column 2 (`es = &data[2]`). One buffer, two logical matrices, no unpacking.

The core operations:

- `matrix_dotproduct(dst, a, b)` — asserts `a.cols == b.rows` and that `dst` has the right shape, zeroes `dst` (because it accumulates with `+=`), then the classic triple loop: for each `dst[i][j]`, sum `a[i][k] * b[k][j]` over `k`.
- `matrix_sum_bias(dst, bias)` — broadcasts a `1 × cols` bias across every row of `dst`.
- `matrix_activate(m)` — applies the sigmoid elementwise.

One performance note stated honestly: the `k` loop of `matrix_dotproduct` walks `b` **down its rows** — i.e., across memory in `stride`-sized jumps rather than contiguously. That is cache-hostile, and it is exactly the access pattern the cache-blocking milestone on the roadmap exists to fix. It is left naive on purpose: correctness first, then a benchmark, then the optimisation measured against it.

---

## The Network Representation

```c
typedef struct {
    size_t   count;      // number of layers = arch_count - 1
    matrix  *weights;    // count matrices
    matrix  *biases;     // count matrices
    matrix  *inputs;     // count + 1 matrices (activations, incl. the input)
} Neural_network;

#define NN_INPUT(nn)  (nn)->inputs[0]
#define NN_OUTPUT(nn) (nn)->inputs[(nn)->count]
```

A network is described by an architecture array such as `{2, 2, 1}` (two inputs, one hidden layer of two units, one output). `nn_allocate` turns that into:

- `weights[i]` shaped `(layer i width) × (layer i+1 width)`,
- `biases[i]` shaped `1 × (layer i+1 width)`,
- `inputs[i]` shaped `1 × (layer i width)` — the **activation buffer for each layer**, with `inputs[0]` the network input and `inputs[count]` the output.

The forward pass is then a short loop:

```c
for (size_t i = 0; i < nn->count; i++) {
    matrix_dotproduct(nn->inputs[i + 1], nn->inputs[i], nn->weights[i]);
    matrix_sum_bias  (nn->inputs[i + 1], nn->biases[i]);
    matrix_activate  (nn->inputs[i + 1]);
}
```

Storing every layer's activation in `inputs[]` is deliberate on two counts. First, the forward pass needs layer *i*'s output as layer *i+1*'s input, so those buffers must persist across the loop anyway. Second — and this is forward planning — **backpropagation needs the stored activations** to compute local gradients. The buffers the forward pass already keeps are precisely what the backward pass will consume, which is why the layout is built this way now rather than being retrofitted later.

---

## Training — Finite-Difference Gradient Descent

Kestrel currently learns without knowing any calculus. It treats the network as a black box and measures the gradient by nudging.

**Cost** (`nn_cost`) is mean squared error over the dataset, accumulated in `double` for numerical headroom: for each training row, copy it into `NN_INPUT`, run the forward pass, and sum the squared differences between `NN_OUTPUT` and the target; divide by the number of rows.

**Gradient** (`nn_fdiff`) is a one-sided finite difference. With the baseline cost `c = nn_cost(...)` computed once, then for every weight and bias parameter `θ`:

```
grad(θ) = ( cost(θ + ε) − c ) / ε
```

The parameter is bumped by `ε`, a full cost evaluation is run, the gradient is recorded, and the parameter is restored. **Update** (`nn_learn`) is plain gradient descent: `θ -= rate * grad(θ)`.

`gradient_descent` wraps these into the loop, and additionally keeps the best model seen: whenever a step lowers the cost below the previous best, it snapshots the weights with `NN_copy`. Finite-difference descent can wander — the estimate is noisy and the step is fixed — so retaining the best-seen parameters rather than only the last ones is cheap insurance.

**The honest cost of this method.** One gradient step evaluates the cost once for the baseline plus once per parameter, and each evaluation is a full forward pass over the entire dataset. So a step is `O(P · N · F)` for `P` parameters, `N` training rows, and forward-pass cost `F`. For XOR that is `P = 9`, `N = 4` — nothing. For anything MNIST-shaped, `P` runs into the tens of thousands and this becomes hopeless: thousands of forward passes to take one step. On top of that, one-sided differencing carries truncation error on the order of `ε`, so `ε = 0.1` yields a deliberately crude gradient — it works here only because XOR's loss surface is forgiving. **This is the reason backpropagation is the next milestone.** Finite differences is kept afterward not as the trainer but as the *gradient-checking oracle*: the slow, obviously-correct reference that the fast analytic gradient is validated against.

---

## Design Decisions and Tradeoffs

**Why an arena instead of `malloc`/`free` per matrix.** A network is a set of allocations with identical lifetimes — they're all born at startup and all die together. Per-object allocation would pay for individual `free` calls, allocator metadata, and fragmentation to support a flexibility this workload never uses. The arena collapses all of it into one allocation and one free, gives `O(1)` sub-allocation with no bookkeeping, and makes total memory use fixed and predictable. The price is exactly the flexibility that was never needed: you cannot free one object, and capacity is committed up front. For a fixed-topology network that is the right trade, and it is the same reason arenas show up in compilers and game engines.

**Why finite differences before backprop.** It is the simplest thing that exercises the *entire* training loop — cost, gradient, update, convergence — with a gradient that is impossible to get subtly wrong. That makes it the ideal first rung: it proves the loop, the data plumbing, and the optimiser are correct before backprop's complexity (stored activations, the chain rule, transpose bookkeeping, gradient accumulation) is introduced. And it doesn't get thrown away — gradient checking against finite differences is the standard way every autodiff implementation is verified, so it becomes Kestrel's oracle. The cost is that it does not scale, which is precisely why it's a starting point and not the destination.

**Why a single header.** Drop `kestrel.h` into a project and it builds — no library to compile, link, or version, no build system to configure. The stb-style `KESTREL_CODE` guard keeps declarations everywhere while the implementation lands in exactly one translation unit. The trade is compile-time cost in that one unit and the discipline of defining the macro in exactly one place; for a zero-dependency engine meant to be easy to vendor, that's worth it.

**Why store activations in `inputs[]`.** The forward pass needs each layer's output as the next layer's input, so those buffers must live across the whole pass regardless. Keeping them in a first-class array (rather than a scratch buffer that's overwritten) costs a little memory and pays it back twice: the forward loop stays a clean three-liner, and backprop — which must read every activation — finds them already laid out.

**Why sigmoid and MSE, for now.** They are the simplest activation/cost pair that makes a non-linear function like XOR learnable, and both have clean derivatives (`σ' = a(1−a)`; MSE's is just the residual) for the backprop step to come. They are also the wrong pair for classification at scale — that wants ReLU hidden units and a softmax + cross-entropy head, whose combined gradient is far cleaner for many-class problems. That swap is on the roadmap, tied to MNIST.

**Why `float`, not `double`.** Single precision halves memory and bandwidth and is the precision edge and embedded targets actually run, which is the direction Kestrel aims. The cost accumulator is `double` to avoid compounding rounding across the dataset, but the parameters and activations are `float`. One caveat this creates for the *current* trainer: finite differencing subtracts two nearby costs and divides by a small `ε`, which is precision-sensitive — another reason the analytic gradient is the better long-term path.

---

## Roadmap

Ordered, and the order is deliberate: **correctness before optimisation.** A fast wrong gradient is worthless, so nothing in the second half begins until the first half is done and verified.

1. **`nn_backprop`** — analytic gradients via the chain rule, matching the three scalar rules to their matrix forms (activation → elementwise `a(1−a)`; bias → gradient passes straight through; matmul → the "other operand" rule, which appears as a transpose). Validated against `nn_fdiff` by gradient checking before it replaces it.
2. **ReLU, softmax, cross-entropy, and MNIST** — real activations and a proper classification head, trained and evaluated on a real dataset.
3. **SIMD compute path** — a vectorised matmul with an **AVX2** backend (x86) and a **NEON** backend (ARM), each benchmarked against the scalar baseline. Requires aligning arena allocations first (see the memory-model caveat).
4. **Cache-blocked / tiled matmul** — fixing the cache-hostile access pattern in the current naive product, benchmarked against both the scalar and SIMD versions.
5. **Beyond** — int8 quantization, compile-time graph resolution, and a GPU backend.

The destination is an **edge-inference engine**: the arena's zero-allocation hot path already gives deterministic, fragmentation-free memory suitable for constrained targets, and the single-header, standard-C, zero-dependency form is meant to compile anywhere from a server to a microcontroller. To be exact about the gap: a *selectable* CPU/SIMD/GPU backend and an actual microcontroller port are **roadmap items, not current capabilities** — today's engine is the finite-difference trainer this document describes. The design is pointed at that destination; it has not arrived there yet.

---

## File Structure

Proposed layout for the standalone repository:

```
kestrel/
├── README.md          — this document
├── LICENSE            — MIT
├── kestrel.h          — the single-header engine: API, plus implementation under KESTREL_CODE
├── examples/
│   └── xor.c          — the {2,2,1} XOR demo (define KESTREL_CODE, include, train)
└── build.sh           — gcc -O2 -Wall -Wextra examples/xor.c -o kestrel -lm
```

`kestrel.h` is the project; everything else is a driver, a document, or a licence. Naming the demo `examples/xor.c` rather than leaving it as `kestrel.c` makes the separation explicit — the library is the header, the `.c` file is one program that uses it.
