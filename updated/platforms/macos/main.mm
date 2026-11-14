// AppKit UI calls the recovered native TLD Session for each displayed frame.
#import <Cocoa/Cocoa.h>
#include "PredatorSession.h"
#include <fstream>
@interface PredatorMac : NSObject <NSApplicationDelegate> {
  NSWindow *_window;
  NSImageView *_image;
  NSTextField *_status, *_detail;
  NSPopUpButton *_source;
  NSButton *_learning, *_run;
  NSTimer *_timer;
  dispatch_queue_t _worker;
  std::unique_ptr<predator::Session> _session;
  BOOL _running, _busy, _autoPlay, _sawOn, _sawOff, _inputSynthetic;
  NSInteger _generation;
  NSString *_output;
  std::ofstream _csv;
  double _sumIou;
  int _visible, _successes, _lost, _reinit, _count;
  unsigned long _positive, _negative;
}
@end
@implementation PredatorMac
- (NSTextField *)label:(NSString *)text frame:(NSRect)frame size:(double)size {
  NSTextField *label = [NSTextField wrappingLabelWithString:text];
  label.frame = frame;
  label.font = [NSFont systemFontOfSize:size];
  [_window.contentView addSubview:label];
  return label;
}
- (NSButton *)button:(NSString *)text frame:(NSRect)frame action:(SEL)action {
  NSButton *button = [NSButton buttonWithTitle:text target:self action:action];
  button.frame = frame;
  [_window.contentView addSubview:button];
  return button;
}
- (void)applicationDidFinishLaunching:(NSNotification *)note {
  NSArray *args = NSProcessInfo.processInfo.arguments;
  NSUInteger outIndex = [args indexOfObject:@"--output"];
  _output =
      outIndex != NSNotFound && outIndex + 1 < args.count
          ? args[outIndex + 1]
          : [NSHomeDirectory()
                stringByAppendingPathComponent:@"Library/Application Support/PredatorUpdated"];
  [NSFileManager.defaultManager createDirectoryAtPath:_output
                          withIntermediateDirectories:YES
                                           attributes:nil
                                                error:nil];
  _autoPlay = [args containsObject:@"--autoplay"];
  _worker = dispatch_queue_create("org.predator.macos.recovered", DISPATCH_QUEUE_SERIAL);
  _session = std::make_unique<predator::Session>();
  _window =
      [[NSWindow alloc] initWithContentRect:NSMakeRect(30, 100, 1100, 700)
                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                            NSWindowStyleMaskMiniaturizable
                                    backing:NSBackingStoreBuffered
                                      defer:NO];
  _window.title = @"Predator · updated native TLD on macOS";
  _window.releasedWhenClosed = NO;
  NSTextField *title = [self label:@"Predator · updated native"
                             frame:NSMakeRect(20, 650, 1060, 34)
                              size:25];
  title.font = [NSFont boldSystemFontOfSize:25];
  [self label:@"Recovered custom TLD · frames computed here by C++ / OpenCV core + imgproc"
        frame:NSMakeRect(20, 620, 1060, 24)
         size:15];
  _image = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 115, 640, 480)];
  _image.imageScaling = NSImageScaleProportionallyUpOrDown;
  [_window.contentView addSubview:_image];
  _status = [self label:@"Preparing native input…" frame:NSMakeRect(685, 500, 390, 90) size:16];
  _detail = [self label:@"" frame:NSMakeRect(685, 340, 390, 150) size:13];
  _detail.font = [NSFont monospacedSystemFontOfSize:13 weight:NSFontWeightRegular];
  _source = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(685, 295, 220, 30) pullsDown:NO];
  [_source addItemsWithTitles:@[ @"Synthetic fixture", @"Recorded DAVIS camel" ]];
  [_source selectItemAtIndex:[args containsObject:@"--camel"] ? 1 : 0];
  _source.target = self;
  _source.action = @selector(reset);
  [_window.contentView addSubview:_source];
  _learning = [NSButton checkboxWithTitle:@"Online updates (initialization always trains)"
                                   target:nil
                                   action:nil];
  _learning.frame = NSMakeRect(685, 255, 390, 26);
  _learning.state =
      [args containsObject:@"--learning-off"] ? NSControlStateValueOff : NSControlStateValueOn;
  [_window.contentView addSubview:_learning];
  _run = [self button:@"Run" frame:NSMakeRect(685, 205, 110, 34) action:@selector(toggle)];
  [self button:@"Step" frame:NSMakeRect(805, 205, 110, 34) action:@selector(singleStep)];
  [self button:@"Restart" frame:NSMakeRect(925, 205, 130, 34) action:@selector(reset)];
  [self label:@"Green: synthetic truth · red: estimate\nYellow: estimated-center path\nRecorded "
              @"input has no evaluation annotations."
        frame:NSMakeRect(685, 100, 390, 85)
         size:13];
  [self label:@"Marian Mansour / Majd Srour · Kalal TLD (GPL)\nDAVIS / emirkisa mirror · CC BY-NC "
              @"4.0 · Updated frontend for Aaron Wetzler\nThis app computes raw input frames; it "
              @"does not play a saved tracking movie."
        frame:NSMakeRect(20, 20, 1060, 75)
         size:12];
  [_window makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
  _timer = [NSTimer scheduledTimerWithTimeInterval:.1
                                            target:self
                                          selector:@selector(tick)
                                          userInfo:nil
                                           repeats:YES];
  [self reset];
}
- (void)reset {
  _running = NO;
  _run.title = @"Run";
  NSInteger generation = ++_generation;
  BOOL synthetic = _source.indexOfSelectedItem == 0;
  NSString *path = [NSBundle.mainBundle pathForResource:@"camel-320x180" ofType:@"gray"];
  dispatch_async(_worker, ^{
    try {
      self->_session->reset(synthetic, path ? path.UTF8String : "");
      self->_inputSynthetic = synthetic;
      self->_sawOn = NO;
      self->_sawOff = NO;
      self->_sumIou = 0;
      self->_visible = 0;
      self->_successes = 0;
      self->_lost = 0;
      self->_reinit = 0;
      self->_count = 0;
      self->_positive = 0;
      self->_negative = 0;
      [[NSFileManager defaultManager]
          removeItemAtPath:[self->_output stringByAppendingPathComponent:@"native-metrics.json"]
                     error:nil];
      self->_csv.close();
      self->_csv.open(
          [[self->_output stringByAppendingPathComponent:@"native-run.csv"] UTF8String]);
      self->_csv << "frame,learning_enabled,valid,x,y,w,h,iou,reinitialized,positive_updates,"
                    "negative_updates,processing_ms\n";
      predator::Frame frame = self->_session->frame();
      dispatch_async(dispatch_get_main_queue(), ^{
        if (generation != self->_generation)
          return;
        [self display:frame];
        if (self->_autoPlay) {
          self->_autoPlay = NO;
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC),
                         dispatch_get_main_queue(), ^{
                           if (generation == self->_generation && !self->_running)
                             [self toggle];
                         });
        }
      });
    } catch (const std::exception &e) {
      NSString *message = [NSString stringWithUTF8String:e.what()];
      dispatch_async(dispatch_get_main_queue(), ^{
        self->_status.stringValue = message;
      });
    }
  });
}
- (void)toggle {
  _running = !_running;
  _run.title = _running ? @"Pause" : @"Run";
}
- (void)tick {
  if (_running)
    [self processNext];
}
- (void)singleStep {
  _running = NO;
  _run.title = @"Run";
  [self processNext];
}
- (void)processNext {
  if (_busy)
    return;
  _busy = YES;
  BOOL learning = _learning.state == NSControlStateValueOn;
  NSInteger generation = _generation;
  dispatch_async(_worker, ^{
    try {
      BOOL advanced = self->_session->step(learning);
      predator::Frame frame = self->_session->frame();
      if (advanced) {
        if (learning)
          self->_sawOn = YES;
        else
          self->_sawOff = YES;
        self->_count++;
        if (frame.visible) {
          self->_sumIou += frame.iou;
          self->_visible++;
          self->_successes += frame.iou >= .5;
        }
        self->_lost += !frame.valid;
        self->_reinit += frame.reinitialized;
        self->_positive += frame.positiveUpdates;
        self->_negative += frame.negativeUpdates;
        self->_csv << frame.index << ',' << learning << ',' << frame.valid << ',' << frame.box[0]
                   << ',' << frame.box[1] << ',' << frame.box[2] << ',' << frame.box[3] << ','
                   << frame.iou << ',' << frame.reinitialized << ',' << frame.positiveUpdates << ','
                   << frame.negativeUpdates << ',' << frame.processingMs << '\n';
        self->_csv.flush();
        if (frame.done) {
          NSDictionary *metrics = @{
            @"platform" : @"macOS AppKit",
            @"learning_mode" : self->_sawOn && self->_sawOff ? @"mixed"
            : self->_sawOn                                   ? @"on"
                                                             : @"off",
            @"input" : [NSString stringWithUTF8String:frame.inputLabel.c_str()],
            @"frames" : @(self->_count),
            @"visible_frames" : self->_inputSynthetic ? @(self->_visible) : NSNull.null,
            @"visible_iou_at_least_half" : self->_inputSynthetic ? @(self->_successes)
                                                                 : NSNull.null,
            @"mean_visible_iou" : self->_visible ? @(self->_sumIou / self->_visible) : NSNull.null,
            @"lost_frames" : @(self->_lost),
            @"reinitializations" : @(self->_reinit),
            @"positive_updates" : @(self->_positive),
            @"negative_updates" : @(self->_negative)
          };
          [[NSJSONSerialization dataWithJSONObject:metrics
                                           options:NSJSONWritingPrettyPrinted
                                             error:nil]
              writeToFile:[self->_output stringByAppendingPathComponent:@"native-metrics.json"]
               atomically:YES];
          NSLog(@"PREDATOR_NATIVE_COMPLETE %@", metrics);
        }
      }
      dispatch_async(dispatch_get_main_queue(), ^{
        self->_busy = NO;
        if (generation != self->_generation)
          return;
        [self display:frame];
        if (frame.done) {
          self->_running = NO;
          self->_run.title = @"Done";
        }
      });
    } catch (const std::exception &e) {
      NSString *message = [NSString stringWithUTF8String:e.what()];
      dispatch_async(dispatch_get_main_queue(), ^{
        self->_busy = NO;
        self->_running = NO;
        self->_status.stringValue = message;
      });
    }
  });
}
- (void)display:(const predator::Frame &)frame {
  NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                  pixelsWide:frame.width
                                                                  pixelsHigh:frame.height
                                                               bitsPerSample:8
                                                             samplesPerPixel:4
                                                                    hasAlpha:YES
                                                                    isPlanar:NO
                                                              colorSpaceName:NSDeviceRGBColorSpace
                                                                bitmapFormat:0
                                                                 bytesPerRow:frame.width * 4
                                                                bitsPerPixel:32];
  memcpy(rep.bitmapData, frame.rgba.data(), frame.rgba.size());
  NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(frame.width, frame.height)];
  [image addRepresentation:rep];
  _image.image = image;
  _status.stringValue =
      [NSString stringWithFormat:@"%@\nFrame %d/%d · %@",
                                 [NSString stringWithUTF8String:frame.inputLabel.c_str()],
                                 frame.index + 1, frame.total,
                                 frame.done        ? @"DONE"
                                 : frame.index < 0 ? @"Ready"
                                 : frame.valid     ? @"Native estimate"
                                                   : @"Target lost"];
  _detail.stringValue =
      [NSString stringWithFormat:@"box %.1f,%.1f %.1f×%.1f\nconfidence %.3f\ncorrelation %.3f\nIoU "
                                 @"%@\ndetector reinit %@\nonline +%lu / −%lu\ncompute %.1f ms",
                                 frame.box[0], frame.box[1], frame.box[2], frame.box[3],
                                 frame.confidence, frame.correlation,
                                 frame.iou >= 0 ? [NSString stringWithFormat:@"%.3f", frame.iou]
                                                : @"unmeasured",
                                 frame.reinitialized ? @"yes" : @"no", frame.positiveUpdates,
                                 frame.negativeUpdates, frame.processingMs];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return YES;
}
@end
int main(int argc, char **argv) {
  @autoreleasepool {
    NSApplication *app = NSApplication.sharedApplication;
    app.activationPolicy = NSApplicationActivationPolicyRegular;
    __attribute__((objc_precise_lifetime)) PredatorMac *delegate = [PredatorMac new];
    app.delegate = delegate;
    [app run];
    return 0;
  }
}
