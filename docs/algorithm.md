# Seam Carving Algorithm

**Milestone:** M0 – Research and Architecture
**Status:** Draft for team review (2026-11-17)
**Reference:** S. Avidan and A. Shamir, "Seam Carving for Content-Aware Image Resizing", ACM SIGGRAPH 2007 (URL in `link.md`).

This document explains the original algorithm and defines **exactly** the variant this project implements.
Section 6 lists every place where we had to choose a detail that the paper leaves open. Those choices are
**our implementation decisions**, not part of the paper.

---

## 1. Idea

Normal resizing (scaling) squeezes every pixel equally, so important objects get distorted.
Cropping throws away whole borders, even if they contain important content.

Seam carving removes the **least important pixels** instead. To make an image one pixel narrower,
it removes one **vertical seam**: a connected path of pixels from the top row to the bottom row,
exactly one pixel per row. The seam is chosen so that the sum of the "importance" (energy)
of its pixels is as small as possible. Repeating this `k` times removes `k` columns.

Because smooth regions (sky, sea, sand) have low energy, seams tend to pass through them,
and objects with strong edges (people, boats, buildings) are kept.

---

## 2. Notation

| Symbol | Meaning |
| ------ | ------- |
| `I` | Gray-scale image, values 0–255 |
| `W`, `H` | Current width and height of the image |
| `x`, `y` | Column index `0..W-1`, row index `0..H-1` (row 0 is the top) |
| `e(x, y)` | Energy of pixel `(x, y)` |
| `M(x, y)` | Cumulative minimum energy of the best seam ending at `(x, y)` |
| `s` | A vertical seam, stored as `s[y]` = column of the seam in row `y` |
| `k` | Number of seams to remove (`k = W - targetWidth`) |

---

## 3. The Algorithm from the Paper

### 3.1 Energy function

The paper's basic energy function (called `e1`) is the sum of the absolute gradients:

```
e1(I) = | dI/dx | + | dI/dy |
```

A pixel on a strong edge has a large gradient and therefore a large energy.

### 3.2 Vertical seam

A vertical seam is a set of pixels

```
s = { (s[y], y) : y = 0 .. H-1 }   with   | s[y] - s[y-1] | <= 1
```

so it goes from top to bottom, contains exactly one pixel per row, and is
**8-connected** (each step moves to the pixel directly below, below-left or below-right).

The cost of a seam is `E(s) = sum over y of e(s[y], y)`. The optimal seam is `s* = argmin E(s)`.

### 3.3 Finding the optimal seam with dynamic programming

There are about `W * 3^(H-1)` possible seams, so trying all of them is impossible.
Dynamic programming (DP) solves it in `O(W * H)` time.

`M(x, y)` = the smallest possible cost of a seam that starts anywhere in row 0 and ends at pixel `(x, y)`:

```
M(x, 0) = e(x, 0)
M(x, y) = e(x, y) + min( M(x-1, y-1), M(x, y-1), M(x+1, y-1) )      for y >= 1
```

Neighbours outside the image (`x-1 < 0` or `x+1 >= W`) are simply ignored.

After filling `M`:

1. **Endpoint:** the minimum value in the last row `M(., H-1)` is the cost of the optimal seam.
2. **Backtracking:** starting from that endpoint, go up one row at a time and move to whichever of the
   three upper neighbours has the smallest `M`. This reconstructs the seam.

### 3.4 Removing the seam

In every row `y`, delete pixel `s[y]` and shift the pixels to its right one position to the left.
The image becomes `(W-1) x H`.

### 3.5 Repeating

After a seam is removed, the image has changed, so the energy and `M` must be computed again before
the next seam is found. Removing `k` seams means running the whole pipeline `k` times.

### 3.6 Reducing the height

A horizontal seam (left to right, one pixel per column) is handled the same way with rows and
columns swapped. The simplest implementation is: **transpose** the image, remove vertical seams,
transpose back.

### 3.7 Parts of the paper that are out of scope

The paper also describes: other energy functions (e.g. `eHoG`), the optimal order of vertical and
horizontal seams when both dimensions shrink (a second DP over a "transport map"), image enlarging
by seam insertion, object removal/protection masks, and multi-size images. These are **not** implemented
in this project. Our goal is the parallelization of the basic algorithm.

---

## 4. Pseudocode (the variant we implement)

