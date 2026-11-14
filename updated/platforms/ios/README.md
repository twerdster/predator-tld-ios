# Updated UIKit frontend

This app runs the recovered `Tld` pipeline through the shared Session adapter for every processed frame. It is an updated Objective-C++ UIKit frontend, separate from the preserved iOS 6 application. It accepts generated frames and bundled unannotated recorded camel input, not camera capture.

## Build

Requirements: Xcode with an iPhoneOS SDK, CMake 3.20+, and the official OpenCV 4 iOS `opencv2.framework`. The tested build used Xcode 16.1/iPhoneOS 18.1 and official OpenCV 4.12.0. The historical framework in `source/` contains only armv7/armv7s/i386 and cannot serve as the modern arm64 dependency.

```sh
cmake -S updated/platforms/ios -B /native/path/predator-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DOPENCV_IOS_FRAMEWORK_DIR=/path/containing/opencv2.framework
cmake --build /native/path/predator-ios --config Release --target PredatorUpdated \
  -- CODE_SIGNING_ALLOWED=NO
```

For a signed device build, set `PREDATOR_DEVELOPMENT_TEAM` privately when configuring and omit `CODE_SIGNING_ALLOWED=NO`. A configurable `PREDATOR_BUNDLE_IDENTIFIER` defaults to `org.awetzler.predator.updated`. Never put signing identities, team values or device identifiers into repository files. Dependencies and generated build products stay outside the checkout. [VALIDATION.json](VALIDATION.json) records the signed arm64 build; [physical-device records](../../validation/native-platforms/) identify the same frozen executable and its actual run.

## Physical run

The signed app ran all 90 recorded camel frames on a physical iPhone 12 with iOS 18.0.1, launched with `--autoplay --camel`. Online updates stayed on; the completion summary reports four detector reinitializations, zero lost-frame reports, 2,653 positive and 8,744 negative online updates. Recorded-input IoU is null; these decisions and updates do not measure tracking accuracy. The executable SHA-256 is `37669ec4cbb4e70a19af01fca6af459a2c0b39e2b119bcc3cca03dcba72daeaf`.

[Actual screen recording (MP4)](../../assets/ios-native.mp4) · [GIF](../../assets/ios-native.gif) · [PNG](../../assets/ios-native.png) · [completion summary](../../validation/native-platforms/ios-result.json) · [capture provenance](../../validation/native-platforms/ios-capture.json). The USB AVFoundation display recording was trimmed, system bars cropped and audio removed; retained segments keep capture timing. This result covers recorded-input autoplay on the named device. Mobile synthetic occlusion, manual controls and general gesture/lifecycle behavior are not established; the app has no camera input.

## Use and traces

Select Synthetic or Recorded camel. Run/Pause, Step and Restart operate the real native pipeline. Drag an initial box while the preview is ready, before processing starts; selections must be at least 15×15 and within the resized image. Online updates can be toggled; initialization always trains. One serial worker owns the recovered global-state implementation.

Launch arguments support repeatable launches:

- `--autoplay`: start the selected fixture automatically.
- `--camel`: select 90 recorded raw frames; the default is the 120-frame synthetic occlusion fixture.
- `--learning-off`: disable subsequent online updates.

The preview has index −1 and has not initialized the tracker. The learning switch applies to subsequent processed frames. Green boxes are synthetic truth, red estimates and yellow the estimated-center path. Later truth is only for scoring and drawing. Recorded input has no evaluated ground truth; IoU is unmeasured.

The app flushes `Documents/native-run.csv` each frame, including its actual learning flag, and writes `Documents/native-metrics.json` after the final frame, also logging `PREDATOR_NATIVE_COMPLETE`. The summary records learning mode as on, off or mixed. A partial run/trace is not completion. Files are available in the app data container with file sharing enabled. Confidence/correlation, per-frame update counts and native processing time are visible; this time excludes UIKit rendering and is not camera/video FPS.

Original students Marian Mansour and Majd Srour, Kalal TLD GPL notices and the historical supervisor/UI records remain. The recorded input retains separate DAVIS/emirkisa CC BY-NC 4.0 attribution; see [`../../DATA_PROVENANCE.json`](../../DATA_PROVENANCE.json). The modern frontend does not establish full reference-TLD equivalence, historical UI restoration, camera operation or general tracking accuracy.
