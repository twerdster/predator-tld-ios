// Headless adapter for the recovered tracker, detector, fern classifier and learning.
#include "Tld.h"
#include "TldDetector.h"
#include "PointTest.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <array>
#include <iostream>
#include <iomanip>
namespace fs = std::filesystem;
using Box = std::array<double, 4>;
void require(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(what);
}
bool valid(Box b) { return b[2] >= 15 && b[3] >= 15 && std::isfinite(b[0]) && std::isfinite(b[1]); }
double overlap(Box a, Box b) { return TldDetector::bbOverlap(a.data(), b.data()); }
cv::Mat texture() {
  cv::Mat p(54, 66, CV_8UC1);
  for (int y = 0; y < p.rows; ++y)
    for (int x = 0; x < p.cols; ++x)
      p.at<unsigned char>(y, x) = 110 + int(65 * std::cos(x * .15) * std::cos(y * .13));
  cv::rectangle(p, {3, 3, 60, 48}, cv::Scalar(235), 3);
  cv::circle(p, {18, 18}, 8, cv::Scalar(25), -1);
  cv::circle(p, {46, 20}, 6, cv::Scalar(245), -1);
  cv::putText(p, "TLD", {9, 43}, cv::FONT_HERSHEY_SIMPLEX, .55, cv::Scalar(30), 2);
  return p;
}
cv::Mat synthetic(int i, Box &truth, bool &visible) {
  cv::Mat frame(240, 320, CV_8UC1);
  for (int y = 0; y < 240; ++y)
    for (int x = 0; x < 320; ++x)
      frame.at<unsigned char>(y, x) = 35 + (x * 7 + y * 11) % 20;
  int x = i < 50 ? 35 + i : 170 + (i - 65) / 2;
  int y = 80 + int(12 * std::sin(i * .08));
  truth = {double(x), double(y), 66, 54};
  visible = !(i >= 50 && i < 65);
  if (visible)
    texture().copyTo(frame(cv::Rect(x, y, 66, 54)));
  return frame;
}
cv::Mat readPGM(const fs::path &path) {
  std::ifstream f(path, std::ios::binary);
  std::string magic;
  int w, h, max;
  f >> magic >> w >> h >> max;
  require(magic == "P5" && w > 0 && h > 0 && w <= 4096 && h <= 4096 && max == 255, "invalid PGM");
  f.get();
  cv::Mat m(h, w, CV_8UC1);
  f.read((char *)m.data, w * h);
  require(bool(f), "short PGM");
  return m;
}
void writePPM(const fs::path &path, const cv::Mat &bgr) {
  std::ofstream f(path, std::ios::binary);
  f << "P6\n" << bgr.cols << ' ' << bgr.rows << "\n255\n";
  for (int y = 0; y < bgr.rows; ++y)
    for (int x = 0; x < bgr.cols; ++x) {
      auto p = bgr.at<cv::Vec3b>(y, x);
      char rgb[] = {char(p[2]), char(p[1]), char(p[0])};
      f.write(rgb, 3);
    }
  require(bool(f), "write PPM");
}
void selfTest() {
  Box a = {1, 2, 10, 10}, b = {6, 2, 10, 10};
  require(std::abs(overlap(a, b) - 1. / 3) < 1e-12, "IoU reference");
  cv::Mat bytes(64, 64, CV_8UC1, cv::Scalar(120));
  {
    TldImage image;
    image.createFromImage(bytes.data, 64, 64);
    PointTest endpoints(1, 1, 0, 0);
    require(endpoints.test(&image, 0, 0, 64, 64) == 0, "endpoint clamping");
  }
  bytes.at<unsigned char>(0, 0) = 5;
  require(bytes.at<unsigned char>(0, 0) == 5, "TldImage borrowed storage lifetime");
  TldImage image;
  image.createFromImage(bytes.data, 64, 64);
  std::srand(0);
  Classifier classifier(10, 8);
  classifier.train(&image, 0, 0, 64, 64, 1);
  float positive = classifier.classify(&image, 0, 0, 64, 64);
  classifier.train(&image, 0, 0, 64, 64, 0);
  float negative = classifier.classify(&image, 0, 0, 64, 64);
  require(positive == 1 && std::abs(negative - .5) < 1e-6,
          "negative fern learning must lower posterior");
  {
    double roi[] = {-1, 0, 30, 30};
    bool rejected = false;
    try {
      Tld t(64, 64, bytes.data, roi, 10, 8, .1, .5, 20, 20);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "invalid ROI must reject");
  }
  {
    Box corner = {34, 34, 30, 30};
    Tld t(64, 64, bytes.data, corner.data(), 5, 8, .1, .5, 20, 20);
    std::vector<unsigned char> empty(4096, 0);
    Box result{};
    t.tldProcessFrame(64, 64, empty.data(), corner.data(), result.data(), .05, .35, .12, false,
                      true);
    require(!valid(result), "textureless frame must return an empty ROI");
  }
  std::cout << "PASS border ROI/warp and textureless loss; IoU analytical, endpoint clamp, "
               "borrowed frame lifetime, positive/negative fern posterior 1->0.5, invalid ROI\n";
}
int main(int argc, char **argv) {
  try {
    fs::path out = "predator-output";
    std::string video;
    Box roi{};
    bool synth = false, tests = false, learning = true, occlude = false;
    int limit = 120;
    double resizeWidth = 320;
    for (int i = 1; i < argc; ++i) {
      std::string a = argv[i];
      if (a == "--self-test")
        tests = true;
      else if (a == "--synthetic")
        synth = true;
      else if (a == "--frames-dir" && i + 1 < argc)
        video = argv[++i];
      else if (a == "--roi" && i + 1 < argc) {
        std::stringstream ss(argv[++i]);
        char c;
        ss >> roi[0] >> c >> roi[1] >> c >> roi[2] >> c >> roi[3];
      } else if (a == "--output" && i + 1 < argc)
        out = argv[++i];
      else if (a == "--frames" && i + 1 < argc)
        limit = std::stoi(argv[++i]);
      else if (a == "--learning" && i + 1 < argc)
        learning = std::stoi(argv[++i]) != 0;
      else if (a == "--width" && i + 1 < argc)
        resizeWidth = std::stod(argv[++i]);
      else
        throw std::invalid_argument(
            "usage: predator_demo --self-test | --synthetic | --frames-dir DIRECTORY --roi x,y,w,h "
            "[--width 320 --frames 120 --learning 0/1 --output DIR]");
    }
    if (tests) {
      selfTest();
      if (!synth && video.empty())
        return 0;
    }
    require(synth || !video.empty(), "choose --synthetic or --video");
    fs::create_directories(out);
    std::vector<fs::path> inputs;
    if (!synth) {
      for (auto &p : fs::directory_iterator(video))
        if (p.is_regular_file() && p.path().extension() == ".pgm" &&
            p.path().filename().string()[0] != '.')
          inputs.push_back(p.path());
      std::sort(inputs.begin(), inputs.end());
      require(!inputs.empty(), "no PGM frames");
    }
    std::ofstream csv(out / "tracks.csv");
    csv << "frame,gt_visible,x,y,w,h,gt_x,gt_y,gt_w,gt_h,iou,confidence,correlation,tracker_"
           "attempted,detector_candidates,reinitialized,positive_updates,negative_updates,process_"
           "ms\n";
    Tld *tld = nullptr;
    Box box{}, truth{};
    bool visible = true;
    double sumIou = 0;
    int visibleCount = 0, successes = 0, reinitializations = 0, lost = 0, postFound = -1;
    unsigned long positives = 0, negatives = 0;
    std::vector<cv::Point> trail;
    int frames = 0;
    for (int i = 0; i < limit; ++i) {
      cv::Mat gray, color;
      if (synth) {
        gray = synthetic(i, truth, visible);
        box = i ? box : truth;
      } else {
        if (i >= int(inputs.size()))
          break;
        cv::Mat raw = readPGM(inputs[i]);
        if (resizeWidth > 0)
          cv::resize(
              raw, raw,
              cv::Size(int(resizeWidth), int(std::round(raw.rows * resizeWidth / raw.cols))));
        if (raw.channels() == 3)
          cv::cvtColor(raw, gray, cv::COLOR_BGR2GRAY);
        else
          gray = raw.clone();
        if (!i)
          box = roi;
        truth = {};
        visible = false;
      }
      if (!gray.isContinuous())
        gray = gray.clone();
      cv::cvtColor(gray, color, cv::COLOR_GRAY2BGR);
      TldStats stats;
      auto begin = std::chrono::steady_clock::now();
      if (i == 0) {
        tld = new Tld(gray.cols, gray.rows, gray.data, box.data(), 10, 10, .1, .5, 35, 30, true,
                      false);
      } else {
        Box next{};
        tld->tldProcessFrame(gray.cols, gray.rows, gray.data, box.data(), next.data(), .05, .35,
                             .12, false, learning);
        box = next;
        stats = tld->last;
      }
      double ms =
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
              .count();
      double iou = synth && visible ? overlap(box, truth) : -1;
      if (synth && visible) {
        sumIou += iou;
        ++visibleCount;
        if (iou >= .5)
          ++successes;
        if (i >= 65 && iou >= .5 && postFound < 0)
          postFound = i;
      }
      if (!valid(box))
        ++lost;
      if (synth && !visible)
        require(!valid(box), "fully hidden synthetic target must be reported lost");
      reinitializations += stats.reinitialized;
      positives += stats.positiveUpdates;
      negatives += stats.negativeUpdates;
      if (synth && visible)
        cv::rectangle(color, cv::Rect(int(truth[0]), int(truth[1]), int(truth[2]), int(truth[3])),
                      cv::Scalar(0, 220, 0), 1);
      if (valid(box)) {
        cv::rectangle(color, cv::Rect(int(box[0]), int(box[1]), int(box[2]), int(box[3])),
                      cv::Scalar(0, 0, 255), 2);
        trail.emplace_back(int(box[0] + box[2] / 2), int(box[1] + box[3] / 2));
      }
      for (size_t j = 1; j < trail.size(); ++j)
        cv::line(color, trail[j - 1], trail[j], cv::Scalar(0, 180, 255), 1);
      std::string label = (synth ? "SYNTHETIC " : "RECORDED ") + std::to_string(i) +
                          " learning=" + (learning ? "on" : "off") +
                          (stats.reinitialized ? " DETECTOR REINIT" : "");
      cv::putText(color, label, {5, 15}, cv::FONT_HERSHEY_SIMPLEX, .35, cv::Scalar(240, 240, 240),
                  1);
      if (synth && !visible)
        cv::putText(color, "target fully hidden", {5, 230}, cv::FONT_HERSHEY_SIMPLEX, .45,
                    cv::Scalar(0, 220, 255), 1);
      std::ostringstream name;
      name << std::setw(5) << std::setfill('0') << i << ".ppm";
      writePPM(out / name.str(), color);
      csv << i << ',' << (synth ? int(visible) : -1) << ',' << box[0] << ',' << box[1] << ','
          << box[2] << ',' << box[3] << ',' << truth[0] << ',' << truth[1] << ',' << truth[2] << ','
          << truth[3] << ',' << iou << ',' << stats.confidence << ',' << stats.correlation << ','
          << stats.trackerAttempted << ',' << stats.detectorCandidates << ',' << stats.reinitialized
          << ',' << stats.positiveUpdates << ',' << stats.negativeUpdates << ',' << ms << '\n';
      ++frames;
    }
    require(tld, "no input frames");
    delete tld;
    std::ofstream json(out / "metrics.json");
    json << "{\n\"input\":\""
         << (synth ? "controlled synthetic textured target with ground truth and full occlusion"
                   : "recorded real video; no ground truth supplied")
         << "\",\n\"frames\":" << frames
         << ",\n\"learning_enabled\":" << (learning ? "true" : "false")
         << ",\n\"ground_truth_supplied\":" << (synth ? "true" : "false")
         << ",\n\"mean_iou_visible\":";
    if (synth && visibleCount)
      json << sumIou / visibleCount;
    else
      json << "null";
    json << ",\n\"visible_iou_at_least_half\":" << (synth ? std::to_string(successes) : "null")
         << ",\n\"visible_frames\":" << (synth ? std::to_string(visibleCount) : "null")
         << ",\n\"reinitializations\":" << reinitializations << ",\n\"lost_frames\":" << lost
         << ",\n\"post_occlusion_first_iou_half_frame\":"
         << (synth && postFound >= 0 ? std::to_string(postFound) : "null")
         << ",\n\"online_positive_updates\":" << positives
         << ",\n\"online_negative_updates\":" << negatives << "\n}\n";
    if (synth && frames >= 70) {
      require(visibleCount > 0 && sumIou / visibleCount > .75 && successes >= visibleCount * .9,
              "synthetic visible tracking regression");
      require(postFound >= 65 && postFound <= 70 && reinitializations > 0,
              "synthetic detector reacquisition regression");
      require(lost >= 15, "synthetic full occlusion must be rejected");
      require(learning ? (positives > 0 && negatives > 0) : (positives == 0 && negatives == 0),
              "online learning control regression");
    }
    std::cout << "RESULT frames=" << frames
              << " meanIoU=" << (visibleCount ? sumIou / visibleCount : -1)
              << " successes=" << successes << " reinit=" << reinitializations
              << " postFound=" << postFound << " online+=" << positives << " online-=" << negatives
              << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