```text
function CARVE(colorImage, targetWidth, targetHeight):
    if targetHeight < H:
        // height reduction: transpose, carve width, transpose back (done after width)
    gray  <- convertToGray(colorImage)            // once, at the start
    width <- colorImage.cols
    while width > targetWidth:                    // k iterations, strictly sequential
        energy <- COMPUTE_ENERGY(gray, width)                 // stage 1
        M      <- COMPUTE_CUMULATIVE(energy, width)           // stage 2
        end    <- FIND_MIN_ENDPOINT(M, width)                 // stage 3
        seam   <- BACKTRACK_SEAM(M, width, end)               // stage 4
        REMOVE_SEAM(colorImage, gray, width, seam)            // stage 5
        width  <- width - 1
    return colorImage[:, 0 .. targetWidth-1]

function COMPUTE_ENERGY(I, W):
    for y in 0 .. H-1:
        for x in 0 .. W-1:
            xl <- max(x-1, 0);   xr <- min(x+1, W-1)
            yu <- max(y-1, 0);   yd <- min(y+1, H-1)
            e(x,y) <- |I(xr,y) - I(xl,y)| + |I(x,yd) - I(x,yu)|

function COMPUTE_CUMULATIVE(e, W):
    for x in 0 .. W-1:  M(x,0) <- e(x,0)
    for y in 1 .. H-1:                            // rows: sequential (dependency!)
        for x in 0 .. W-1:                        // columns: independent
            best <- M(x, y-1)
            if x > 0   : best <- min(best, M(x-1, y-1))
            if x < W-1 : best <- min(best, M(x+1, y-1))
            M(x,y) <- e(x,y) + best

function FIND_MIN_ENDPOINT(M, W):
    end <- 0
    for x in 1 .. W-1:
        if M(x,H-1) < M(end,H-1): end <- x        // strict '<': smallest column wins ties

function BACKTRACK_SEAM(M, W, end):
    s[H-1] <- end
    for y in H-1 down to 1:
        x <- s[y];  best <- -1
        for c in (x-1, x, x+1):                   // in THIS order
            if 0 <= c < W and (best == -1 or M(c,y-1) < M(best,y-1)):
                best <- c                         // strict '<': smallest column wins ties
        s[y-1] <- best

function REMOVE_SEAM(colorImage, gray, W, s):
    for y in 0 .. H-1:
        shift colorImage(s[y]+1 .. W-1, y) and gray(s[y]+1 .. W-1, y) one position left
```

---

## 5. Worked Example (4 x 4)

This example is the reference for unit tests in M1 and M3. All values were computed by hand and
double-checked with a small independent script.

### 5.1 Input gray image `I`

```
        x=0  x=1  x=2  x=3
y=0      10   20   30   40
y=1      50   10   60   20
y=2      30   70   10   50
y=3      20   40   60   10
```

### 5.2 Energy `e`

Rule: `e(x,y) = |I(xr,y) - I(xl,y)| + |I(x,yd) - I(x,yu)|`, out-of-range indices are clamped to the border.

Some cells in detail:

| Cell | Horizontal part | Vertical part | e |
| ---- | --------------- | ------------- | - |
| (0,0) | `|I(1,0) - I(0,0)| = |20-10| = 10` (left clamped to x=0) | `|I(0,1) - I(0,0)| = |50-10| = 40` (up clamped to y=0) | 50 |
| (1,1) | `|I(2,1) - I(0,1)| = |60-50| = 10` | `|I(1,2) - I(1,0)| = |70-20| = 50` | 60 |
| (2,2) | `|I(3,2) - I(1,2)| = |50-70| = 20` | `|I(2,3) - I(2,1)| = |60-60| = 0` | 20 |
| (3,3) | `|I(3,3) - I(2,3)| = |10-60| = 50` (right clamped) | `|I(3,3) - I(3,2)| = |10-50| = 40` (down clamped) | 90 |

Full energy map:

```
        x=0  x=1  x=2  x=3
y=0      50   30   50   30
y=1      60   60   30   50
y=2      70   50   20   50
y=3      30   70   80   90
```

### 5.3 Cumulative energy `M`

Row 0 is copied from `e`. Each next cell adds the smallest of its (up to) three upper neighbours.

| Cell | Calculation | M |
| ---- | ----------- | - |
| (0,1) | `60 + min(50, 30)` (no left neighbour) | 90 |
| (1,1) | `60 + min(50, 30, 50)` | 90 |
| (2,1) | `30 + min(30, 50, 30)` | 60 |
| (3,1) | `50 + min(50, 30)` (no right neighbour) | 80 |
| (0,2) | `70 + min(90, 90)` | 160 |
| (1,2) | `50 + min(90, 90, 60)` | 110 |
| (2,2) | `20 + min(90, 60, 80)` | 80 |
| (3,2) | `50 + min(60, 80)` | 110 |
| (0,3) | `30 + min(160, 110)` | 140 |
| (1,3) | `70 + min(160, 110, 80)` | 150 |
| (2,3) | `80 + min(110, 80, 110)` | 160 |
| (3,3) | `90 + min(80, 110)` | 170 |

