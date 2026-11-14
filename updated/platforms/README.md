# Updated native frontends

These frontends invoke the recovered C++ tracking, detector, fern and learning pipeline for every processed frame. They share [`common/PredatorSession.cpp`](common/PredatorSession.cpp), which calls `Tld` directly. They do not restore the historical camera/UI application. Input is either the controlled generated scene or bundled **unannotated** grayscale DAVIS camel frames; estimates and the center trail are computed during the run.

| Platform | Frontend | Current evidence |
| --- | --- | --- |
| Android | Java controls, worker thread, JNI, recovered C++ | Final APK ran all 90 recorded frames on a physical Pixel 2 / Android 10; per-frame trace and screen recording retained |
| iOS | UIKit Objective-C++, serial worker, recovered C++ | Final signed executable ran all 90 recorded frames on a physical iPhone 12 / iOS 18.0.1; completion summary and screen recording retained |
| macOS | AppKit, serial worker, recovered C++ | Actual generated and recorded runs completed on macOS 15.1.1 arm64; app-window screenshots encoded as videos |
| Linux | Tk 8.6, ctypes C ABI, recovered C++ shared library | Actual generated and recorded runs completed on Ubuntu 22.04 x86_64 in isolated Xvfb; screenshots encoded as videos |

Device availability, build success and recorded runtime are separate evidence. No frontend here has camera capture. Android declares API 26 and two ABIs; the physical Android 10 result does not establish every supported API/ABI combination. The iOS target declares iOS 15; the physical result covers the named iPhone and iOS 18.0.1.

## Physical-device recordings

| Actual running UI | Recorded camel | Runtime evidence |
| --- | --- | --- |
| iPhone 12 / iOS 18.0.1 | [GIF](../assets/ios-native.gif) · [MP4](../assets/ios-native.mp4) · [PNG](../assets/ios-native.png) | [Executable hash](../validation/native-platforms/ios-artifact.json) · [90-frame summary](../validation/native-platforms/ios-result.json) · [capture record](../validation/native-platforms/ios-capture.json) |
| Pixel 2 / Android 10 | [GIF](../assets/android-native.gif) · [MP4](../assets/android-native.mp4) · [PNG](../assets/android-native.png) | [APK hash](../validation/native-platforms/android-artifact.json) · [90-frame trace](../validation/native-platforms/android-camel-trace.jsonl) · [derived summary](../validation/native-platforms/android-result.json) · [capture record](../validation/native-platforms/android-capture.json) |

iOS was captured from the physical display over USB with AVFoundation; Android used `adb screenrecord`. The published clips are excerpts; completion is established by the linked runtime records. The excerpts retain capture timing, crop system bars and remove audio. Together with artifact hashes and runtime records, they document the native app processing bundled raw pixels rather than playing prior annotated output. These recorded-input device runs use online updates throughout; mobile synthetic occlusion, manual interaction and general gesture/lifecycle behavior are not established.

## Desktop recordings

| Actual running UI | Recorded camel | Generated occlusion fixture |
| --- | --- | --- |
| macOS AppKit | [GIF](../assets/macos-native-camel.gif) · [MP4](../assets/macos-native-camel.mp4) · [PNG](../assets/macos-native-camel.png) | [GIF](../assets/macos-native-synthetic.gif) · [MP4](../assets/macos-native-synthetic.mp4) |
| Linux Tk / native C++ | [GIF](../assets/linux-native-camel.gif) · [MP4](../assets/linux-native-camel.mp4) · [PNG](../assets/linux-native-camel.png) | [GIF](../assets/linux-native-synthetic.gif) · [MP4](../assets/linux-native-synthetic.mp4) |

The Mac captures sample only the actual AppKit window with `screencapture -l`; Linux captures sample the actual Tk window with Pillow/XCB on isolated Xvfb. Both platforms’ screenshots were sampled at 5 fps. Mac FFmpeg encoded both platforms' MP4s at 5 fps and GIF previews at 3 fps. These presentation rates are not tracker performance measurements. Linux used a virtual display, not a physical Linux monitor. [DESKTOP_VALIDATION.json](DESKTOP_VALIDATION.json) and [`../results/`](../results/) retain runtime summaries, traces and capture sampling records.

