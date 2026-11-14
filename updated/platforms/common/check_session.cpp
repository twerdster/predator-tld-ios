// Cross-platform regression for the exact session bridge used by native UIs.
#include "PredatorSession.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    predator::Session session;
    bool rejected = false;
    try {
      session.setInitialBox(-1, 0, 40, 40);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "invalid native selection must reject");
    rejected = false;
    try {
      session.reset(false, "");
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected && session.frame().height == 240 && session.frame().index == -1,
            "failed recorded reset must preserve the prior session");
    require(session.step(true) && session.frame().valid,
            "prior session remains usable after failed reset");
    require(argc > 1, "pass the recorded raw input to the Session regression");

    for (bool learning : {true, false}) {
      session.reset(true);
      double sum = 0;
      int visible = 0, successful = 0, hidden = 0, reacquired = -1;
      unsigned long positive = 0, negative = 0;
      while (session.step(learning)) {
        const auto &f = session.frame();
        require(f.rgba.size() == size_t(f.width) * f.height * 4, "native RGBA dimensions");
        for (double coordinate : f.box)
          require(std::isfinite(coordinate), "finite native box output");
        if (f.visible) {
          require(std::isfinite(f.iou) && f.iou >= 0 && f.iou <= 1, "finite visible IoU");
          sum += f.iou;
          visible++;
          successful += f.iou >= .5;
        } else {
          require(!f.valid, "native hidden frame must reject");
          hidden++;
        }
        if (f.index >= 65 && f.iou >= .5 && reacquired < 0)
          reacquired = f.index;
        if (f.index == 65)
          require(f.reinitialized, "native frame65 requires detector decision");
        positive += f.positiveUpdates;
        negative += f.negativeUpdates;
      }
      require(visible == 105 && hidden == 15 && successful >= 95 && sum / visible > .75,
              "native synthetic tracking acceptance");
      require(reacquired >= 65 && reacquired <= 70, "native synthetic reacquisition acceptance");
      require(learning ? (positive > 0 && negative > 0) : (positive == 0 && negative == 0),
              "native learning control");
      std::cout << "NATIVE_SESSION_RESULT learning=" << learning << " mean_iou=" << sum / visible
                << " successes=" << successful << " hidden=" << hidden
                << " reacquired=" << reacquired << " online_positive=" << positive
                << " online_negative=" << negative << '\n';
    }
    if (argc > 1) {
      session.reset(false, argv[1]);
      int count = 0, lost = 0, reinitializations = 0;
      while (session.step(true)) {
        const auto &f = session.frame();
        require(f.iou == -1 && !f.visible, "recorded native input must have no evaluation truth");
        count++;
        lost += !f.valid;
        reinitializations += f.reinitialized;
      }
      require(count == 90, "recorded native input complete");
      std::cout << "NATIVE_SESSION_RECORDED frames=" << count << " lost=" << lost
                << " reinitializations=" << reinitializations << " accuracy=unmeasured\n";
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL " << error.what() << '\n';
    return 1;
  }
}
