# Recorded validation, 2026-11-14

The maintained executable uses recovered source algorithms. The original submodule and main README were not changed. [BUILD.md](BUILD.md) gives reusable build/run commands and describes adaptations; [VALIDATION.json](VALIDATION.json) retains measured results and artifact hashes.

## Tested environments

| Platform | Toolchain | Dependencies |
| --- | --- | --- |
| Ubuntu 22.04 x86_64, kernel 6.8.0-138-generic | GCC 11.4.0, CMake 3.22.1 | OpenCV 4.5.4 core/imgproc, matching TBB 2 |
| macOS 15.1.1 arm64 | Apple clang 16.0.0, CMake 4.0.3 | OpenCV 4.12.0 core/imgproc |

No GPU was used. Python 3.10 on Linux and 3.14 on Mac were available; FFmpeg 8.1.1 on Mac decoded/encoded videos. The existing Mac OpenCV videoio/imgcodecs dependency chain was broken; external FFmpeg avoided global package changes. Declared minimum compiler/CMake/Python combinations were not separately tested.

## Controlled scene

The generated 120-frame fixture contains a stable textured target, static background, modest translation, complete absence during frames 50–64, and relocation at frame 65. Only the first box initializes the algorithm. Later ground truth is used solely for overlays/scoring; it is not passed into tracking, scanning or reinitialization.

| Result | Linux updates on | Linux updates off | Mac updates on | Mac updates off |
| --- | ---: | ---: | ---: | ---: |
| Visible frames | 105 | 105 | 105 | 105 |
| Mean visible-frame IoU | 0.946363 | 0.874728 | 0.934852 | 0.934852 |
| Visible frames with IoU ≥ 0.5 | 105 | 97 | 105 | 105 |
| First post-occlusion frame with IoU ≥ 0.5 | 65 | 65 | 65 | 65 |
| Detector reinitialization decisions | 2 | 7 | 2 | 2 |
| Lost-frame reports | 15 | 16 | 15 | 15 |
| Positive / negative online updates | 664 / 848 | 0 / 0 | 642 / 1029 | 0 / 0 |

All hidden frames were reported lost in each run. The Linux updates-off run additionally lost one visible frame. In the updates-on Linux run, frame 65 was accepted through detector reinitialization. Initialization always trains even with subsequent updates disabled. Different system results do not establish which random features differed: feature arrays were not compared. Linux learning improved this single fixture; identical Mac geometry prevents a general learning-benefit claim.

For generated runs of at least 70 frames the executable requires mean visible IoU > 0.75, at least 90% of visible frames at IoU ≥ 0.5, first post-occlusion IoU ≥ 0.5 at frame 65–70, and at least one detector reinitialization. Each fully hidden frame must yield an invalid/lost box. Online updates must include both positive and negative counts when enabled, and neither when disabled. These are regression acceptance thresholds for this selected scene, not benchmark standards. Frame 0 is the supplied initialization box and contributes to visible-frame counts.

## Recorded footage

A 90-frame excerpt of DAVIS camel, resized to 320 pixels wide, used a manually selected first-frame ROI `[105,35,135,85]`. DAVIS segmentation masks were not downloaded, converted to boxes or used for evaluation. Linux reported four reinitializations, four lost frames and 3,250/10,610 positive/negative online updates. Mac reported four reinitializations, zero lost frames and 2,653/8,744 updates. Reinitialization counters alone do not prove correct reacquisition. The videos provide qualitative recorded-input execution, without per-frame accuracy, a real occlusion score or a dataset benchmark.

The Python/FFmpeg helper was exercised on Mac. Linux consumed grayscale PGM frames decoded on Mac. Published GIF/MP4 files were encoded on Mac from **actual Linux-annotated PPM frames**. Encoding is outside tracker processing time. Synthetic figures use the Linux updates-on run; recorded figures use the Linux camel run. The composite PNG was assembled with Pillow from those annotated frames and factual captions. The data source, CC BY-NC 4.0 terms, modifications and raw file hash are in [DATA_PROVENANCE.json](DATA_PROVENANCE.json).

## Component and sanitizer checks

