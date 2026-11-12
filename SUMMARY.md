# predator-tld-ios: maintenance context

Read this file first, then SKILL.md and README.md. This context describes the repository as of 2026-11-12.

## Purpose and current source

Historical iOS object tracking/detection with conditional classifier updates, derived from Kalal TLD. Report and upstream README spell student Marian Mansour; report credits supervisors Aaron Wetzler/Yaron Honen, while upstream README credits Aaron Wetzler/Ron Kimmel and Anton Anton for UI/graphics.

source/ pins kenguish/Predator-iOS-Technion at a63f005bf34b041ff0dd985a8e18083f2d3065eb unchanged. Tracker.cpp uses forward/backward LK; TldDetector scans candidate windows; Tld.mm combines confidence/correlation and calls classifier->train under learning flag. UI exposes learning controls.

## Use and validation

Clone recursively and open source/Predator.xcodeproj for inspection. Historical target iOS 6.0, bundled OpenCV 2, Apple camera/graphics frameworks; own signing required.

Source/pin/project inspection only; no compilation, device runtime, tracking/learning benchmark or reproduced FPS. Earlier handoff said online learning omitted, but pinned later revision contains conditional training calls. The learning path is implemented in source but uncompiled/unexercised here; it is not runtime validation or full reference-TLD equivalence.

Old architectures, APIs and dependencies may need porting. No updated port/notebook. Several TLD source files carry GPL notices; preserve all source/upstream/third-party terms. Historical screenshot FPS is not a measured maintenance benchmark.

## Documentation maintenance

The 2026-11-12 documentation revision organizes the README around implementation, available source, setup, recorded checks and limits. It preserves historical visuals and links, with captions separating historical results from updated examples. No runnable code, source provenance, pinned revisions, licenses or validation results changed in this revision. Recorded earlier tests were not rerun for the prose-only edits.

Preserve original source and authorship. New ports belong under updated/ and require their own scope/validation. Do not infer a blanket license from recovery. Review this file and SKILL.md before every commit and synchronize changes to workflow, commands, validation or status. Check relative links and figure rendering; never describe inventory counts or component tests as full-application validation.

Two actual Claude editorial review rounds on 2026-11-12 covered the README and related changed documentation, including context files, component guides and applicable algorithm/build notes or schematic text. Useful qualifications were checked against source, preserved project records and saved validation logs, then applied. Editorial review adds no runtime coverage.
