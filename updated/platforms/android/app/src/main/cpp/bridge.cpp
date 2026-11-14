// Android adapter for the updated recovered TLD pipeline. GPL-3.0-or-later.
#include "PredatorSession.h"
#include <jni.h>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
predator::Session &session(jlong handle) {
  if (!handle)
    throw std::logic_error("native session has been closed");
  return *reinterpret_cast<predator::Session *>(handle);
}
void fail(JNIEnv *env, const char *message) {
  if (!env->ExceptionCheck()) {
    jclass type = env->FindClass("java/lang/IllegalStateException");
    if (type)
      env->ThrowNew(type, message);
  }
}
jobject snapshot(JNIEnv *env, const predator::Frame &f) {
  const size_t count = size_t(f.width) * f.height;
  if (f.width <= 0 || f.height <= 0 || f.rgba.size() != count * 4)
    throw std::runtime_error("invalid native RGBA frame");
  std::vector<jint> argb(count);
  for (size_t i = 0; i < count; ++i) {
    const auto *p = f.rgba.data() + i * 4;
    argb[i] = jint((uint32_t(p[3]) << 24) | (uint32_t(p[0]) << 16) |
                   (uint32_t(p[1]) << 8) | uint32_t(p[2]));
  }
  jdouble values[] = {f.box[0], f.box[1], f.box[2], f.box[3], f.iou, f.correlation,
                      f.confidence, double(f.visible), double(f.valid), double(f.reinitialized),
                      double(f.done), double(f.positiveUpdates), double(f.negativeUpdates),
                      f.processingMs};
  jintArray pixels = env->NewIntArray(jsize(count));
  jdoubleArray metrics = env->NewDoubleArray(14);
  jstring label = env->NewStringUTF(f.inputLabel.c_str());
  if (!pixels || !metrics || !label)
    return nullptr;
  env->SetIntArrayRegion(pixels, 0, jsize(count), argb.data());
  env->SetDoubleArrayRegion(metrics, 0, 14, values);
  jclass type = env->FindClass("org/awetzler/predator/updated/NativeBridge$FrameResult");
  if (!type)
    return nullptr;
  jmethodID ctor = env->GetMethodID(type, "<init>", "(IIII[I[DLjava/lang/String;)V");
  if (!ctor)
    return nullptr;
  return env->NewObject(type, ctor, f.index, f.width, f.height, f.total, pixels, metrics, label);
}
} // namespace

#define JNI_METHOD(name) Java_org_awetzler_predator_updated_NativeBridge_##name
#define CATCH_NULL \
  catch (const std::exception &e) { fail(env, e.what()); return nullptr; } \
  catch (...) { fail(env, "unknown native exception"); return nullptr; }

extern "C" JNIEXPORT jlong JNICALL JNI_METHOD(create)(JNIEnv *env, jclass) {
  try { return reinterpret_cast<jlong>(new predator::Session); }
  catch (const std::exception &e) { fail(env, e.what()); return 0; }
  catch (...) { fail(env, "unknown native exception"); return 0; }
}
extern "C" JNIEXPORT void JNICALL JNI_METHOD(destroy)(JNIEnv *env, jclass, jlong handle) {
  try { delete reinterpret_cast<predator::Session *>(handle); }
  catch (const std::exception &e) { fail(env, e.what()); }
  catch (...) { fail(env, "unknown native exception"); }
}
extern "C" JNIEXPORT jobject JNICALL JNI_METHOD(reset)(JNIEnv *env, jclass, jlong handle,
                                                        jboolean synthetic, jstring path) {
  try {
    std::string filename;
    if (path) {
      const char *utf = env->GetStringUTFChars(path, nullptr);
      if (!utf) return nullptr;
      filename = utf;
      env->ReleaseStringUTFChars(path, utf);
    }
    auto &s = session(handle);
    s.reset(synthetic, filename);
    return snapshot(env, s.frame());
  } CATCH_NULL
}
extern "C" JNIEXPORT jobject JNICALL JNI_METHOD(step)(JNIEnv *env, jclass, jlong handle,
                                                       jboolean learning) {
  try {
    auto &s = session(handle);
    s.step(learning);
    return snapshot(env, s.frame());
  } CATCH_NULL
}
extern "C" JNIEXPORT jobject JNICALL JNI_METHOD(select)(JNIEnv *env, jclass, jlong handle,
                                                         jdouble x, jdouble y, jdouble w, jdouble h) {
  try {
    auto &s = session(handle);
    s.setInitialBox(x, y, w, h);
    return snapshot(env, s.frame());
  } CATCH_NULL
}
