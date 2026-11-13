# Build, input formats and adaptations

Read [README.md](README.md) for the target and evidence scope. All paths below are relative to the repository root unless stated otherwise. Use a native filesystem for checkout/build trees; bulk frames can live elsewhere.

## Dependencies

The declared minimums are a C++17 compiler, CMake 3.20, and OpenCV 4 development files for **core and imgproc**. Python 3.10+ and FFmpeg are needed only by `run_demo.py`. Tested toolchains are recorded in [VALIDATION.md](VALIDATION.md); minimum-version combinations were not separately tested.

```sh
cmake -S updated -B build/predator -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/predator -j2
ctest --test-dir build/predator --output-on-failure
```

For a private prefix containing OpenCV headers and libraries, configure with `-DPREDATOR_OPENCV_PREFIX=/path/to/prefix`. CMake finds only core/imgproc, without requiring the complete OpenCV module set. The tested Linux extraction also needs its matching `libtbb.so.2` on `LD_LIBRARY_PATH`; substituting the host’s newer TBB ABI does not work. See the maintainer-local paths in [VALIDATION.md](VALIDATION.md#maintainer-local-storage).

No OpenCV video/tracking module, GPU, camera, pretrained model or offline training is required. The existing Mac installation had broken optional videoio/imgcodecs dependencies; external FFmpeg avoided altering global packages.

## Generated and recorded demonstrations

```sh
python3 updated/tools/run_demo.py --build-dir build/predator \
  --synthetic --frames 120 --learning 1 --output /path/to/new-synthetic-on
python3 updated/tools/run_demo.py --build-dir build/predator \
  --synthetic --frames 120 --learning 0 --output /path/to/new-synthetic-off
python3 updated/tools/run_demo.py --build-dir build/predator \
  --video /path/to/input.mp4 --roi 105,35,135,85 --frames 90 \
  --learning 1 --fps 24 --output /path/to/new-recorded-demo
```

Use a new or empty output directory. A 120-frame generated run writes approximately 28 MB of uncompressed frames. The FFmpeg helper was exercised on Mac; Linux runs used the C++ CLI below with copied decoded frames. The helper decodes video to 8-bit grayscale PGM, runs the C++ executable, encodes its actual PPM annotations, and records commands/input/artifact hashes. Presentation frame rate is separate from tracker processing time. The recorded example ROI is specific to the linked camel clip; select your own box on the first frame **after resizing to 320 pixels wide**.

The C++ executable also runs without Python or FFmpeg:

```sh
build/predator/predator_demo --self-test
build/predator/predator_demo --synthetic --frames 120 --learning 1 --output /path/to/frames
build/predator/predator_demo --frames-dir /path/to/pgm-frames \
  --roi 105,35,135,85 --frames 90 --learning 1 --output /path/to/annotations
```

Input is sorted binary P5 PGM frames, with the same dimensions throughout a sequence. Hidden files, including exFAT AppleDouble files, are ignored. Outputs include numbered P6 PPM frames, `tracks.csv`, and `metrics.json`. The harness resizes to 320 pixels wide by default. Invalid/out-of-bounds initial ROIs and changed frame dimensions are rejected.

CSV coordinates use resized-image pixels. Red is the estimated box; yellow is the estimated-center trail. Synthetic green boxes are ground truth, supplied only for initial selection and later scoring/overlays. For recorded clips, CSV `gt_visible=-1` and `iou=-1` mean unmeasured; JSON ground-truth metrics are `null`. Frame 0 is the initialization box and has zero per-update counters. `--learning 0` disables subsequent online updates, not initial classifier training.

## Changes from the recovered code

Original source files and their notices remain unchanged under `source/`. [SOURCE_PROVENANCE.json](SOURCE_PROVENANCE.json) records exact original and adapted hashes. These are scoped compatibility/behavior changes, not a claim of historical numerical equivalence.

- Public OpenCV headers replace removed internals. The original custom LK loop body runs serially; its forward/backward point grid and optical-flow implementation remain. `PredatorTracker` avoids the newer `cv::Tracker` name. No replacement tracker is used.
- Borrowed image storage is not freed by `TldImage`. Owned warped patches, arrays, previous-frame images and detector candidates are released. Point-test endpoints and warped coordinates are clamped inside image bounds.
- Empty point sets return a lost track. Even-length medians use their two middle elements, and backward filtering uses backward errors rather than correlation values. Median ties remain eligible.
- Patch correlation respects image row stride. Reinitialization uses the chosen candidate’s correlation rather than the final candidate examined. Finite and minimum-size box checks reject unusable ROIs.
- The original `Classifier::train` immediately returned for every negative patch. The adaptation enables the negative counts already implemented by `Fern::train`. A positive sample followed by its negative changes the posterior from 1 to 0.5; the original stays at 1. This deliberately changes initialization and later learning.
- A fixed random seed supports repeatability within an environment. Results differ between systems; random features themselves were not compared across platforms.

The recovered scanning scales, decision thresholds and global single-instance model remain. Feature-scale constructor arguments were unused originally and remain unused. Optional spatial blur is not validated; all reported runs use the no-blur path. See [VALIDATION.md](VALIDATION.md) for diagnostic controls, acceptance gates and limits.