```
        x=0  x=1  x=2  x=3
y=0      50   30   50   30
y=1      90   90   60   80
y=2     160  110   80  110
y=3     140  150  160  170
```

### 5.4 Endpoint and backtracking

- Last row: `140, 150, 160, 170` → minimum 140 at **x = 0**. `s[3] = 0`.
- Row 2, candidates x = 0, 1 (x = -1 is outside): `M = 160, 110` → **x = 1**. `s[2] = 1`.
- Row 1, candidates x = 0, 1, 2: `M = 90, 90, 60` → **x = 2**. `s[1] = 2`.
- Row 0, candidates x = 1, 2, 3: `M = 30, 50, 30` → **tie** between x = 1 and x = 3.
  Tie rule "smallest column wins" → **x = 1**. `s[0] = 1`.

**Seam:** `s = [1, 2, 1, 0]`, total energy `30 + 30 + 50 + 30 = 140` (equals `M(0,3)`, as expected).

### 5.5 Image after removing the seam (3 x 4)

```
y=0      10   30   40      (removed 20 at x=1)
y=1      50   10   20      (removed 60 at x=2)
y=2      30   10   50      (removed 70 at x=1)
y=3      40   60   10      (removed 20 at x=0)
```

### 5.6 Second example: uniform image (all ties)

For a 4 x 4 image where every pixel is 7: all energies are 0, all `M` values are 0, the endpoint is
x = 0 and the seam is `[0, 0, 0, 0]`. This example is important for the parallel version (M2): when
every value ties, any mistake in tie-breaking immediately produces a different seam.

---

## 6. Implementation Decisions (not specified by the paper)

| # | Decision | Reason |
| - | -------- | ------ |
| D1 | Gradient = **central difference without division by 2**: `|I(x+1)-I(x-1)| + |I(y+1)-I(y-1)|` | Integer only; dividing by 2 would only scale all energies and not change the chosen seam, but would lose precision. Maximum value 255 + 255 = 510. |
| D2 | Energy computed on the **gray** image, gray computed **once** by `cv::cvtColor(BGR2GRAY)` | One channel = 3x less work. Removing seams from the gray image too (instead of re-converting) avoids repeated rounding and is cheaper. |
| D3 | Borders: indices are **clamped** to the nearest valid pixel, using the **current width**, not `mat.cols` | Every pixel gets an energy; clamping is simple and deterministic. Border pixels get a one-sided difference. |
| D4 | Energy and `M` stored as **32-bit signed integers** | Exact arithmetic → sequential and parallel results can be compared bit for bit. Overflow check: `510 * 4320 rows ≈ 2.2 million`, far below `2^31 ≈ 2.1 billion`. |
| D5 | Tie rule: **smallest column wins**, both for the endpoint and in backtracking (candidates in order x-1, x, x+1, strict `<`) | Makes the result unique and independent of thread count. |
| D6 | Energy is **recomputed for the whole image** after every seam | Simplest correct baseline. Updating only pixels near the removed seam is an optimization for M5. |
| D7 | Height reduction by **transpose**, after the width has been reduced | Reuses the vertical-seam code. The paper's optimal seam order is out of scope. |

Special sizes follow directly from the clamping rule: if `W = 1` the horizontal part is 0;
if `H = 1` the vertical part is 0.

---

## 7. Computational Complexity

For one seam on a `W x H` image:

| Stage | Work | Notes |
| ----- | ---- | ----- |
| 1. Energy | `O(W * H)` | Constant work per pixel |
| 2. DP (cumulative `M`) | `O(W * H)` | 3 reads + 1 min + 1 add per pixel |
| 3. Minimum endpoint | `O(W)` | One pass over the last row |
| 4. Backtracking | `O(H)` | 3 comparisons per row |
| 5. Seam removal | `O(W * H)` | Shifts on average half a row per row, for color + gray |
| **Total per seam** | **`O(W * H)`** | |

Removing `k` seams: the width goes `W, W-1, ..., W-k+1`, so the total work is

```
sum_{i=0}^{k-1} (W - i) * H  =  k * H * (W - (k-1)/2)   =  O(k * W * H)
```

Example (calculation, not a measurement): a 3840 x 2160 image reduced by 30 % removes
`k = 1152` seams; the pipeline processes about `1152 * 2160 * 3264 ≈ 8.1 * 10^9` pixel-stage visits
per stage that is `O(W * H)`.

**Memory:** `O(W * H)` – color image, gray image, energy map and `M` (each allocated once at the original size).

Stages 1, 2 and 5 dominate the run time; stages 3 and 4 are small. This is why the parallelization
effort (see `parallelization_strategy.md`) focuses on stages 1, 2 and 5.
