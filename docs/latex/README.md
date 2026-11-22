# LaTeX report and slides

| File | Content |
| ---- | ------- |
| `report.tex` → `report.pdf` | Final report (LaTeX version of `docs/final_report.md`, 17 pages) |
| `slides.tex` → `slides.pdf` | Beamer slides, 16:9, metropolis theme (follows `docs/presentation/slides_outline.md`, 14 slides) |
| `figures/` | Comparison images: `original`, `scaled`, `cropped`, `carved` (our program), `seams` (first 30 seams) |

Charts are taken from `docs/figures/` (`\graphicspath{{../figures/}{figures/}}`).

## Build (no local LaTeX needed, Docker only)

```bash
# from the repository root
MSYS_NO_PATHCONV=1 docker run --rm -v "D:/Projects/TTSS:/src" -w /src/docs/latex \
    texlive/texlive:latest latexmk -pdf report.tex slides.tex
```

Alternative: upload `docs/latex/` together with the PNG files of `docs/figures/` to Overleaf
(put them in one folder; the `figures/` path is searched too).

## Regenerate the comparison images

```bash
MSYS_NO_PATHCONV=1 docker run --rm -v "D:/Projects/TTSS:/src" -v ttss-build:/build -w /src ttss-dev bash -c \
  '/build/rel/seam_carving --input tests/data/person_desert.png --output docs/latex/figures/carved.png --width 560 --mode opt --threads 8 \
   && python3 scripts/make_latex_figures.py'
```

## Placeholders to fill in

`[Team members]`, `[Course code]`, `[University]`, `[Date]` in the `\author`, `\institute` and `\date`
lines of both files.