One CTest program checks analytical IoU, endpoint bounds, borrowed-buffer lifetime, fern posterior updates, invalid initial ROIs, a valid border-touching ROI, and rejection of a subsequent textureless frame. This is one program containing several assertions, not a large independent test suite.

```sh
cmake -S updated -B asan -DCMAKE_BUILD_TYPE=Debug -DPREDATOR_SANITIZE=ON \
  -DPREDATOR_OPENCV_PREFIX=/path/to/private/opencv/prefix
cmake --build asan -j2
ASAN_OPTIONS=detect_leaks=1 asan/predator_demo --self-test --synthetic \
  --frames 120 --learning 1 --output /path/to/new-synthetic-asan
```

The Linux run completed with exit 0 and no AddressSanitizer, UndefinedBehaviorSanitizer or leak report. [Saved output](results/linux-asan.log). The Mac completed a 120-frame ASan/UBSan run without the Linux leak-detection claim. Project sources were instrumented; prebuilt OpenCV was not. These checks cover exercised paths, not exhaustive fuzzing or the optional spatial-blur path.

## Unchanged original negative controls

```sh
python3 updated/tools/check_original_defects.py --work-dir /path/to/native/original-check
```

For a private Linux extraction, add `--cxx g++ --opencv-prefix /path/to/prefix` and retain its library path. The script supplies public compatibility headers, then compiles **unchanged** original `Classifier.cpp`, `Fern.cpp`, `PointTest.cpp` and `TldImage.cpp`. Build errors are not accepted as defect evidence.

Both systems produced the three expected failures: the original posterior remains 1 after positive then negative training rather than 0.5; borrowed image destruction frees caller storage and triggers a double-free; an endpoint feature triggers an out-of-bounds (heap-buffer-overflow) read. The negative-class witness uses a plain executable and its exact posterior output. Ownership/endpoint witnesses require their specific sanitizer diagnostics. This avoids treating an unrelated Mac TBB sanitizer shutdown fault seen in an early experimental witness as classifier evidence. [Linux cases](results/original-defects.json), [diagnostic excerpts](results/original-defect-excerpts.txt); both systems are recorded in [VALIDATION.json](VALIDATION.json). Updated counterpart assertions pass.

## Original application attempt

An untouched `git archive` snapshot of the pinned original was built with:

```sh
xcodebuild -project original-snapshot/Predator.xcodeproj -scheme Predator \
  -sdk iphonesimulator -configuration Debug -derivedDataPath original-snapshot-build \
  CODE_SIGNING_ALLOWED=NO build
```

Xcode 16.1 rejected the deprecated i386 architecture **before source compilation**. It also warned that the iOS 6 deployment target is outside the supported range. No later app compilation result, simulator/device execution, camera capture or historical FPS was established. [Saved excerpt](results/original-xcodebuild.txt).

## Maintainer-local storage

These are session-specific paths, not prerequisites for another checkout:

- Native Mac builds/logs: `/Users/aaron/.codex/project-revivals-2026-11-14/predator`.
- Native Linux builds, original snapshots and run files: `/home/aaron/project-revivals-20261114/predator` on SSH host `aaron`.
- Read-only private Linux dependency extraction: `/home/aaron/project-revivals-20261114/freehand/deps/usr`. Configure `PREDATOR_OPENCV_PREFIX` to this prefix and set `LD_LIBRARY_PATH` to its `lib/x86_64-linux-gnu` directory, which includes matching `libtbb.so.2`. No apt/brew system installation was made for Predator.
- Bulk recordings and decoded/annotated frames: `/Volumes/AgentSSD/agent-work/project-revivals-2026-11-14/predator`. SSD volume UUID `3233266B-11A5-3130-ADF0-972393997381` was checked mounted/writable with free space before use. Repositories/builds remained on native filesystems. Recheck the mount before future bulk writes.

## Limits

The original iOS application remains unrecovered as a runnable app. These runs establish neither full reference-TLD equivalence nor general robustness, cross-platform bit parity, historical FPS or benchmark performance. The target retains a single-live-instance state model. Source attribution/GPL notices and separate recording terms remain; the main README stays the historical reference. Two actual tools-disabled Claude editorial reviews were completed; the second found no material issues, and minor qualification edits were applied. Private review logs are retained with the native Mac run files. Editorial review adds no runtime coverage.
