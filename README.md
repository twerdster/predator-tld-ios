# Predator TLD on iOS

An implementation of Zdenek Kalal's Tracking-Learning-Detection algorithm for near-real-time tracking on an iPad.

Students: Mariaan Mansour, Majd Srour.

Supervision: Aaron Wetzler.

## Status

The historical Technion implementation, preserved by Kenneth Anguish with iOS compatibility fixes, is included as the `source/` Git submodule from [kenguish/Predator-iOS-Technion](https://github.com/kenguish/Predator-iOS-Technion), pinned to `a63f005bf34b041ff0dd985a8e18083f2d3065eb`. Original student attribution remains intact. This is not a tested current iOS release.

## Running

```sh
git clone --recurse-submodules https://github.com/twerdster/predator-tld-ios.git
cd predator-tld-ios
open source/Predator.xcodeproj
```

Use Xcode to inspect the historical `Predator` target, its Objective-C/C++ sources, bundled OpenCV 2 framework and Apple camera/graphics frameworks. The preserved revision has iOS 6 compatibility fixes and an app deployment target of iOS 6.0. Use your own signing configuration. Old OpenCV binaries, architectures and removed iOS/C++ APIs may require replacement or porting before a modern build is possible.

The project implements tracking/detection from Kalal's TLD work; contemporaneous handoff notes state that online learning was omitted, so do not infer full TLD behavior from the general algorithm description in the upstream README. No compilation, device runtime or performance validation has been performed during recovery.

## Links

- [Project entry on Aaron Wetzler's website](https://twerdster.github.io/#predator-tld-on-ios)
- [Recovered original project report](docs/report.pdf)
- [Preserved original source and attribution](https://github.com/kenguish/Predator-iOS-Technion)

## Attribution and Updates

Original student and third-party attribution will be retained. No license for unrecovered source code is implied by this README. Original authors can open an issue to contribute updates or request corrections or removal of their material.

