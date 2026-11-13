// Negative controls compiled directly against unchanged recovered source.
#include "Classifier.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
int main(int argc, char **argv) {
  std::string mode = argc > 1 ? argv[1] : "negative";
  if (mode == "borrowed") {
    std::vector<unsigned char> frame(4096, 120);
    {
      TldImage image;
      image.createFromImage(frame.data(), 64, 64);
    }
    return 0;
  }
  auto data = (unsigned char *)std::malloc(4096);
  std::memset(data, 120, 4096);
  TldImage image;
  image.createFromImage(data, 64, 64);
  if (mode == "endpoint") {
    PointTest point(1, 1, 0, 0);
    volatile int value = point.test(&image, 0, 0, 64, 64);
    return value;
  }
  std::srand(0);
  Classifier c(10, 8);
  c.train(&image, 16, 16, 32, 32, 1);
  float before = c.classify(&image, 16, 16, 32, 32);
  c.train(&image, 16, 16, 32, 32, 0);
  float after = c.classify(&image, 16, 16, 32, 32);
  std::cout << "Original positive posterior=" << before << " after negative update=" << after
            << " (correct fern counting expects 0.5)\n";
  return after == .5f ? 0 : 1;
}
