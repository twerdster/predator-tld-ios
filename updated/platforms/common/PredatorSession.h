// Updated native frontends run the recovered TLD core, one serial session per process.
#pragma once
#include <memory>
#include <string>
#include <vector>
namespace predator {
struct Frame {
  int index = -1, width = 320, height = 240, total = 120;
  std::vector<unsigned char> rgba;
  double box[4]{}, iou = -1, correlation = 0, confidence = 0, processingMs = 0;
  bool visible = false, valid = false, reinitialized = false, done = false;
  unsigned long positiveUpdates = 0, negativeUpdates = 0;
  std::string inputLabel;
};
class Session {
public:
  Session();
  ~Session();
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
  void reset(bool synthetic, const std::string &rawFramesPath = "");
  void setInitialBox(double x, double y, double w, double h);
  bool step(bool learning);
  const Frame &frame() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
} // namespace predator
