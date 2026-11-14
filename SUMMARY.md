# predator-tld-ios: maintenance context

Read this file first, then SKILL.md. Current restoration record: 2026-11-14. The main README is deliberately retained as the historical project reference; use updated/README.md for the runnable maintained target.

## Updated-work navigation (2026-11-14)

Aaron authorized a top-of-page `New!` badge and “Updated and ported for modern hardware” link in the root README. Both lead to the corresponding heading in `updated/README.md`, which now opens with a brief description of the actual port, repairs or reimplementation and its limits. An italic line marks the following content as the historical reference. The historical root reference body remains unchanged; current implementation details stay under `updated/`.

This documentation change does not alter source, submodule pins, media or recorded validation. Earlier root-README byte-identity statements and hashes apply to their recorded baseline; current preservation checks remove only the authorized navigation block before comparing the historical body.

## Preserved original

source/ remains unchanged at kenguish/Predator-iOS-Technion commit a63f005bf34b041ff0dd985a8e18083f2d3065eb. Students Marian Mansour and Majd Srour; the report credits Aaron Wetzler/Yaron Honen, while upstream README credits Aaron Wetzler/Ron Kimmel and Anton Anton for UI/graphics. Preserve these differences and original GPL/third-party notices.

An untouched git-archive snapshot was attempted with Xcode 16.1/iPhoneSimulator 18.1. The build stopped before compilation at deprecated i386 architecture, with an unsupported iOS 6 deployment warning. The original iOS app has not been restored or run on a device.

## Updated executable

updated/ builds the recovered custom forward/backward Lucas–Kanade tracker, scanning detector, fern classifier, correlation decisions and conditional updates as C++17. It uses OpenCV 4 core/imgproc primitives, not a modern replacement tracker. FFmpeg performs external video decoding/encoding; the computational target accepts grayscale PGM frames and writes annotated PPM frames/CSV/JSON. No GPU is needed.

Modernization changes include public headers and serial invocation of the original custom LK loop, borrowed-buffer/array/image ownership, endpoint/warp bounds, empty/even median handling, backward-error filtering, stride-aware patch correlation and chosen-candidate reinitialization. The original classifier ignored all negative patches; the updated one performs the already-implemented negative fern counts. This deliberately changes classifier behavior, including initialization. Fixed-seed random features are repeatable within an environment, not bit-identical across systems. One live TLD instance per process; only no-blur paths validated.

## Evidence and commands

See updated/BUILD.md, updated/VALIDATION.json, updated/VALIDATION.md, updated/SOURCE_PROVENANCE.json and updated/results/. Main commands from the root:

- cmake -S updated -B build/predator -DCMAKE_BUILD_TYPE=RelWithDebInfo
- cmake --build build/predator -j2
- ctest --test-dir build/predator --output-on-failure
- python3 updated/tools/run_demo.py --build-dir build/predator --synthetic --learning 1 --output /path/to/new-directory

Linux Ubuntu 22.04/GCC 11.4/OpenCV 4.5.4 and macOS 15.1.1/Apple clang 16/OpenCV 4.12 builds and component checks passed. Linux complete 120-frame synthetic run passed ASan/UBSan/leak checks. The selected controlled Linux trajectory reached visible mean IoU 0.946363 with online updates (105/105 visible frames IoU >= 0.5), versus 0.874728 without (97/105). Frames 50–64 were hidden/reported lost; detector reinitialized at first reappearance 65. Counts: 664 positive / 848 negative online updates versus 0/0. This is one regression fixture, not a benchmark/general learning benefit. Mac on/off geometry was identical (mean IoU 0.934852, 105/105); results are recorded separately.

A 90-frame real DAVIS camel clip also ran on Linux/Mac. Linux reported four lost frames, Mac zero; both reported four detector reinitializations. Its boxes/path are qualitative; DAVIS masks were not used for accuracy/reacquisition evaluation. Recorded assets are derived from actual Linux output, encoded on Mac, and carry separate CC BY-NC 4.0 attribution documented by DAVIS and the mirror. Generated scene assets are explicitly synthetic. Main README/historical image unchanged.

Unchanged original component negative controls demonstrate ignored negative updates (posterior 1 instead of 0.5), borrowed-frame double-free and an endpoint out-of-bounds (heap-buffer-overflow) read. Updated counterpart checks pass. See check_original_defects.py; builds failing are never accepted as defect evidence.

## Updated native frontends (same recovered core)

