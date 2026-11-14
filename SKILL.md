# Maintaining predator-tld-ios

Read SUMMARY.md first, then this file, updated/README.md and updated/VALIDATION.json. The main README remains the historical project reference by user request; do not revise it as part of this restoration. Original source/ is pinned unchanged at a63f005bf34b041ff0dd985a8e18083f2d3065eb. Read relevant operators before making technical claims.

## Updated-work navigation

Aaron explicitly authorized the root README’s local `New!` badge and exact “Updated and ported for modern hardware” link on 2026-11-14. Keep both linked to `updated/README.md#updated-and-ported-for-modern-hardware`; retain the italic historical-reference qualifier and preserve the rest of the historical root reference. The matching updated README heading must introduce a short, factual account of the actual changes and limitations before the existing visuals, setup and evidence. Keep implementation descriptions and detailed results under `updated/`.

This navigation addition is the authorized exception to older root-README preservation instructions below. Verify the historical body after removing only that block, preserve source hashes/pins and leave historical validation hashes attached to their original freezes. Documentation-only navigation edits do not add runtime coverage or require a package rebuild.

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

## Native frontend workflow

Read updated/platforms/README.md, the platform-specific README and validation records before claiming platform compatibility. The C++ Session calls recovered Tld directly; keep it serial because the recovered core has one live global instance. Later synthetic truth is drawing/scoring only. Native frontends may use generated input or bundled unannotated camel pixels; never substitute prior tracked output as input. Initialization trains even with online updates off. Base CTest now has two programs, including Session synthetic on/off and recorded-input regression.

UIKit builds use updated/platforms/ios/CMakeLists.txt, a modern official iOS OpenCV framework and private signing configuration. Android uses the checked-in Gradle wrapper and cached/private SDK/NDK/OpenCV; see its README. Keep build/signing/device values outside source. Device installation/launch/capture is coordinated centrally during this task; do not contend for attached phones. Runtime traces and actual screen captures belong under updated/validation/native-platforms and assets. A compile, simulator or virtual-display result must not be described as physical-device proof.

The final frozen iOS and Android apps each completed recorded camel autoplay on the physical devices named in SUMMARY.md. Check ios-artifact.json/ios-result.json and android-artifact.json/android-camel-trace.jsonl/android-result.json before repeating the claims. Android summaries must be derived from event=frame rows, not the reset event; require contiguous frames 0–89, final done:true and the per-frame learning flags. Lost reports and detector reinitialization counters do not establish recorded-video accuracy. Mobile synthetic occlusion, manual controls and rapid lifecycle changes remain outside these runs.

Mac AppKit is built by the base CMake target PredatorDesktop; Linux Tk calls libpredator_session through ctypes. Native app-window screenshots may be sampled/encoded, with capture rate distinct from processing time. Current Linux Tk8.6 private prefix is /home/aaron/project-revivals-20261114/predator/tk-prefix; PYTHONPATH includes usr/lib/python3.10 and its lib-dynload, LD_LIBRARY_PATH includes usr/lib and usr/lib/x86_64-linux-gnu, and TK_LIBRARY is usr/share/tcltk/tk8.6 under that prefix. Reuse read-only, no global installation. Mac system Tk8.5 rendered blank, so AppKit is the maintained Mac UI.

Use a new output directory for each native run and check completion records against their input label. A failed Tk reset stops computation but may leave prior correctly labeled output files on disk. The native Session validates recorded input before replacing its previous session; CTest covers a synthetic prior session and the actual missing-file Tk startup control is in updated/results/linux-invalid-input-ui-control.json. Android device validation uses force-stop before launch; rapid Activity destruction/relaunch timing is not covered.

## Validation scope

updated/VALIDATION.json records Linux/macOS environments, original build failure, component checks, 120-frame synthetic on/off results, sanitizer scope and 90-frame recorded-video execution. CSV results retain actual predictions/updates. That earlier record does not cover native UI/device work. Neither record is a broad benchmark, platform bit-parity result or full reference-TLD reproduction; no frontend implements camera capture. Real footage has no supplied evaluation ground truth; detector reinitialization counters alone do not establish accurate reacquisition. Do not infer general learning improvement from one synthetic scene.

The port retains the recovered custom LK code and fern/scanning pipeline; do not silently replace it with an OpenCV tracker or unrelated implementation. Keep behavioral adaptations, including enabled negative fern counts, distinct from historical parity claims. The original remains available under source/ for comparison.

## Storage, assets and attribution

Current native Mac/Linux paths and SSD working root are in SUMMARY.md and updated/VALIDATION.md. Recheck actual SSD mount/UUID/free space/write access before bulk writes. Keep builds/venvs/repos off exFAT; use SSD for ordinary recordings/frames/artifacts. Preserve unrelated jobs.

Keep actual-demo GIFs < 2,000,000 bytes and MP4s < 10,000,000 bytes, with factual PNG proof and input/output hashes. Earlier headless assets depict Linux annotations; new assets also include actual AppKit/Tk and physical-device UI captures, each identified by its platform record. Label generated fixtures as synthetic. Preserve student/supervisor/source attribution, all GPL/third-party notices and the separate CC BY-NC 4.0 recording provenance. Do not apply a blanket new license to the archive or footage.

Before every commit, review/synchronize SUMMARY.md and SKILL.md, inspect source-submodule cleanliness and verify the main README remains unchanged. Check Markdown links/image rendering and artifact sizes. Save actual Claude review evidence when requested; editorial review adds no runtime coverage.

The unannotated `.gray` fixture is marked binary for Git diffs. Preserve its recorded byte hash and the bundled third-party license files verbatim; their upstream formatting is outside maintainer prose/whitespace checks.