Both desktop generated runs processed all 120 frames: Mac mean visible IoU 0.934852; Linux 0.946363; each had 105/105 visible frames (including the supplied initialization frame) at IoU ≥ 0.5, 15 hidden/lost frames and two detector reinitializations.

The completed 90-frame recorded runs used online updates throughout:

| Runtime | Lost-frame reports | Detector reinitializations | Positive / negative online updates |
| --- | ---: | ---: | ---: |
| Physical iOS | 0 | 4 | 2,653 / 8,744 |
| Physical Android | 0 | 6 | 2,369 / 8,109 |
| macOS AppKit | 0 | 4 | 2,653 / 8,744 |
| Linux Tk | 4 | 4 | 3,250 / 10,610 |

Update counts sum per-frame online training operations; initialization is separate. Lost and reinitialization counts describe the algorithm's decisions, not correctness against ground truth. Recorded-input IoU is null/unmeasured. These bounded demonstrations establish neither tracking accuracy nor cross-platform numerical parity.

## Build and replay

The base build also provides `predator_session_check` and the `predator_session` C ABI library. CTest contains two programs: the earlier component checks and a Session regression that processes the synthetic scene with updates on and off, plus the raw recorded input. The second checks native RGBA dimensions, hidden-frame rejection, detector recovery, update controls, invalid selection and preservation of a synthetic prior session on recorded-input validation failure. [`../VALIDATION.json`](../VALIDATION.json) predates the Session test and lists one CTest program.

```sh
cmake -S updated -B build/predator -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/predator -j2
ctest --test-dir build/predator --output-on-failure
```

On macOS, run the AppKit bundle executable directly to pass launch arguments:

```sh
build/predator/PredatorDesktop.app/Contents/MacOS/PredatorDesktop \
  --camel --autoplay --output /path/to/new-macos-ui-run
```

Omit `--camel` for generated input; `--learning-off` disables subsequent updates. Controls provide Run/Pause, Step, Restart, input selection and online-update toggle. Per-frame `native-run.csv` (including the learning flag) and completion `native-metrics.json` (including on/off/mixed learning mode) go to `--output`. Recorded-clip results contain lost-frame, reinitialization and online-update counts; `mean_visible_iou:null` means no evaluation ground truth.

Linux additionally needs Python 3 with Tkinter. It calls the local shared C++ library through ctypes:

```sh
python3 updated/platforms/desktop/predator_ui.py \
  --library build/predator/libpredator_session.so --source camel \
  --autoplay --output /path/to/new-linux-ui-run
```

Use `--source synthetic` and `--learning 0` for the other cases. Run inside a desktop session or set `DISPLAY` to an existing isolated X server. The GUI writes per-frame JSON and a completion summary. Per-frame learning flags and on/off/mixed summaries preserve any mid-run toggle changes, including the flag selected for frame 0 even though initialization always trains. The maintainer-local private Tk/OpenCV dependency paths are maintainer-local, not required package locations.

[iOS instructions](ios/README.md) and [Android instructions](android/README.md) cover their platform-specific toolchains and launch arguments.

## Input and attribution

`data/camel-320x180.gray` contains exactly 90 × 320 × 180 bytes of unannotated 8-bit grayscale pixels. It was decoded from the source MP4 with FFmpeg’s `scale=320:180` and `-pix_fmt gray`; no tracking boxes, masks or later ground truth are stored in it. Initial selection is `[105,35,135,85]`; later estimates come from the recovered algorithm. Raw input SHA-256: `975588cf00538d8391dbd67f905c16becc4fc5abb44aa81c2bbfbce43286f379`.

Footage: DAVIS, Pont-Tuset/Perazzi/Caelles/Arbeláez/Sorkine-Hornung/Van Gool; MP4 supplied by emirkisa’s mirror, with separate CC BY-NC 4.0 terms. See [data provenance](../DATA_PROVENANCE.json). Original students Marian Mansour and Majd Srour and Kalal’s GPL notices remain credited. The native frontends are updated work prepared for Aaron Wetzler; original `source/` and the historical root README remain unchanged.
