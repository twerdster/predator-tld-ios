# Maintaining predator-tld-ios

Read SUMMARY.md first, then this file, updated/README.md and updated/VALIDATION.json. The main README remains the historical project reference by user request; do not revise it as part of this restoration. Original source/ is pinned unchanged at a63f005bf34b041ff0dd985a8e18083f2d3065eb. Read relevant operators before making technical claims.

## Build and test

Use a native filesystem for checkout/builds, C++17/CMake 3.20+ and OpenCV 4 core/imgproc. updated/CMakeLists.txt supports a normal OpenCV package or PREDATOR_OPENCV_PREFIX for a private core/imgproc extraction. The private Linux prefix also requires its matching libtbb.so.2 through LD_LIBRARY_PATH; see updated/BUILD.md and updated/VALIDATION.md. No GPU or camera is needed for the maintained executable.

From the root:

```sh
cmake -S updated -B build/predator -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/predator -j2
ctest --test-dir build/predator --output-on-failure
python3 updated/tools/run_demo.py --build-dir build/predator --synthetic --learning 1 --output /path/to/new-directory
```

Python 3.10+ and FFmpeg are only needed by the video helper. Use a new/empty output directory to exclude stale frames. Input ROI coordinates refer to the first image resized to 320 pixels wide. Native CLI accepts grayscale PGM frames, produces annotated PPM/CSV/JSON and supports online updates on/off. Initialization always trains. Synthetic ground truth after frame 0 is only used for scoring/overlays; never pass it into tracking. --learning 0 disables subsequent updates, not initialization. Only no-blur operation and a single live TLD instance are validated.

Run affected CTest, full synthetic on/off and sanitizer paths when changing core behavior. The synthetic fixture checks complete-occlusion rejection, detector recovery and numerical IoU rather than merely successful process exit. check_original_defects.py compiles unchanged original components and requires specific diagnostic failures; build errors/timeouts are not evidence. Current witnesses cover negative-training omission, borrowed-buffer ownership and endpoint OOB. For original app attempts, archive source into a private native snapshot and put DerivedData outside original source.

## Validation scope

updated/VALIDATION.json records Linux/macOS environments, original build failure, component checks, 120-frame synthetic on/off results, sanitizer scope and 90-frame recorded-video execution. CSV results retain actual predictions/updates. This is not a broad benchmark, platform bit-parity result, full reference-TLD reproduction, iOS app build or camera/device test. Real footage has no supplied evaluation ground truth; detector reinitialization counters alone do not establish accurate reacquisition. Do not infer general learning improvement from one synthetic scene.

The port retains the recovered custom LK code and fern/scanning pipeline; do not silently replace it with an OpenCV tracker or unrelated implementation. Keep behavioral adaptations, including enabled negative fern counts, distinct from historical parity claims. The original remains available under source/ for comparison.

## Storage, assets and attribution

Current native Mac/Linux paths and SSD working root are in SUMMARY.md and updated/VALIDATION.md. Recheck actual SSD mount/UUID/free space/write access before bulk writes. Keep builds/venvs/repos off exFAT; use SSD for ordinary recordings/frames/artifacts. Preserve unrelated jobs.

Keep actual-demo GIFs < 2,000,000 bytes and MP4s < 10,000,000 bytes, with factual PNG proof and input/output hashes. Updated assets depict actual Linux annotations encoded on Mac. Label generated fixtures as synthetic. Preserve student/supervisor/source attribution, all GPL/third-party notices and the separate CC BY-NC 4.0 recording provenance. Do not apply a blanket new license to the archive or footage.

Before every commit, review/synchronize SUMMARY.md and SKILL.md, inspect source-submodule cleanliness and verify the main README remains unchanged. Check Markdown links/image rendering and artifact sizes. Save actual Claude review evidence when requested; editorial review adds no runtime coverage.
