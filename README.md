# Kestrel

A neural-network engine written from scratch in C — no frameworks, no BLAS, no third-party dependencies — in a single header you `#include` and compile. The matrix math, the network representation, the forward pass, and the training loop all live behind that one include; the only things it links are the C standard library and `libm`.

The engine is built around one decision, and it's the reason to look: **all memory is arena-allocated up front, and the forward and training loops never touch the heap.** A single `malloc` at startup carves out every weight, bias, and activation buffer; from then on there is no allocator on the hot path at all. Memory use is fixed, fragmentation-free, and fully known before the first iteration runs — no `malloc`/`free`, no jitter, no surprises. That determinism is what points Kestrel at edge and embedded inference, and it's the property the rest of the design is shaped around.

In the box today: a bump-allocator arena, a strided `matrix` type with zero-copy row views, an arbitrary-depth feed-forward network, a forward pass (matmul → bias → sigmoid), mean-squared-error cost accumulated in `double`, and gradient descent with best-model tracking. It learns XOR to ~`2.6e-5` cost, and the whole training run finishes in about four seconds.

What follows is the full technical record: how it's built, how each piece works, and the reasoning behind every design decision.

---

## Table of Contents

1. [Architecture](#architecture)
2. [Building and Running](#building-and-running)
3. [Verification](#verification)
4. [The Memory Model — Arena Allocator](#the-memory-model--arena-allocator)
5. [The Matrix Type](#the-matrix-type)
6. [The Network Representation](#the-network-representation)
7. [Training](#training)
8. [Design Decisions and Tradeoffs](#design-decisions-and-tradeoffs)
9. [Roadmap](#roadmap)
10. [File Structure](#file-structure)

---

## Architecture

Kestrel is four layers stacked on one contiguous block of memory. Everything above the arena is a *view into it* or a *loop over it* — no layer owns heap memory of its own.

```
┌──────────────────────────────────────────────────────────────┐
│  Training loop        gradient_descent()                       │
│  cost → gradient → update → track best                         │
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

Every `matrix.es`, every `weights[i]`, every `biases[i]`, every activation buffer is a pointer into the arena's single block. `nn_allocate` carves the whole network out of the arena in one pass at startup. After that, training touches only floats that already exist.

---

## Building and Running

Kestrel is a single header in the stb style: declarations are always visible; the implementation is compiled into exactly **one** translation unit that defines `KESTREL_CODE` before including it.

```c
#define KESTREL_CODE
#include "kestrel.h"
```

The bundled demo (`examples/xor.c`) is that translation unit. Build and run it:

```bash
gcc -O2 -Wall -Wextra -I. examples/xor.c -o kestrel -lm
./kestrel
```

`-lm` links `libm` for `expf` (used by the sigmoid). There are no other link-time dependencies.

To use Kestrel in your own program, include the header everywhere you need the API, and put `#define KESTREL_CODE` in front of the include in exactly one `.c` file. That single definition is the whole build system.

---

## Verification

Compiles clean under full warnings:

```bash
gcc -O2 -Wall -Wextra -I. examples/xor.c -o kestrel -lm   # zero diagnostics
```

The XOR demo (`{2, 2, 1}`, sigmoid, MSE, 1,000,000 iterations) trains from a random init and learns the function:

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

Two things worth reading in that trace. The cost sits on a **plateau around 0.25** early on — the network outputting ≈0.5 for every input, the local flat spot XOR is famous for — then breaks off it and collapses toward zero once the hidden units differentiate. And the final predictions are ~0.005 / 0.995 against targets of 0 / 1: the sigmoid output asymptotes toward the extremes without reaching them, exactly as it should. Wall time is ~4.2 s at `-O2`; the init is random (`srand(time(0))`), so the trajectory varies run to run while the outcome doesn't.

---

## The Memory Model — Arena Allocator

This is the load-bearing idea, so it comes first.

```c
typedef struct {
    size_t   offset;     // bytes handed out so far
    size_t   capacity;   // total size of the block
    uint8_t *data;       // the single underlying allocation
} Chunk_memory;
```

`arena_init` performs **one** `malloc` of `capacity` bytes. From then on, `custom_alloc` is a bump pointer: it returns `data + offset` and advances `offset`, or returns `NULL` if the request would exceed `capacity`. No per-object free list, no per-allocation header, no fragmentation. Cleanup is whole-arena: `arena_reset` sets `offset = 0` to reuse the block; `custom_free` releases it.

```c
void *custom_alloc(size_t size, Chunk_memory *arena) {
    if (arena->offset + size > arena->capacity) return NULL;   // out of room
    uint8_t *ptr = arena->data + arena->offset;
    arena->offset += size;
    return ptr;
}
```

The payoff: the entire network — every weight matrix, bias vector, and activation buffer — is carved out of the arena in one pass inside `nn_allocate`, at startup. **The forward pass and the training loop never allocate.** They only read and write floats that already exist. Total memory is fixed and knowable before the first iteration, and there's no allocator on the hot path to introduce latency spikes. That is the property that makes this design a real candidate for deterministic and embedded inference.

Two constraints the design deliberately accepts, worth knowing if you build on it. Capacity is committed up front — you size the arena once (the demo uses a generous 100 MB placeholder; a deployment would size it to the model) — and there's no individual free, only whole-arena reset or release. Both are the intended trade for a fixed-topology network, not oversights (see [Design Decisions](#design-decisions-and-tradeoffs)). One forward-looking note: `custom_alloc` returns byte-exact offsets, so every `matrix.es` lands on a 4-byte (`float`) boundary — perfect for scalar access, and the place to add 32-byte / 16-byte alignment when the SIMD backend arrives.

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

The `stride` field is the clever one. It decouples a matrix's logical shape (`rows × cols`) from its physical row spacing in memory, and that one indirection buys **zero-copy views**:

- `row_matricize(m, i)` returns a `1 × cols` matrix whose `es` points *into* `m` at row `i`, inheriting `m`'s stride. No data is copied — it's a window onto an existing row.
- The XOR demo uses stride to treat one flat, interleaved array as two matrices at once: with `stride = 3`, the inputs are the `2`-wide view at column 0 and the outputs are the `1`-wide view at column 2 (`es = &data[2]`). One buffer, two logical matrices, no unpacking.

The core operations: `matrix_dotproduct(dst, a, b)` checks the shapes, zeroes `dst`, and runs the classic triple loop (`dst[i][j] += a[i][k] * b[k][j]`); `matrix_sum_bias` broadcasts a `1 × cols` bias across every row; `matrix_activate` applies the sigmoid elementwise.

The matmul is intentionally the textbook version, because it's the honest baseline the optimised path will be measured against. Its `k` loop walks `b` down its rows — across memory in stride-sized jumps — which is the cache behaviour the cache-blocking milestone on the roadmap targets. Correctness first, then a benchmark, then the optimisation proven against it.

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

A network is described by an architecture array such as `{2, 2, 1}` (two inputs, a hidden layer of two units, one output). `nn_allocate` turns that into `weights[i]` shaped `(layer i) × (layer i+1)`, `biases[i]` shaped `1 × (layer i+1)`, and `inputs[i]` — the activation buffer for each layer, with `inputs[0]` the network input and `inputs[count]` the output.

The forward pass is then a short, clean loop:

```c
for (size_t i = 0; i < nn->count; i++) {
    matrix_dotproduct(nn->inputs[i + 1], nn->inputs[i], nn->weights[i]);
    matrix_sum_bias  (nn->inputs[i + 1], nn->biases[i]);
    matrix_activate  (nn->inputs[i + 1]);
}
```

Storing every layer's activation in `inputs[]` pays off twice. The forward pass needs layer *i*'s output as layer *i+1*'s input, so those buffers must persist anyway — and backpropagation, which reads every activation to compute local gradients, will find them already laid out. The representation is built now for the pass that's coming next.

---

## Training

Kestrel currently learns without knowing any calculus: it treats the network as a black box and measures the gradient by nudging it.

**Cost** (`nn_cost`) is mean squared error over the dataset, accumulated in `double`: for each row, copy it into `NN_INPUT`, run the forward pass, sum the squared differences at `NN_OUTPUT`, divide by the row count.

**Gradient** (`nn_fdiff`) is a finite difference. With the baseline cost `c` computed once, then for every parameter `θ`: bump it by `ε`, re-evaluate the cost, record `grad(θ) = (cost(θ + ε) − c) / ε`, and restore. **Update** (`nn_learn`) is plain descent: `θ -= rate * grad(θ)`. `gradient_descent` wraps these and keeps the best model seen — whenever a step beats the previous best cost, it snapshots the weights with `NN_copy`, cheap insurance against a noisy estimate wandering.

Choosing finite differences here is deliberate, not a placeholder. It exercises the *entire* training loop — cost, gradient, update, convergence — with a gradient that's impossible to get subtly wrong, which makes it the ideal way to prove the plumbing and the optimiser are correct before analytic gradients are introduced. And it earns a permanent place afterward: gradient checking against finite differences is how every autodiff implementation is verified, so it becomes the oracle the upcoming `nn_backprop` is validated against.

Its ceiling is complexity, and the README is exact about it: a step evaluates the cost once per parameter, each evaluation a full forward pass over the dataset, so a step costs `O(P · N · F)` for `P` parameters, `N` rows, forward cost `F`. For XOR (`P = 9`, `N = 4`) that's nothing; at MNIST parameter counts it stops being viable. That bound is precisely why analytic backpropagation is the next milestone — and why finite differences stays behind it as the reference, not the workhorse.

---

## Design Decisions and Tradeoffs

**An arena instead of `malloc`/`free` per matrix.** A network is a set of allocations with one shared lifetime — born together at startup, freed together. Per-object allocation would pay for individual frees, allocator metadata, and fragmentation to buy a flexibility this workload never uses. The arena collapses all of it into one allocation and one free, gives `O(1)` sub-allocation with zero bookkeeping, and makes memory fixed and predictable. The price — no single-object free, capacity committed up front — is exactly the flexibility that wasn't needed. It's the same reason arenas run compilers and game engines.

**Finite differences before backprop.** The simplest thing that exercises the whole training loop with an un-get-wrong gradient, so it validates the loop before backprop's real complexity (stored activations, the chain rule, transpose bookkeeping, gradient accumulation) lands on top of it — and it survives as the gradient-checking oracle. It doesn't scale, which is exactly why it's the first rung and backprop is the next.

**A single header.** Drop `kestrel.h` in and it builds — no library to compile, link, or version. The stb-style `KESTREL_CODE` guard keeps declarations everywhere while the implementation lands in one translation unit. The trade is compile-time cost in that unit and the discipline of one definition site; for a zero-dependency engine meant to be vendored, that's the right call.

**Activations as first-class buffers in `inputs[]`.** They must live across the forward pass regardless, and keeping them addressable (rather than overwriting scratch) keeps the forward loop a three-liner and hands backprop the activations it needs already laid out.

**Sigmoid and MSE.** The simplest activation/cost pair that makes a non-linear function like XOR learnable, and both have clean derivatives (`σ' = a(1−a)`; MSE's is the residual) for the backprop step to come. Classification at scale wants ReLU plus a softmax + cross-entropy head, whose combined gradient is far cleaner — that swap is on the roadmap with MNIST.

**`float`, not `double`.** Single precision halves memory and bandwidth and matches the precision edge and embedded targets run — the direction Kestrel aims. The cost accumulator stays `double` to avoid compounding rounding across the dataset.

---

## Roadmap

Ordered deliberately: **correctness before optimisation.** A fast wrong gradient is worthless, so nothing in the second half starts until the first half is done and verified.

1. **`nn_backprop`** — analytic gradients via the chain rule, mapping the scalar rules to their matrix forms (activation → elementwise `a(1−a)`; bias → gradient passes through; matmul → the "other operand" rule, which shows up as a transpose). Validated against `nn_fdiff` by gradient checking.
2. **ReLU, softmax, cross-entropy, MNIST** — real activations and a proper classification head, trained and evaluated on a real dataset.
3. **SIMD compute path** — a vectorised matmul with an **AVX2** backend (x86) and a **NEON** backend (ARM), each benchmarked against the scalar baseline. Aligns the arena first.
4. **Cache-blocked / tiled matmul** — fixing the naive product's access pattern, benchmarked against both the scalar and SIMD versions.
5. **Beyond** — int8 quantization, compile-time graph resolution, and a GPU backend.

The destination is an **edge-inference engine**: the arena's zero-allocation hot path already gives the deterministic, fragmentation-free memory those targets need, and the single-header, zero-dependency form is built to compile anywhere from a server to a microcontroller. The design is pointed squarely there; the roadmap above is the distance still to cover.

---

## File Structure

```
kestrel/
├── README.md          — this document
├── LICENSE            — MIT
├── kestrel.h          — the single-header engine: API, plus implementation under KESTREL_CODE
├── examples/
│   └── xor.c          — the {2,2,1} XOR demo
└── build.sh           — gcc -O2 -Wall -Wextra -I. examples/xor.c -o kestrel -lm
```

`kestrel.h` is the project; everything else is a driver, a document, or a licence.
