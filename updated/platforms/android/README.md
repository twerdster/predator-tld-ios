# Updated Android frontend

This native Java/JNI application runs the recovered custom forward/backward Lucas–Kanade tracker, scanning detector, fern classifier and conditional online updates from [`../../src`](../../src). OpenCV supplies core/image-processing primitives. It processes generated frames or bundled **unannotated** DAVIS camel frames on the device; it does not play a saved tracking output.

The original iOS app and source archive remain separate. This is an updated Android frontend, not the historical application.

## Build

Use JDK 21, Android SDK API 34/build-tools 34.0.0, NDK 27.2.12479018, CMake 3.22.1 and the official OpenCV Android SDK 4.12.0. The wrapper pins Gradle 8.7 with its distribution checksum; the Android plugin is 8.3.0. The application declares API 26 minimum and builds `arm64-v8a` and `armeabi-v7a`. Declared API/ABI support is distinct from device validation.

```sh
export JAVA_HOME=/path/to/jdk-21
export ANDROID_HOME=/path/to/android-sdk
export OPENCV_ANDROID_SDK=/path/to/OpenCV-android-sdk
./gradlew --no-daemon --max-workers=2 assembleDebug lintDebug
```

The APK is normally `app/build/outputs/apk/debug/app-debug.apk`. To keep generated files outside the checkout, pass `-PpredatorBuildDir=/native/path/build`, `-PpredatorNativeDir=/native/path/cxx` and `--project-cache-dir /native/path/project-cache`. Use a task-specific `GRADLE_USER_HOME` for private Gradle caches. No global installs are needed when these dependencies are already available. The debug build uses local debug signing; no signing credentials are included here.

## Use

Select **Synthetic** (120 frames, target fully hidden at frames 50–64) or **Recorded** (90 DAVIS camel frames), then Play, Pause, Step or Restart. Drag a box in the preview to change the initial region; it must be at least 15×15 resized-image pixels. Restart restores the default selection. Online updates can be toggled during a run; initialization always trains. A process-wide FIFO worker serializes native calls and queued cleanup. Device validation uses the explicit force-stop launch below; rapid Back/finish followed by relaunch before old Activity destruction is not validated. The main thread applies rendered frames and labels; closed or superseded Activity jobs/results are discarded.

The preview has index −1 and has not run the tracker. The online-update switch applies to subsequently processed frames, and every processed frame and trace records its actual value.

Red marks estimates, yellow the estimated-center path, and green synthetic truth. Subsequent synthetic truth is used only for scoring and overlays. IoU is unavailable on hidden frames and on the recorded clip. Correlation, confidence, processing time, reinitializations, losses and per-frame/cumulative online update counts are displayed. Processing time covers native image generation/loading and tracking, excluding rendering/JNI/UI; it is not a video FPS measurement.

For an explicit, repeatable launch after installation:

```sh
adb shell am force-stop org.awetzler.predator.updated
adb shell am start -n org.awetzler.predator.updated/.MainActivity \
  --es source synthetic --ez learning true --ez autoRun true
```

Use `source camel` or `learning false` for the other cases. Each reset starts an internal `files/runs/{synthetic|camel}-learning-{on|off}.jsonl` trace; the name identifies the **initial** toggle, while every frame records its actual learning flag. A completed run ends with the final frame's `done:true`; a partial trace is not completion evidence. The same frame records appear under logcat tag `PredatorRuntime`. A debug build permits `adb exec-out run-as org.awetzler.predator.updated cat files/runs/synthetic-learning-on.jsonl`.

## Checks and limits

See [`VALIDATION.json`](VALIDATION.json) for build/package checks. The final APK (SHA-256 `6f6179c7a179c95a218ef9fe24549fb9a0f1aff7fb4844050647c5de0d826931`) also ran all 90 recorded camel frames on a physical Pixel 2 with Android 10, using the force-stop launch above with `source camel`. Its contiguous per-frame trace ends at frame 89 with `done:true`, online updates enabled throughout, six detector reinitializations, zero lost-frame reports, 2,369 positive and 8,109 negative online updates. All recorded-input IoUs are null; these counts do not measure tracking accuracy.

[Actual screen recording (MP4)](../../assets/android-native.mp4) · [GIF](../../assets/android-native.gif) · [PNG](../../assets/android-native.png) · [APK record](../../validation/native-platforms/android-artifact.json) · [trace](../../validation/native-platforms/android-camel-trace.jsonl) · [derived summary](../../validation/native-platforms/android-result.json). Compile targets beyond this device, mobile synthetic occlusion, manual controls and rapid Activity recreation remain unvalidated. This frontend has no camera input. Only the recovered no-blur, single-session path is used, with the behavioral changes described in [`../../BUILD.md`](../../BUILD.md#changes-from-the-recovered-code).

Original students: Marian Mansour and Majd Srour, based on Zdeněk Kalal’s TLD; GPL notices remain. The Gradle wrapper's Apache license and notice are preserved. OpenCV/dependency notices, recovered-source GPL text and footage attribution are included in the APK and accessible through **Credits, scope and licenses**.

Recorded input: [DAVIS](https://davischallenge.org/), Pont-Tuset, Perazzi, Caelles, Arbeláez, Sorkine-Hornung and Van Gool; MP4 from [emirkisa’s mirror](https://huggingface.co/datasets/emirkisa/DAVIS-2017-480p-mp4). DAVIS metadata and the mirror card name **CC BY-NC 4.0**. The first 90 frames were decoded directly to grayscale 320×180 pixels without tracking annotations. Recording terms remain separate from code licensing; see [`../../DATA_PROVENANCE.json`](../../DATA_PROVENANCE.json).
