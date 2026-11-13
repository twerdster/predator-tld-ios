// Copyright 2011 Zdenek Kalal
//
// This file is part of TLD.
//
// TLD is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TLD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TLD.  If not, see <http://www.gnu.org/licenses/>.

#include "Tld.h"
#include "Utils.h"
#include "TldImage.h"
#include "Classifier.h"
#include "TldDetector.h"
#include "Tracker.h"
#include <stdlib.h>
#include <math.h>

// Updated 2026: headless lifetime/ROI fixes around the recovered TLD decisions.
#include <array>
#include <stdexcept>
#include <vector>
namespace {
bool initialized = false;
int frameWidth = 0, frameHeight = 0;
CvSize *frameSize = nullptr;
Classifier *classifier = nullptr;
PredatorTracker *tracker = nullptr;
TldDetector *detector = nullptr;
IplImage *firstPatch = nullptr;
double confidence = 0;
bool video = false;
bool validBox(const double *b) {
  return b && std::isfinite(b[0]) && std::isfinite(b[1]) && std::isfinite(b[2]) &&
         std::isfinite(b[3]) && b[0] >= 0 && b[1] >= 0 && b[2] >= 15 && b[3] >= 15 &&
         b[0] + b[2] <= frameWidth && b[1] + b[3] <= frameHeight;
}
IplImage *patch(IplImage *image, const double *b) {
  cvSetImageROI(image, cvRect(int(b[0]), int(b[1]), int(b[2]), int(b[3])));
  IplImage *resized = cvCreateImage(cvSize(15, 15), IPL_DEPTH_8U, 1);
  cvResize(image, resized);
  cvResetImageROI(image);
  return resized;
}
double correlation(IplImage *image, const double *b) {
  if (!validBox(b))
    return 0;
  IplImage *q = patch(image, b);
  unsigned char a[225], z[225];
  for (int y = 0; y < 15; ++y)
    for (int x = 0; x < 15; ++x) {
      a[y * 15 + x] = (unsigned char)firstPatch->imageData[y * firstPatch->widthStep + x];
      z[y * 15 + x] = (unsigned char)q->imageData[y * q->widthStep + x];
    }
  double value = ccorr_normed(a, z, 225);
  cvReleaseImage(&q);
  return value;
}
} // namespace
Tld::Tld(int width, int height, unsigned char *frame, double *bb, int trees, int feats,
         double minScale, double maxScale, int xs, int ys, bool is_video, bool blur) {
  if (initialized)
    throw std::runtime_error("Recovered TLD supports one live instance per process");
  frameWidth = width;
  frameHeight = height;
  if (!frame || width < 32 || height < 32 || !validBox(bb) || trees < 1 || trees > 64 ||
      feats < 1 || feats > 16 || xs < 2 || ys < 2)
    throw std::invalid_argument("Invalid image, ROI or fern/scan configuration");
  video = is_video;
  std::srand(0); // Reproducible recovered random fern features.
  frameSize = new CvSize(cvSize(width, height));
  TldImage first;
  first.createFromImage(frame, width, height, blur);
  IplImage *image = imageFromChar(frame, frameSize, width, height);
  firstPatch = patch(image, bb);
  classifier = new Classifier(trees, feats);
  tracker = new PredatorTracker(width, height, frameSize, image, classifier);
  detector = new TldDetector(width, height, bb, classifier, xs, ys);
  cvReleaseImage(&image);
  classifier->train(&first, int(bb[0]), int(bb[1]), int(bb[2]), int(bb[3]), 1);
  bbWarpPatch(&first, bb, classifier);
  trainClassifier(&first, bb, classifier);
  confidence = 1;
  initialized = true;
}
Tld::~Tld() {
  delete tracker;
  delete detector;
  delete classifier;
  cvReleaseImage(&firstPatch);
  delete frameSize;
  tracker = nullptr;
  detector = nullptr;
  classifier = nullptr;
  frameSize = nullptr;
  initialized = false;
}
unsigned long Tld::positiveUpdates() const { return classifier->positiveUpdates; }
unsigned long Tld::negativeUpdates() const { return classifier->negativeUpdates; }
void Tld::tldProcessFrame(int width, int height, unsigned char *pixels, double *bb, double *out,
                          double minTrack, double minReinit, double minLearn, bool blur,
                          bool learning) {
  if (width != frameWidth || height != frameHeight || !pixels || !out)
    throw std::invalid_argument("Frame dimensions must match initialization");
  last = {};
  IplImage *image = imageFromChar(pixels, frameSize, width, height);
  TldImage current;
  current.createFromImage(pixels, width, height, blur);
  double *tracked = nullptr;
  if (confidence > minTrack && validBox(bb)) {
    last.trackerAttempted = true;
    tracked = tracker->track(image, &current, bb);
  } else {
    tracker->setPrevFrame(image);
    tracked = new double[5]{};
  }
  if (!validBox(tracked))
    std::fill(tracked, tracked + 5, 0.0);
  auto candidates = detector->detect(&current, tracked[4] > minTrack ? tracked : nullptr);
  last.detectorCandidates = candidates->size();
  std::vector<double> correlations(candidates->size());
  int best = -1;
  double bestCorr = 0;
  for (size_t i = 0; i < candidates->size(); ++i) {
    double *b = (*candidates)[i];
    double c = correlation(image, b);
    correlations[i] = c;
    if (c > bestCorr && b[4] > .6) {
      best = int(i);
      bestCorr = c;
    }
  }
  double trackingCorr = tracked[4] > 0 ? correlation(image, tracked) : 0;
  if (best >= 0 && bestCorr > minReinit && bestCorr > tracked[4] &&
      bestCorr > (video ? .943 : .949) && bestCorr > trackingCorr) {
    std::copy((*candidates)[best], (*candidates)[best] + 5, tracked);
    last.reinitialized = true;
  } else if (tracked[4] > minTrack && confidence > minLearn && trackingCorr > .94) {
    if (learning) {
      for (size_t i = 0; i < candidates->size(); ++i) {
        double *b = (*candidates)[i];
        if (b[5] == 1 && correlations[i] > .91) {
          classifier->train(&current, int(b[0]), int(b[1]), int(b[2]), int(b[3]), 1);
          ++last.positiveUpdates;
        } else if (b[5] == 0 && correlations[i] < .999) {
          classifier->train(&current, int(b[0]), int(b[1]), int(b[2]), int(b[3]), 0);
          ++last.negativeUpdates;
        }
      }
      classifier->train(&current, int(tracked[0]), int(tracked[1]), int(tracked[2]),
                        int(tracked[3]), 1);
      ++last.positiveUpdates;
    }
  } else {
    std::fill(tracked, tracked + 5, 0.0);
    last.rejected = true;
  }
  confidence = tracked[4];
  last.confidence = confidence;
  last.correlation = trackingCorr;
  std::copy(tracked, tracked + 4, out);
  delete[] tracked;
  for (auto b : *candidates)
    delete[] b;
  delete candidates;
  cvReleaseImage(&image);
}
