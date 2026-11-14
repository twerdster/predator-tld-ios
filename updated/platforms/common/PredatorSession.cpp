// Native adapter for the recovered TLD implementation under updated/src/.
// Synthetic generation matches updated/tools/main.cpp; later truth is scoring/overlay only.
#include "PredatorSession.h"
#include "Tld.h"
#include "TldDetector.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace predator {
using Box = std::array<double, 4>;
static bool validBox(const Box &b) {
  return std::isfinite(b[0]) && std::isfinite(b[1]) && std::isfinite(b[2]) && std::isfinite(b[3]) &&
         b[2] >= 15 && b[3] >= 15;
}
static cv::Mat targetTexture() {
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
struct Session::Impl {
  std::unique_ptr<Tld> tld;
  Frame output;
  bool synthetic = true;
  std::vector<unsigned char> raw;
  Box initial{35, 80, 66, 54}, box{}, truth{};
  std::vector<cv::Point> trail;
  cv::Mat image(int index, bool &visible) {
    if (!synthetic) {
      visible = false;
      return cv::Mat(180, 320, CV_8UC1, raw.data() + size_t(index) * 320 * 180).clone();
    }
    cv::Mat gray(240, 320, CV_8UC1);
    for (int y = 0; y < 240; ++y)
      for (int x = 0; x < 320; ++x)
        gray.at<unsigned char>(y, x) = 35 + (x * 7 + y * 11) % 20;
    int x = index < 50 ? 35 + index : 170 + (index - 65) / 2;
    int y = 80 + int(12 * std::sin(index * .08));
    truth = {double(x), double(y), 66, 54};
    visible = !(index >= 50 && index < 65);
    if (visible)
      targetTexture().copyTo(gray(cv::Rect(x, y, 66, 54)));
    return gray;
  }
  void render(const cv::Mat &gray, bool processed, bool learning) {
    cv::Mat color, rgba;
    cv::cvtColor(gray, color, cv::COLOR_GRAY2BGR);
    auto rect = [](const Box &b) { return cv::Rect(int(b[0]), int(b[1]), int(b[2]), int(b[3])); };
    if (synthetic && output.visible)
      cv::rectangle(color, rect(truth), cv::Scalar(0, 220, 0), 1);
    if (processed && validBox(box)) {
      cv::rectangle(color, rect(box), cv::Scalar(0, 0, 255), 2);
      trail.emplace_back(int(box[0] + box[2] / 2), int(box[1] + box[3] / 2));
    } else if (!processed) {
      cv::rectangle(color, rect(initial), cv::Scalar(0, 180, 255), 1);
    }
    for (size_t j = 1; j < trail.size(); ++j)
      cv::line(color, trail[j - 1], trail[j], cv::Scalar(0, 180, 255), 1);
    std::string label = (synthetic ? "SYNTHETIC " : "RECORDED ") +
                        (processed ? std::to_string(output.index) : "ready") +
                        (processed ? std::string(" learning=") + (learning ? "on" : "off") : "");
    cv::putText(color, label, {5, 15}, cv::FONT_HERSHEY_SIMPLEX, .35, cv::Scalar(240, 240, 240), 1);
    if (synthetic && !output.visible)
      cv::putText(color, "target fully hidden", {5, 230}, cv::FONT_HERSHEY_SIMPLEX, .45,
                  cv::Scalar(0, 220, 255), 1);
    cv::cvtColor(color, rgba, cv::COLOR_BGR2RGBA);
    output.rgba.assign(rgba.data, rgba.data + rgba.total() * rgba.elemSize());
  }
};
Session::Session() : impl(new Impl) { reset(true); }
Session::~Session() = default;
void Session::reset(bool synthetic, const std::string &path) {
  // Validate/load first: a failed input selection must preserve the live session.
  std::vector<unsigned char> loaded;
  if (!synthetic) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() != std::streamoff(90 * 320 * 180))
      throw std::invalid_argument(
          "camel raw input must contain exactly 90 grayscale 320x180 frames");
    file.seekg(0);
    loaded.resize(90 * 320 * 180);
    file.read(reinterpret_cast<char *>(loaded.data()), loaded.size());
    if (!file)
      throw std::runtime_error("short camel raw frame input");
  }
  impl->tld.reset();
  impl->raw = std::move(loaded);
  impl->trail.clear();
  impl->synthetic = synthetic;
  impl->output = Frame{};
  impl->output.height = synthetic ? 240 : 180;
  impl->output.total = synthetic ? 120 : 90;
  impl->output.inputLabel =
      synthetic ? "Synthetic occlusion fixture" : "Recorded DAVIS camel · CC BY-NC 4.0";
  impl->initial = synthetic ? Box{35, 80, 66, 54} : Box{105, 35, 135, 85};
  impl->box = impl->initial;
  cv::Mat gray = impl->image(0, impl->output.visible);
  std::copy(impl->initial.begin(), impl->initial.end(), impl->output.box);
  impl->render(gray, false, true);
}
void Session::setInitialBox(double x, double y, double w, double h) {
  if (impl->output.index >= 0)
    throw std::logic_error("reset before changing the initial selection");
  Box selected{x, y, w, h};
  if (!validBox(selected) || x < 0 || y < 0 || x + w > 320 || y + h > impl->output.height)
    throw std::invalid_argument("selection must be at least 15x15 and inside the resized image");
  impl->initial = impl->box = selected;
  std::copy(selected.begin(), selected.end(), impl->output.box);
  cv::Mat gray = impl->image(0, impl->output.visible);
  impl->render(gray, false, true);
}
bool Session::step(bool learning) {
  if (impl->output.done)
    return false;
  Frame next;
  next.index = impl->output.index + 1;
  next.height = impl->output.height;
  next.total = impl->output.total;
  next.inputLabel = impl->output.inputLabel;
  auto start = std::chrono::steady_clock::now();
  cv::Mat gray = impl->image(next.index, next.visible);
  TldStats stats;
  if (next.index == 0) {
    impl->box = impl->initial;
    impl->tld.reset(new Tld(gray.cols, gray.rows, gray.data, impl->box.data(), 10, 10, .1, .5, 35,
                            30, true, false));
  } else {
    Box estimate{};
    impl->tld->tldProcessFrame(gray.cols, gray.rows, gray.data, impl->box.data(), estimate.data(),
                               .05, .35, .12, false, learning);
    impl->box = estimate;
    stats = impl->tld->last;
  }
  next.processingMs =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  if (!std::all_of(impl->box.begin(), impl->box.end(), [](double x) { return std::isfinite(x); }))
    impl->box = {};
  next.valid = validBox(impl->box);
  next.iou = impl->synthetic && next.visible
                 ? (next.valid ? TldDetector::bbOverlap(impl->box.data(), impl->truth.data()) : 0)
                 : -1;
  if (next.visible && !std::isfinite(next.iou))
    next.iou = 0;
  next.reinitialized = stats.reinitialized;
  next.confidence = stats.confidence;
  next.correlation = stats.correlation;
  next.positiveUpdates = stats.positiveUpdates;
  next.negativeUpdates = stats.negativeUpdates;
  next.done = next.index + 1 == next.total;
  std::copy(impl->box.begin(), impl->box.end(), next.box);
  impl->output = std::move(next);
  impl->render(gray, true, learning);
  std::cout << "PREDATOR_NATIVE frame=" << impl->output.index << " valid=" << impl->output.valid
            << " iou=" << impl->output.iou << " reinit=" << stats.reinitialized
            << " online_positive=" << stats.positiveUpdates
            << " online_negative=" << stats.negativeUpdates
            << " processing_ms=" << impl->output.processingMs << std::endl;
  return true;
}
const Frame &Session::frame() const { return impl->output; }
} // namespace predator
