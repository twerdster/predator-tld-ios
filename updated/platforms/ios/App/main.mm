// Updated UIKit frontend. Frames are processed by the recovered native TLD core.
#import <UIKit/UIKit.h>
#include "PredatorSession.h"
#include <sstream>
#include <fstream>
using predator::Frame;
@interface PredatorController : UIViewController {
  std::unique_ptr<predator::Session> _session;
  dispatch_queue_t _worker;
  UIImageView *_image;
  UILabel *_status, *_detail, *_credit;
  UISegmentedControl *_input;
  UISwitch *_learning;
  UIButton *_play;
  NSTimer *_timer;
  BOOL _running, _busy, _autoPlay, _sawOn, _sawOff, _inputSynthetic;
  NSInteger _generation;
  int _displayWidth, _displayHeight, _lastIndex;
  CGPoint _selectionStart;
  CAShapeLayer *_selection;
  std::ofstream _csv;
  double _sumIou;
  int _visible, _successes, _lost, _reinitializations;
  unsigned long _positive, _negative;
}
@end
@implementation PredatorController
- (UIButton *)button:(NSString *)title action:(SEL)action {
  UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
  [button setTitle:title forState:UIControlStateNormal];
  [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
  button.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
  return button;
}
- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = UIColor.systemBackgroundColor;
  _worker = dispatch_queue_create("org.predator.recovered-tld", DISPATCH_QUEUE_SERIAL);
  _session = std::make_unique<predator::Session>();
  NSArray *args = NSProcessInfo.processInfo.arguments;
  _autoPlay = [args containsObject:@"--autoplay"];
  UILabel *title = [UILabel new];
  title.text = @"Predator · updated native";
  title.font = [UIFont systemFontOfSize:24 weight:UIFontWeightBold];
  UILabel *subtitle = [UILabel new];
  subtitle.text = @"Recovered TLD · computed on this device";
  subtitle.font = [UIFont systemFontOfSize:14];
  _input = [[UISegmentedControl alloc] initWithItems:@[ @"Synthetic", @"Recorded camel" ]];
  _input.selectedSegmentIndex = [args containsObject:@"--camel"] ? 1 : 0;
  [_input addTarget:self action:@selector(resetRun) forControlEvents:UIControlEventValueChanged];
  _image = [UIImageView new];
  _image.contentMode = UIViewContentModeScaleAspectFit;
  _image.backgroundColor = [UIColor colorWithWhite:.08 alpha:1];
  _image.userInteractionEnabled = YES;
  [_image
      addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self
                                                                   action:@selector(selectBox:)]];
  _selection = [CAShapeLayer layer];
  _selection.strokeColor = UIColor.systemYellowColor.CGColor;
  _selection.fillColor = UIColor.clearColor.CGColor;
  _selection.lineWidth = 2;
  [_image.layer addSublayer:_selection];
  _status = [UILabel new];
  _status.font = [UIFont monospacedSystemFontOfSize:15 weight:UIFontWeightMedium];
  _status.numberOfLines = 3;
  _detail = [UILabel new];
  _detail.font = [UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular];
  _detail.numberOfLines = 5;
  _learning = [UISwitch new];
  _learning.on = ![args containsObject:@"--learning-off"];
  UILabel *learnLabel = [UILabel new];
  learnLabel.text = @"Online updates";
  UIStackView *learn = [[UIStackView alloc] initWithArrangedSubviews:@[ learnLabel, _learning ]];
  learn.spacing = 12;
  _play = [self button:@"Run" action:@selector(toggleRun)];
  UIStackView *buttons = [[UIStackView alloc] initWithArrangedSubviews:@[
    _play, [self button:@"Step" action:@selector(singleStep)],
    [self button:@"Restart" action:@selector(resetRun)]
  ]];
  buttons.distribution = UIStackViewDistributionFillEqually;
  buttons.spacing = 12;
  UILabel *help = [UILabel new];
  help.numberOfLines = 2;
  help.font = [UIFont systemFontOfSize:12];
  help.text = @"Before running, drag an initial box or use the preset. Green: synthetic truth; "
              @"red: estimate; yellow: path.";
  _credit = [UILabel new];
  _credit.numberOfLines = 3;
  _credit.font = [UIFont systemFontOfSize:11];
  _credit.text = @"Marian Mansour / Majd Srour · Kalal TLD (GPL)\nDAVIS camel / emirkisa mirror · "
                 @"CC BY-NC 4.0\nUpdated frontend for Aaron Wetzler; no camera capture.";
  UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[
    title, subtitle, _input, _image, _status, _detail, learn, buttons, help, _credit
  ]];
  stack.axis = UILayoutConstraintAxisVertical;
  stack.spacing = 8;
  stack.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:stack];
  UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    [stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:16],
    [stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-16],
    [stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:10],
    [stack.bottomAnchor constraintLessThanOrEqualToAnchor:safe.bottomAnchor constant:-8],
    [_image.heightAnchor constraintEqualToAnchor:_image.widthAnchor multiplier:.75]
  ]];
  _timer = [NSTimer scheduledTimerWithTimeInterval:.12
                                            target:self
                                          selector:@selector(tick)
                                          userInfo:nil
                                           repeats:YES];
  [self resetRun];
}
- (void)resetRun {
  _running = NO;
  [_play setTitle:@"Run" forState:UIControlStateNormal];
  NSInteger generation = ++_generation;
  BOOL synthetic = _input.selectedSegmentIndex == 0;
  NSString *raw = [NSBundle.mainBundle pathForResource:@"camel-320x180" ofType:@"gray"];
  _status.text = @"Preparing native input…";
  dispatch_async(_worker, ^{
    try {
      self->_session->reset(synthetic, raw ? raw.UTF8String : "");
      self->_inputSynthetic = synthetic;
      self->_sawOn = NO;
      self->_sawOff = NO;
      self->_sumIou = 0;
      self->_visible = 0;
      self->_successes = 0;
      self->_lost = 0;
      self->_reinitializations = 0;
      self->_positive = 0;
      self->_negative = 0;
      NSString *documents =
          NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
              .firstObject;
      [[NSFileManager defaultManager]
          removeItemAtPath:[documents stringByAppendingPathComponent:@"native-metrics.json"]
                     error:nil];
      self->_csv.close();
      self->_csv.open([[documents stringByAppendingPathComponent:@"native-run.csv"] UTF8String]);
      self->_csv << "frame,learning_enabled,valid,x,y,w,h,iou,reinitialized,positive_updates,"
                    "negative_updates,processing_ms\n";
      Frame frame = self->_session->frame();
      dispatch_async(dispatch_get_main_queue(), ^{
        if (generation != self->_generation)
          return;
        [self display:frame];
        if (self->_autoPlay) {
          self->_autoPlay = NO;
          [self toggleRun];
        }
      });
    } catch (const std::exception &error) {
      NSString *message = [NSString stringWithUTF8String:error.what()];
      dispatch_async(dispatch_get_main_queue(), ^{
        self->_status.text = message;
      });
    }
  });
}
- (void)toggleRun {
  if (_lastIndex >= (_input.selectedSegmentIndex == 0 ? 119 : 89))
    return;
  _running = !_running;
  [_play setTitle:_running ? @"Pause" : @"Run" forState:UIControlStateNormal];
}
- (void)tick {
  if (_running)
    [self processNext];
}
- (void)singleStep {
  _running = NO;
  [_play setTitle:@"Run" forState:UIControlStateNormal];
  [self processNext];
}
- (void)processNext {
  if (_busy)
    return;
  _busy = YES;
  BOOL learning = _learning.on;
  NSInteger generation = _generation;
  dispatch_async(_worker, ^{
    try {
      BOOL advanced = self->_session->step(learning);
      Frame frame = self->_session->frame();
      if (advanced) {
        if (learning)
          self->_sawOn = YES;
        else
          self->_sawOff = YES;
        if (frame.visible) {
          self->_sumIou += frame.iou;
          self->_visible++;
          if (frame.iou >= .5)
            self->_successes++;
        }
        if (!frame.valid)
          self->_lost++;
        self->_reinitializations += frame.reinitialized;
        self->_positive += frame.positiveUpdates;
        self->_negative += frame.negativeUpdates;
        self->_csv << frame.index << ',' << learning << ',' << frame.valid << ',' << frame.box[0]
                   << ',' << frame.box[1] << ',' << frame.box[2] << ',' << frame.box[3] << ','
                   << frame.iou << ',' << frame.reinitialized << ',' << frame.positiveUpdates << ','
                   << frame.negativeUpdates << ',' << frame.processingMs << '\n';
        self->_csv.flush();
        if (frame.done) {
          NSString *documents =
              NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
                  .firstObject;
          NSDictionary *metrics = @{
            @"platform" : @"iOS UIKit",
            @"learning_mode" : self->_sawOn && self->_sawOff ? @"mixed"
            : self->_sawOn                                   ? @"on"
                                                             : @"off",
            @"input" : [NSString stringWithUTF8String:frame.inputLabel.c_str()],
            @"frames" : @(frame.total),
            @"online_updates_last_frame" : @(learning),
            @"visible_frames" : self->_inputSynthetic ? @(self->_visible) : NSNull.null,
            @"visible_iou_at_least_half" : self->_inputSynthetic ? @(self->_successes)
                                                                 : NSNull.null,
            @"mean_visible_iou" : self->_visible ? @(self->_sumIou / self->_visible) : NSNull.null,
            @"lost_frames" : @(self->_lost),
            @"reinitializations" : @(self->_reinitializations),
            @"positive_updates" : @(self->_positive),
            @"negative_updates" : @(self->_negative)
          };
          [[NSJSONSerialization dataWithJSONObject:metrics
                                           options:NSJSONWritingPrettyPrinted
                                             error:nil]
              writeToFile:[documents stringByAppendingPathComponent:@"native-metrics.json"]
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
          [self->_play setTitle:@"Done" forState:UIControlStateNormal];
        }
      });
    } catch (const std::exception &error) {
      NSString *message = [NSString stringWithUTF8String:error.what()];
      dispatch_async(dispatch_get_main_queue(), ^{
        self->_busy = NO;
        self->_running = NO;
        self->_status.text = message;
      });
    }
  });
}
- (void)display:(const Frame &)frame {
  _displayWidth = frame.width;
  _displayHeight = frame.height;
  _lastIndex = frame.index;
  NSData *data = [NSData dataWithBytes:frame.rgba.data() length:frame.rgba.size()];
  CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGImageRef image = CGImageCreate(frame.width, frame.height, 8, 32, frame.width * 4, space,
                                   kCGImageAlphaPremultipliedLast, provider, NULL, false,
                                   kCGRenderingIntentDefault);
  _image.image = [UIImage imageWithCGImage:image];
  CGImageRelease(image);
  CGColorSpaceRelease(space);
  CGDataProviderRelease(provider);
  _status.text =
      [NSString stringWithFormat:@"%@ · frame %d/%d\n%@%@",
                                 [NSString stringWithUTF8String:frame.inputLabel.c_str()],
                                 frame.index + 1, frame.total, frame.done ? @"DONE · " : @"",
                                 frame.index < 0 ? @"Ready: initial selection"
                                 : frame.valid   ? @"Native estimate"
                                                 : @"Target lost"];
  _detail.text = [NSString
      stringWithFormat:@"box %.1f,%.1f %.1f×%.1f  · %.1f ms\nconfidence %.3f · correlation "
                       @"%.3f\nIoU %@ · detector reinit %@\nonline +%lu / −%lu (this frame)",
                       frame.box[0], frame.box[1], frame.box[2], frame.box[3], frame.processingMs,
                       frame.confidence, frame.correlation,
                       frame.iou >= 0 ? [NSString stringWithFormat:@"%.3f", frame.iou]
                                      : @"unmeasured",
                       frame.reinitialized ? @"yes" : @"no", frame.positiveUpdates,
                       frame.negativeUpdates];
}
- (CGPoint)imagePoint:(CGPoint)point {
  double scale =
      MIN(_image.bounds.size.width / _displayWidth, _image.bounds.size.height / _displayHeight);
  double ox = (_image.bounds.size.width - _displayWidth * scale) / 2,
         oy = (_image.bounds.size.height - _displayHeight * scale) / 2;
  return CGPointMake(MAX(0, MIN(_displayWidth, (point.x - ox) / scale)),
                     MAX(0, MIN(_displayHeight, (point.y - oy) / scale)));
}
- (void)selectBox:(UIPanGestureRecognizer *)gesture {
  if (_lastIndex >= 0 || _busy)
    return;
  CGPoint point = [gesture locationInView:_image];
  if (gesture.state == UIGestureRecognizerStateBegan)
    _selectionStart = point;
  CGRect rect = CGRectMake(MIN(_selectionStart.x, point.x), MIN(_selectionStart.y, point.y),
                           fabs(point.x - _selectionStart.x), fabs(point.y - _selectionStart.y));
  _selection.path = [UIBezierPath bezierPathWithRect:rect].CGPath;
  if (gesture.state == UIGestureRecognizerStateEnded) {
    CGPoint a = [self imagePoint:_selectionStart], b = [self imagePoint:point];
    _selection.path = nil;
    dispatch_async(_worker, ^{
      try {
        self->_session->setInitialBox(MIN(a.x, b.x), MIN(a.y, b.y), fabs(a.x - b.x),
                                      fabs(a.y - b.y));
        Frame frame = self->_session->frame();
        dispatch_async(dispatch_get_main_queue(), ^{
          [self display:frame];
        });
      } catch (const std::exception &error) {
        NSString *message = [NSString stringWithUTF8String:error.what()];
        dispatch_async(dispatch_get_main_queue(), ^{
          self->_status.text = message;
        });
      }
    });
  }
}
@end
@interface PredatorDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow *window;
@end
@implementation PredatorDelegate
- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)options {
  self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
  self.window.rootViewController = [PredatorController new];
  [self.window makeKeyAndVisible];
  return YES;
}
@end
int main(int argc, char **argv) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass(PredatorDelegate.class));
  }
}
