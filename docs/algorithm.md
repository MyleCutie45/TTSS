# Algorithm – Seam Carving (Avidan & Shamir, 2007)

This document describes the algorithm we implement. Section 8 records our implementation choices where the paper leaves details open. Those choices are ours, not the paper's.

Reference: S. Avidan, A. Shamir. *Seam Carving for Content-Aware Image Resizing*. ACM Transactions on Graphics 26(3), Article 10, SIGGRAPH 2007.

---

## 1. Problem

Given an image `I` of height `H` and width `W`, produce an image of width `W' < W` (and optionally height `H' < H`) that keeps the important content undistorted.

- Uniform scaling distorts objects.
- Cropping removes content at the borders.

Seam carving instead removes, one at a time, connected paths of *unimportant* pixels.

## 2. Notation

| Symbol | Meaning |
| ------ | ------- |
| `H`, `W` | current image height (rows) and width (columns) |
| `(i, j)` | pixel at row `i`, column `j`, 0-based |
| `g(i, j)` | grayscale intensity, integer in [0, 255] |
| `e(i, j)` | energy of pixel (i, j) |
| `M(i, j)` | cumulative minimum energy |
| `s[i]` | column of the seam in row `i` |

Mapping to other documents:

| Source | Writes | Means |
| ------ | ------ | ----- |
| Paper | image `n × m`, indices from 1, vertical seam `{(x(i), i)}` | n = H, m = W, `x(i)` = `s[i]` |
| `M2.md` | `M(x, y)` with x = column, y = row | `M2`'s `M(x,y)` = our `M(y, x)` |

## 3. Energy function

The paper's main energy function (Eq. 1):

```text
e1(I) = |∂I/∂x| + |∂I/∂y|
```

The paper does not specify a discretization. Ours (decision D2) uses central differences without division, on integer grayscale, with clamped borders:

```text
jl = max(j-1, 0)     jr = min(j+1, W-1)
iu = max(i-1, 0)     id = min(i+1, H-1)

e(i, j) = |g(i, jr) - g(i, jl)| + |g(id, j) - g(iu, j)|        0 ≤ e ≤ 510
```

Grayscale is computed once from BGR with integer arithmetic:

```text
g = (299·R + 587·G + 114·B + 500) / 1000        (integer division)
```

After that, seams are removed from the gray buffer exactly as from the color buffer, so gray never has to be recomputed.

## 4. Seams

A **vertical seam** is a sequence `s[0..H-1]` with one pixel per row, 8-connected:

```text
0 ≤ s[i] < W          for all i
|s[i] - s[i-1]| ≤ 1   for all i ≥ 1
```

Seam cost and optimal seam (paper Eq. 4):

```text
E(s) = Σ_{i=0}^{H-1} e(i, s[i])          s* = argmin_s E(s)
```

There are up to `W · 3^(H-1)` seams, so exhaustive search is infeasible. Dynamic programming finds `s*` in O(H·W).

## 5. Dynamic programming

**State:** `M(i, j)` = minimum cost of any connected path from row 0 to pixel (i, j), including e(i, j).

```text
M(0, j) = e(0, j)
M(i, j) = e(i, j) + min( M(i-1, j-1), M(i-1, j), M(i-1, j+1) )     for i ≥ 1
```

Neighbours outside [0, W-1] are skipped, so only 2 parents exist at the left and right borders.

**Correctness (induction on i).**
- Row 0 holds trivially.
- For i ≥ 1, any path to (i, j) passes through exactly one parent (i-1, j') with |j' - j| ≤ 1. Its cost is at least `M(i-1, j') + e(i, j) ≥ M(i, j)`.
- Equality holds when the path extends an optimal path to the cheapest parent.

**Dependency structure.**
- Row i depends only on row i-1.
- Cells in the same row are independent of each other.

This property is the basis of the parallel DP (see `parallelization_strategy.md`).

## 6. Seam extraction (backtracking)

```text
s[H-1] = argmin_j M(H-1, j)
for i = H-2 down to 0:
    j = s[i+1]
    s[i] = argmin over j' ∈ {j, j-1, j+1} ∩ [0, W-1] of M(i, j')
```

Tie-breaking (decision D2), which must be identical in every implementation:

- **Last-row argmin:** the smallest index `j` achieving the minimum (left-to-right scan with strict `<`).
- **Parent choice:** prefer the middle `j`, then `j-1`, then `j+1`, using strict `<` comparisons in that order.

Because several seams can have the same minimal cost, a fixed tie rule is what makes the sequential and parallel results identical.

## 7. Seam removal and main loop

For every row `i`, delete the pixel at column `s[i]` and shift the pixels to its right one position left. Apply this to both the color and the gray buffer.

```text
while W > targetW:
    computeEnergy(gray → energy)        O(H·W)
    computeCumulative(energy → M)       O(H·W)
    j* = argminLastRow(M)               O(W)
    backtrack(M, j* → s)                O(H)
    removeSeam(color, gray, s)          O(H·W) worst case, ~H·W/2 average
    W = W - 1
```

Energy must be recomputed (at least near the seam) after each removal. Pixels that were on opposite sides of the seam are now neighbours.

**Greedy nature.** Each iteration removes the optimal seam of the *current* image. The resulting set of `k` seams is not guaranteed to be the globally cheapest set of `k` seams. This is the paper's approach (Section 4.1) and ours.

**Height reduction.** A horizontal seam of `I` is a vertical seam of the transpose `Iᵀ`.
1. Reduce the width first.
2. Transpose, reduce the width (which is the original height), transpose back.

The paper's optimal-order transport map (Section 4.2) is out of scope (decision D6).

## 8. Worked example

Energy matrix (4 × 5):

```text
      j=0 j=1 j=2 j=3 j=4
i=0    3   1   4   1   5
i=1    9   2   6   5   3
i=2    5   8   9   7   9
i=3    3   2   3   8   4
```

Cumulative matrix M:

```text
i=0    3   1   4   1   5
i=1   10   3   7   6   4      e.g. M(1,0) = 9 + min(3,1) = 10
i=2    8  11  12  11  13
i=3   11  10  14  19  15
```

- Last-row minimum: 10 at j = 1.
- Backtrack: row 2 picks among (8, 11, 12) → j = 0; row 1 among (10, 3) → j = 1; row 0 among (3, 1, 4) → j = 1.
- Seam `s = [1, 1, 0, 1]`, cost `1 + 2 + 5 + 2 = 10`.
- This example becomes a unit test in M1.

## 9. Complexity

| Step | Per seam | For k seams |
| ---- | -------- | ----------- |
| Energy (full recompute) | Θ(H·W) | Θ(k·H·W) |
| DP | Θ(H·W) | Θ(k·H·W) |
| Argmin | Θ(W) | Θ(k·W) |
| Backtrack | Θ(H) | Θ(k·H) |
| Removal | Θ(H·W) | Θ(k·H·W) |
| **Total** | Θ(H·W) | Θ(k·H·W) |

Memory: color 3·H·W bytes, gray H·W bytes, energy and M 4·H·W bytes each.
- 1920×1080: about 6.2 + 2.1 + 8.3 + 8.3 ≈ 25 MB.
- 7680×4320: about 400 MB.

Upper bound for M: `H · 510` (e.g. 4320 · 510 = 2 203 200), far below the `int32` limit.

## 10. What is out of scope

The paper also covers the following, which are not required by M0–M6 (decision D6):

- seam insertion (enlarging)
- optimal seam order (transport map)
- content amplification
- gradient-domain carving
- object removal
- multi-size images
- other energy functions

Forward energy (Rubinstein et al. 2008) is a different algorithm and is also out of scope.
