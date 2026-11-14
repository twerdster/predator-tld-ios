package org.awetzler.predator.updated;

/** All methods are called on MainActivity's process-wide serial native worker. */
final class NativeBridge {
    static { System.loadLibrary("predator_android"); }
    static native long create();
    static native void destroy(long handle);
    static native FrameResult reset(long handle, boolean synthetic, String rawPath);
    static native FrameResult step(long handle, boolean learning);
    static native FrameResult select(long handle, double x, double y, double w, double h);

    static final class FrameResult {
        final int index, width, height, total;
        final int[] pixels;
        // x,y,w,h,IoU,correlation,confidence,visible,valid,reinitialized,done,pos,neg,processingMs.
        final double[] values;
        final String label;
        FrameResult(int index, int width, int height, int total, int[] pixels, double[] values, String label) {
            this.index = index; this.width = width; this.height = height; this.total = total;
            this.pixels = pixels; this.values = values; this.label = label;
        }
    }
}
