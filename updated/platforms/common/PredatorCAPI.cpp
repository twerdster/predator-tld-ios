// C ABI for desktop Tk UI; all tracking runs in the recovered C++ Session.
#include "PredatorSession.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
namespace {
struct Handle {
  predator::Session session;
  std::string error, json;
};
static std::string quote(const std::string &text) {
  std::string out = "\"";
  for (char c : text) {
    if (c == '\\' || c == '\"')
      out += '\\';
    if (c == '\n') {
      out += "\\n";
      continue;
    }
    out += c;
  }
  return out + '\"';
}
} // namespace
extern "C" {
void *predator_create() {
  try {
    return new Handle;
  } catch (...) {
    return nullptr;
  }
}
void predator_destroy(void *p) { delete static_cast<Handle *>(p); }
const char *predator_error(void *p) { return static_cast<Handle *>(p)->error.c_str(); }
int predator_reset(void *p, int synthetic, const char *path) {
  auto *h = static_cast<Handle *>(p);
  try {
    h->session.reset(synthetic != 0, path ? path : "");
    h->error.clear();
    return 1;
  } catch (const std::exception &e) {
    h->error = e.what();
    return 0;
  }
}
int predator_step(void *p, int learning) {
  auto *h = static_cast<Handle *>(p);
  try {
    h->error.clear();
    return h->session.step(learning != 0) ? 1 : 0;
  } catch (const std::exception &e) {
    h->error = e.what();
    return -1;
  }
}
const char *predator_frame_json(void *p) {
  auto *h = static_cast<Handle *>(p);
  const auto &f = h->session.frame();
  std::ostringstream s;
  s << "{\"frame\":" << f.index << ",\"width\":" << f.width << ",\"height\":" << f.height
    << ",\"total\":" << f.total << ",\"box\":[" << f.box[0] << ',' << f.box[1] << ',' << f.box[2]
    << ',' << f.box[3] << "],\"valid\":" << (f.valid ? "true" : "false")
    << ",\"visible\":" << (f.visible ? "true" : "false")
    << ",\"done\":" << (f.done ? "true" : "false")
    << ",\"iou\":" << (f.iou >= 0 ? std::to_string(f.iou) : "null")
    << ",\"confidence\":" << f.confidence << ",\"correlation\":" << f.correlation
    << ",\"processing_ms\":" << f.processingMs
    << ",\"reinitialized\":" << (f.reinitialized ? "true" : "false")
    << ",\"positive_updates\":" << f.positiveUpdates
    << ",\"negative_updates\":" << f.negativeUpdates << ",\"input\":" << quote(f.inputLabel) << "}";
  h->json = s.str();
  return h->json.c_str();
}
int predator_copy_rgba(void *p, unsigned char *destination, int capacity) {
  const auto &v = static_cast<Handle *>(p)->session.frame().rgba;
  if (capacity < 0 || size_t(capacity) < v.size())
    return -1;
  std::memcpy(destination, v.data(), v.size());
  return int(v.size());
}
}