updated/platforms/common/PredatorSession.cpp calls recovered Tld directly and computes all displayed boxes/trails from generated or unannotated raw recorded frames. Source core files were not changed for these UIs. UIKit/Objective-C++ iOS and Java/JNI Android frontends provide input, run/pause/step/restart, online-update and initial-selection controls. Final frozen apps completed 90 recorded camel frames with online updates on: physical iPhone 12/iOS 18.0.1, executable SHA256 37669ec4cbb4e70a19af01fca6af459a2c0b39e2b119bcc3cca03dcba72daeaf; physical Pixel 2/Android 10, APK SHA256 6f6179c7a179c95a218ef9fe24549fb9a0f1aff7fb4844050647c5de0d826931. iOS reported 4 reinitializations/0 lost and 2,653 positive/8,744 negative online updates; Android reported 6/0 and 2,369/8,109. Recorded IoUs are null/unmeasured. Exact artifacts, Android trace/derived summary, iOS completion summary and actual display-capture provenance live under updated/validation/native-platforms. No camera capture, mobile synthetic-occlusion validation, general gesture/lifecycle validation or historical UI restoration is claimed.

Mac AppKit and Linux Tk/ctypes UIs actually ran 120 generated frames and 90 raw camel frames. Mac generated mean visible IoU 0.934852, Linux 0.946363; both 105/105 visible frames >=0.5, 15 lost/hidden and two reinitializations. Real Mac/Linux runs had four reinitializations each, zero/four lost reports respectively; accuracy unmeasured. App-window screenshots were sampled at 5 fps and encoded into GIF/MP4 assets; these rates are not processing FPS. Linux used isolated Xvfb :97 rather than a physical monitor. See updated/platforms/README.md and DESKTOP_VALIDATION.json. Base CTest now runs two programs (component checks plus actual Session on/off/recorded regression); older VALIDATION.json retains the earlier one-program record.

Native build products stay under the private task root (ios-build, ios-final-build, dist/PredatorUpdated-final-37669ec4cbb4.app, android/dist/predator-updated-final.apk, build/PredatorDesktop.app). Do not commit signing values/device identifiers. Shared raw input updated/platforms/data/camel-320x180.gray has 5,184,000 bytes, SHA256 975588cf00538d8391dbd67f905c16becc4fc5abb44aa81c2bbfbce43286f379; source footage terms remain separate. updated/README.md compares the actual iOS screen recording with earlier Linux headless output and links all four native-platform movies. Mobile videos are excerpts, with completion established separately by runtime records. The iOS excerpt starts 3 seconds into the raw capture, after the preceding app/transition; sampled first/middle/end MP4 and GIF frames and the PNG were visually checked for the correct Predator interface. A final tools-disabled Claude mobile-documentation review completed; its useful evidence/scope qualifications were applied.

## Storage and dependencies

Native Mac build/logs: /Users/aaron/.codex/project-revivals-2026-11-14/predator. Native Linux build/logs: /home/aaron/project-revivals-20261114/predator on SSH aaron. Linux reuses a read-only private OpenCV/TBB2 extraction at /home/aaron/project-revivals-20261114/freehand/deps/usr; set LD_LIBRARY_PATH to its lib/x86_64-linux-gnu and PREDATOR_OPENCV_PREFIX at configure time. No Predator apt/brew mutations were made.

Bulk recordings/frames: /Volumes/AgentSSD/agent-work/project-revivals-2026-11-14/predator. SSD volume UUID 3233266B-11A5-3130-ADF0-972393997381 was checked mounted/writable/free before use; builds/repos remain on native filesystems. Recheck before future bulk writes.

## Maintenance

Before commits, review/synchronize this file and SKILL.md. Keep source pin/main historical README intact unless the user authorizes otherwise. Record environments, actual measured scope, skips and limitations. Do not claim device compatibility, complete reference-TLD equivalence or reproduced historical FPS from headless tests. Keep demo GIF < 2,000,000 bytes and MP4 < 10,000,000 bytes, plus factual PNG/CSV/JSON evidence. Preserve original/derived source notices and separate recording terms. Two earlier tools-disabled Claude editorial reviews completed. Native-adapter reviews prompted atomic input loading, honest preview/learning traces, worker serialization and reset/autoplay fixes. Final normal and ASan/UBSan CTest runs passed 2/2 on Mac and Linux. A final focused tools-disabled review found no material issue in the corrected Tk reset/autoplay path; its actual missing-input startup control changed from 16 incorrectly started synthetic frames to zero. Android's explicit force-stop launch is the validation workflow; rapid Back/finish followed by relaunch before old Activity destruction is unvalidated. Private exact prompts/results are stored with native Mac run files. Review is separate from runtime evidence.

The unannotated `.gray` fixture is marked binary for Git diffs. Preserve its recorded byte hash and the bundled third-party license files verbatim; their upstream formatting is outside maintainer prose/whitespace checks.
