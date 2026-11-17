# Image Sources

All photos come from Unsplash (https://unsplash.com/license).
The photographer is taken from the original download file name.

| File | Unsplash link | Photographer (from file name) | Original size | Purpose |
| ---- | ------------- | ----------------------------- | ------------- | ------- |
| boat_sea | https://unsplash.com/photos/ZGYWhxaiMOc | daniel-stenholm | 3945x2957 | Good case: small subject, smooth sea and sky |
| tree_field | https://unsplash.com/photos/pVV39dmFCEE | ja-kubislav | 5184x3456 | Good case: lone tree; regular field furrows |
| person_desert | https://unsplash.com/photos/l2K7561Pj80 | jakob-owens | 6000x4000 | Good case: very smooth background |
| building_windows | https://unsplash.com/photos/Hxr7HCW7MDE | pawel-czerwinski | 3311x4967 (portrait) | Hard case: straight lines, repeated windows |
| face_portrait | https://unsplash.com/photos/SNvrVyUYyK0 | vitaly-gariev | 3840x2160 (4K) | Hard case: human face; also a 4K benchmark candidate |

## Folders

- `images/original/`: full-resolution JPG files. **Not committed** (ignored by git). Download them again from the links above if needed.
- `tests/data/`: reduced PNG copies (longest side 800 px, Lanczos resampling with Python Pillow). These are committed and used for tests and demos.
  PNG is used because it is lossless: every library decodes exactly the same pixels, so outputs can be compared bit for bit.
