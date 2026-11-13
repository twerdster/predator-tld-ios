# predator-tld-ios: maintenance context

Read this file first, then SKILL.md. Current restoration record: 2026-11-14. The main README is deliberately retained as the historical project reference; use updated/README.md for the runnable maintained target.

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

## Storage and dependencies

Native Mac build/logs: /Users/aaron/.codex/project-revivals-2026-11-14/predator. Native Linux build/logs: /home/aaron/project-revivals-20261114/predator on SSH aaron. Linux reuses a read-only private OpenCV/TBB2 extraction at /home/aaron/project-revivals-20261114/freehand/deps/usr; set LD_LIBRARY_PATH to its lib/x86_64-linux-gnu and PREDATOR_OPENCV_PREFIX at configure time. No Predator apt/brew mutations were made.

Bulk recordings/frames: /Volumes/AgentSSD/agent-work/project-revivals-2026-11-14/predator. SSD volume UUID 3233266B-11A5-3130-ADF0-972393997381 was checked mounted/writable/free before use; builds/repos remain on native filesystems. Recheck before future bulk writes.

## Maintenance

Before commits, review/synchronize this file and SKILL.md. Keep source pin/main historical README intact unless the user authorizes otherwise. Record environments, actual measured scope, skips and limitations. Do not claim device compatibility, complete reference-TLD equivalence or reproduced historical FPS from headless tests. Keep demo GIF < 2,000,000 bytes and MP4 < 10,000,000 bytes, plus factual PNG/CSV/JSON evidence. Preserve original/derived source notices and separate recording terms. Two actual tools-disabled Claude editorial reviews completed; round two found no material issues and minor qualifications were applied. Private review logs are stored with native Mac run files. Editorial review is separate from runtime evidence.
