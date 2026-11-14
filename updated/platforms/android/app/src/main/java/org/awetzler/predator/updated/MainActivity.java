package org.awetzler.predator.updated;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.File;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Updated UI; one process-wide FIFO worker protects the recovered C++ singleton. */
public final class MainActivity extends Activity {
    private final Handler main = new Handler(Looper.getMainLooper());
    // Activity recreation must not create a second concurrent native worker.
    // Old-instance destruction and new-instance initialization share this FIFO.
    private static final ExecutorService worker = Executors.newSingleThreadExecutor();
    private long nativeHandle; // Worker-thread access only.
    private File traceFile;    // Worker-thread access only.
    private Spinner source;
    private Switch learning;
    private Button play, restart, singleStep;
    private TextView status, metrics;
    private FrameView image;
    private boolean ready, busy, playing, finished, synthetic = true;
    private volatile boolean closed;
    private volatile int generation;
    private int lastSource;
    private long positiveTotal, negativeTotal;
    private int reinitializations, lostFrames;
    private boolean autoRun;
    private final Runnable nextStep = () -> { if (playing) advance(); };

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        synthetic = !"camel".equals(getIntent().getStringExtra("source"));
        autoRun = getIntent().getBooleanExtra("autoRun", false);
        ScrollView scroll = new ScrollView(this);
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(dp(16), dp(12), dp(16), dp(18));
        scroll.addView(column);
        TextView title = text("Predator · recovered TLD", 23, Color.rgb(23, 56, 70));
        column.addView(title);
        column.addView(text("Custom Lucas–Kanade tracking · fern detection · online updates", 13, Color.DKGRAY));
        source = new Spinner(this);
        source.setId(R.id.source_selector);
        source.setContentDescription("Input sequence");
        source.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
                new String[]{"Synthetic · 120-frame occlusion test", "Recorded · DAVIS camel, 90 frames"}));
        lastSource = synthetic ? 0 : 1;
        source.setSelection(lastSource);
        column.addView(source, new LinearLayout.LayoutParams(-1, dp(48)));
        image = new FrameView();
        image.setId(R.id.frame_view);
        image.setContentDescription("Actual native tracking output. Red estimate, green synthetic truth, yellow path.");
        column.addView(image, new LinearLayout.LayoutParams(-1, -2));
        status = text("Loading native pipeline…", 15, Color.rgb(23, 56, 70));
        status.setId(R.id.status);
        column.addView(status);
        metrics = text("", 14, Color.DKGRAY);
        metrics.setId(R.id.metrics);
        metrics.setTypeface(android.graphics.Typeface.MONOSPACE);
        metrics.setMinLines(5);
        column.addView(metrics);
        LinearLayout controls = new LinearLayout(this);
        play = button("Play", R.id.play_pause, controls);
        singleStep = button("Step", R.id.single_step, controls);
        restart = button("Restart", R.id.restart, controls);
        column.addView(controls);
        learning = new Switch(this);
        learning.setId(R.id.learning);
        learning.setText("Online updates (initialization always trains)");
        learning.setTextSize(14);
        learning.setChecked(getIntent().getBooleanExtra("learning", true));
        column.addView(learning, new LinearLayout.LayoutParams(-1, dp(48)));
        column.addView(text("Red: estimate. Yellow: estimated-center path. Green: synthetic truth only. "
                + "Drag a box before the first step to select an initial region (minimum 15×15 pixels). "
                + "Restart restores the fixture’s default selection.", 12, Color.DKGRAY));
        TextView attribution = text("Recorded input: DAVIS camel · CC BY-NC 4.0. "
                + "Grayscale, resized to 320×180. Recorded boxes are qualitative; no accuracy labels loaded.", 12, Color.DKGRAY);
        attribution.setPadding(0, dp(10), 0, 0);
        column.addView(attribution);
        Button credits = new Button(this);
        credits.setId(R.id.credits);
        credits.setText("Credits, scope and licenses");
        credits.setOnClickListener(v -> showCredits());
        column.addView(credits);
        setContentView(scroll);
        play.setOnClickListener(v -> {
            if (playing) pause();
            else if (ready && !finished) { playing = true; updateControls(); advance(); }
        });
        singleStep.setOnClickListener(v -> { pause(); advance(); });
        restart.setOnClickListener(v -> resetSequence(false));
        learning.setOnCheckedChangeListener((v, checked) -> {
            if (ready) status.setText("Online updates " + (checked ? "on" : "off") + " for subsequent steps");
        });
        source.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (position != lastSource) {
                    lastSource = position;
                    synthetic = position == 0;
                    resetSequence(false);
                }
            }
            @Override public void onNothingSelected(AdapterView<?> parent) { }
        });
        updateControls();
        resetSequence(autoRun);
    }

    private TextView text(String value, int size, int color) {
        TextView view = new TextView(this);
        view.setText(value); view.setTextSize(size); view.setTextColor(color);
        view.setPadding(0, dp(3), 0, dp(3));
        return view;
    }
    private Button button(String label, int id, LinearLayout parent) {
        Button b = new Button(this); b.setText(label); b.setId(id);
        parent.addView(b, new LinearLayout.LayoutParams(0, dp(52), 1));
        return b;
    }
    private int dp(int value) { return Math.round(value * getResources().getDisplayMetrics().density); }
    private void pause() {
        playing = false;
        main.removeCallbacks(nextStep);
        updateControls();
    }
    private void updateControls() {
        if (play == null) return;
        play.setText(playing ? "Pause" : "Play");
        play.setEnabled(ready && !finished);
        singleStep.setEnabled(ready && !busy && !finished);
        restart.setEnabled(ready);
        source.setEnabled(!busy);
        image.selectionAllowed = ready && !busy && !playing && image.frameIndex < 0;
    }

    private void resetSequence(boolean startAfterReset) {
        if (closed) return;
        pause();
        final int request = ++generation;
        final boolean inputSynthetic = synthetic;
        final boolean updates = learning.isChecked();
        ready = false; busy = true; finished = false;
        positiveTotal = negativeTotal = 0; reinitializations = lostFrames = 0;
        status.setText("Preparing " + (inputSynthetic ? "synthetic fixture" : "raw DAVIS frames") + "…");
        updateControls();
        worker.execute(() -> {
            if (closed || request != generation) return;
            try {
                if (nativeHandle == 0) nativeHandle = NativeBridge.create();
                String rawPath = inputSynthetic ? "" : copyCamelInput().getAbsolutePath();
                NativeBridge.FrameResult result = NativeBridge.reset(nativeHandle, inputSynthetic, rawPath);
                if (closed || request != generation) return;
                File traces = new File(getFilesDir(), "runs");
                if (!traces.isDirectory() && !traces.mkdirs()) throw new IllegalStateException("cannot create trace directory");
                traceFile = new File(traces, (inputSynthetic ? "synthetic" : "camel") + "-learning-" + (updates ? "on" : "off") + ".jsonl");
                JSONObject begin = new JSONObject();
                begin.put("event", "reset"); begin.put("source", inputSynthetic ? "synthetic" : "camel");
                begin.put("learning_initial", updates); begin.put("initialization_always_trains", true);
                begin.put("width", result.width); begin.put("height", result.height); begin.put("frames", result.total);
                begin.put("implementation", "updated recovered TLD; custom LK + detector + ferns");
                writeTrace(begin, false);
                main.post(() -> {
                    if (closed || request != generation) return;
                    ready = true; busy = false;
                    display(result);
                    status.setText("Ready · select a region, play or step");
                    updateControls();
                    if (startAfterReset) { playing = true; updateControls(); advance(); }
                });
            } catch (Throwable error) { reportFailure(error, request); }
        });
    }

    private File copyCamelInput() throws Exception {
        File destination = new File(getFilesDir(), "camel-320x180.gray");
        // Assets are unannotated grayscale pixels; no tracking results are loaded.
        File temporary = new File(getFilesDir(), "camel-320x180.gray.tmp");
        try (InputStream in = getAssets().open("camel-320x180.gray");
             FileOutputStream out = new FileOutputStream(temporary)) {
            byte[] block = new byte[65536];
            int n;
            while ((n = in.read(block)) != -1) out.write(block, 0, n);
            out.getFD().sync();
        }
        if (temporary.length() != 90L * 320 * 180 || !temporary.renameTo(destination))
            throw new IllegalStateException("raw frame asset is incomplete");
        return destination;
    }

    private void advance() {
        if (closed || !ready || busy || finished) return;
        final int request = generation;
        final boolean updates = learning.isChecked();
        busy = true; updateControls();
        worker.execute(() -> {
            if (closed || request != generation) return;
            try {
                NativeBridge.FrameResult result = NativeBridge.step(nativeHandle, updates);
                if (closed || request != generation) return;
                recordFrame(result, updates);
                main.post(() -> {
                    if (closed || request != generation) return;
                    busy = false; finished = result.values[10] != 0;
                    positiveTotal += (long)result.values[11]; negativeTotal += (long)result.values[12];
                    if (result.values[9] != 0) reinitializations++;
                    if (result.values[8] == 0) lostFrames++;
                    display(result);
                    if (finished) {
                        pause();
                        status.setText("Complete · " + result.total + " frames computed by native TLD");
                    }
                    updateControls();
                    if (playing && !finished) main.postDelayed(nextStep, 80);
                });
            } catch (Throwable error) { reportFailure(error, request); }
        });
    }

    private void display(NativeBridge.FrameResult f) {
        image.setFrame(f);
        double[] m = f.values;
        String iou = m[4] < 0 ? (synthetic ? "hidden / n/a" : "not evaluated") : String.format(Locale.US, "%.4f", m[4]);
        status.setText((f.index < 0 ? "Preview" : "Frame " + f.index + " / " + (f.total - 1)) + " · "
                + (f.index < 0 ? "not processed" : m[8] != 0 ? "estimate present" : "target lost")
                + (m[9] != 0 ? " · detector reinitialized" : ""));
        metrics.setText(String.format(Locale.US,
                "Box  %.1f, %.1f  %.1f × %.1f\nCorr %.4f   confidence %.4f\nIoU  %s   compute %.1f ms\nOnline +%d / −%d (sum %d / %d)\nReinitializations %d   lost frames %d",
                m[0], m[1], m[2], m[3], m[5], m[6], iou, m[13], (long)m[11], (long)m[12],
                positiveTotal, negativeTotal, reinitializations, lostFrames));
        updateControls();
    }

    private void selectRegion(double x, double y, double w, double h) {
        if (!ready || busy || playing || image.frameIndex >= 0) return;
        final int request = generation;
        busy = true; updateControls();
        worker.execute(() -> {
            if (closed || request != generation) return;
            try {
                NativeBridge.FrameResult result = NativeBridge.select(nativeHandle, x, y, w, h);
                if (closed || request != generation) return;
                JSONObject selection = new JSONObject(); selection.put("event", "initial_selection");
                selection.put("box", new JSONArray(new double[]{x, y, w, h}));
                writeTrace(selection, true);
                main.post(() -> {
                    if (closed || request != generation) return;
                    busy = false; display(result); status.setText("Initial region selected · play or step");
                });
            } catch (Throwable error) {
                Log.e("PredatorRuntime", "selection rejected", error);
                main.post(() -> {
                    if (closed || request != generation) return;
                    busy = false; updateControls();
                    Toast.makeText(this, error.getMessage(), Toast.LENGTH_LONG).show();
                });
            }
        });
    }

    private void recordFrame(NativeBridge.FrameResult f, boolean updates) throws Exception {
        double[] m = f.values;
        JSONObject line = new JSONObject();
        line.put("event", "frame"); line.put("frame", f.index); line.put("total", f.total);
        line.put("learning", updates); line.put("valid", m[8] != 0); line.put("reinitialized", m[9] != 0);
        line.put("done", m[10] != 0); line.put("box", new JSONArray(new double[]{m[0], m[1], m[2], m[3]}));
        line.put("iou", m[4] < 0 ? JSONObject.NULL : m[4]);
        line.put("visible", f.total == 120 ? m[7] != 0 : JSONObject.NULL);
        line.put("correlation", m[5]); line.put("confidence", m[6]);
        line.put("positive_updates", (long)m[11]); line.put("negative_updates", (long)m[12]);
        line.put("processing_ms", m[13]);
        writeTrace(line, true);
        Log.i("PredatorRuntime", line.toString());
    }
    private void writeTrace(JSONObject value, boolean append) throws Exception {
        try (FileOutputStream out = new FileOutputStream(traceFile, append)) {
            out.write((value + "\n").getBytes(StandardCharsets.UTF_8));
        }
    }
    private void reportFailure(Throwable error, int request) {
        Log.e("PredatorRuntime", "native run failed", error);
        main.post(() -> {
            if (closed || request != generation) return;
            busy = false; ready = false; pause();
            status.setText("Run failed: " + error.getClass().getSimpleName() + " · " + error.getMessage());
            restart.setEnabled(true);
        });
    }

    private void showCredits() {
        String credits = "Updated Android frontend for Aaron Wetzler. It runs the recovered custom forward/backward "
                + "Lucas–Kanade tracker, scanning detector, fern classifier, correlation decisions and conditional online updates. "
                + "The frames are processed by JNI on this device; the application does not load precomputed tracking outputs.\n\n"
                + "Original student project: Marian Mansour and Majd Srour, based on Zdeněk Kalal’s TLD. "
                + "Recovered C++ source carries GPL-3.0-or-later notices. Original historical supervisor/UI credits remain in the repository.\n\n"
                + "Recorded DAVIS camel: Jordi Pont-Tuset, Federico Perazzi, Sergi Caelles, Pablo Arbeláez, "
                + "Alexander Sorkine-Hornung and Luc Van Gool. DAVIS dataset metadata and emirkisa’s MP4 mirror specify "
                + "CC BY-NC 4.0 (https://creativecommons.org/licenses/by-nc/4.0/). "
                + "https://davischallenge.org/ · https://huggingface.co/datasets/emirkisa/DAVIS-2017-480p-mp4\n"
                + "This copy is grayscale, 320×180, first 90 decoded frames. Source and recording terms are separate.\n\n"
                + "Generated frames are a controlled synthetic regression scene, not a benchmark. Recorded boxes have no loaded accuracy labels. "
                + "No camera input or general tracking-accuracy claim. Only the no-blur, one-session core is used. "
                + "Online-off still trains the initial classifier. Initialization is not included in online-update counters.\n\n"
                + "OpenCV 4 provides core/image-processing primitives. Its notices, dependency licenses and Gradle wrapper "
                + "Apache license/notice are bundled under assets/licenses. Source: https://github.com/twerdster/predator-tld-ios";
        new AlertDialog.Builder(this).setTitle("Credits and validation scope").setMessage(credits)
                .setPositiveButton("Close", null).setNeutralButton("Bundled licenses", (d, which) -> showLicenseIndex()).show();
    }
    private void showLicenseIndex() {
        try {
            java.util.ArrayList<String> names = new java.util.ArrayList<>();
            collectAssets("licenses", names);
            new AlertDialog.Builder(this).setTitle("Bundled license and provenance files")
                    .setItems(names.toArray(new String[0]), (d, which) -> {
                        try (InputStream in = getAssets().open(names.get(which))) {
                            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                            byte[] block = new byte[8192]; int n;
                            while ((n = in.read(block)) != -1) bytes.write(block, 0, n);
                            String contents = bytes.toString(StandardCharsets.UTF_8.name());
                            TextView text = text(contents, 12, Color.DKGRAY);
                            text.setTextIsSelectable(true);
                            ScrollView scroll = new ScrollView(this); scroll.addView(text);
                            new AlertDialog.Builder(this).setTitle(names.get(which)).setView(scroll)
                                    .setPositiveButton("Close", null).show();
                        } catch (Exception e) { Toast.makeText(this, e.toString(), Toast.LENGTH_LONG).show(); }
                    }).setPositiveButton("Close", null).show();
        } catch (Exception e) { Toast.makeText(this, e.toString(), Toast.LENGTH_LONG).show(); }
    }
    private void collectAssets(String path, java.util.List<String> names) throws Exception {
        String[] children = getAssets().list(path);
        if (children == null || children.length == 0) { names.add(path); return; }
        for (String child : children) collectAssets(path + "/" + child, names);
    }

    @Override protected void onPause() { pause(); super.onPause(); }
    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        synthetic = !"camel".equals(intent.getStringExtra("source"));
        lastSource = synthetic ? 0 : 1;
        source.setSelection(lastSource);
        learning.setChecked(intent.getBooleanExtra("learning", true));
        resetSequence(intent.getBooleanExtra("autoRun", false));
    }
    @Override protected void onDestroy() {
        closed = true; ++generation; main.removeCallbacksAndMessages(null);
        worker.execute(() -> {
            long closingHandle = nativeHandle;
            nativeHandle = 0;
            if (closingHandle != 0) {
                try { NativeBridge.destroy(closingHandle); }
                catch (Throwable error) { Log.e("PredatorRuntime", "native cleanup failed", error); }
            }
        });
        // Keep the process-wide FIFO alive for a replacement Activity.
        super.onDestroy();
    }

    private final class FrameView extends View {
        private Bitmap bitmap;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF destination = new RectF();
        private boolean selectionAllowed, dragging;
        private float startX, startY, endX, endY;
        private int frameIndex = -1;
        FrameView() { super(MainActivity.this); setFocusable(true); }
        void setFrame(NativeBridge.FrameResult result) {
            if (bitmap == null || bitmap.getWidth() != result.width || bitmap.getHeight() != result.height) {
                bitmap = Bitmap.createBitmap(result.width, result.height, Bitmap.Config.ARGB_8888);
                requestLayout();
            }
            bitmap.setPixels(result.pixels, 0, result.width, 0, 0, result.width, result.height);
            frameIndex = result.index; invalidate();
        }
        @Override protected void onMeasure(int width, int height) {
            int w = MeasureSpec.getSize(width);
            float ratio = bitmap == null ? .75f : (float)bitmap.getHeight() / bitmap.getWidth();
            setMeasuredDimension(w, Math.round(w * ratio));
        }
        @Override protected void onDraw(Canvas canvas) {
            canvas.drawColor(Color.rgb(23, 35, 41));
            if (bitmap == null) return;
            destination.set(0, 0, getWidth(), getHeight());
            paint.setFilterBitmap(false);
            canvas.drawBitmap(bitmap, null, destination, paint);
            if (dragging) {
                paint.setColor(Color.YELLOW); paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(dp(2));
                canvas.drawRect(Math.min(startX, endX), Math.min(startY, endY), Math.max(startX, endX), Math.max(startY, endY), paint);
                paint.setStyle(Paint.Style.FILL);
            }
        }
        @Override public boolean onTouchEvent(MotionEvent event) {
            if (!selectionAllowed || bitmap == null) return false;
            float x = Math.max(0, Math.min(getWidth(), event.getX()));
            float y = Math.max(0, Math.min(getHeight(), event.getY()));
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    startX = endX = x; startY = endY = y; dragging = true;
                    getParent().requestDisallowInterceptTouchEvent(true); invalidate(); return true;
                case MotionEvent.ACTION_MOVE:
                    endX = x; endY = y; invalidate(); return true;
                case MotionEvent.ACTION_UP:
                    endX = x; endY = y; dragging = false; invalidate(); performClick();
                    getParent().requestDisallowInterceptTouchEvent(false);
                    double scaleX = (double)bitmap.getWidth() / getWidth(), scaleY = (double)bitmap.getHeight() / getHeight();
                    double w = Math.abs(endX - startX) * scaleX, h = Math.abs(endY - startY) * scaleY;
                    if (w >= 15 && h >= 15) selectRegion(Math.min(startX, endX) * scaleX, Math.min(startY, endY) * scaleY, w, h);
                    else Toast.makeText(MainActivity.this, "Select at least 15×15 image pixels", Toast.LENGTH_SHORT).show();
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    dragging = false; invalidate(); getParent().requestDisallowInterceptTouchEvent(false); return true;
                default: return false;
            }
        }
        @Override public boolean performClick() { super.performClick(); return true; }
    }
}
