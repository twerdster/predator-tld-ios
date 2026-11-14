#!/usr/bin/env python3
"""Native Tk frontend: ctypes invokes recovered TLD C++ on every displayed frame.
No saved tracking annotations or result movies are read. Requires tkinter only.
"""
import argparse
import ctypes
import json
import platform
import os
from pathlib import Path
import tkinter as tk
from tkinter import ttk

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--library', required=True, type=Path)
p.add_argument('--raw', type=Path, default=Path(__file__).parents[1] / 'data/camel-320x180.gray')
p.add_argument('--source', choices=['synthetic', 'camel'], default='synthetic')
p.add_argument('--learning', type=int, choices=[0, 1], default=1)
p.add_argument('--autoplay', action='store_true')
p.add_argument('--start-delay-ms', type=int, default=500)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
lib = ctypes.CDLL(str(a.library.resolve()))
lib.predator_create.restype = ctypes.c_void_p
lib.predator_destroy.argtypes = [ctypes.c_void_p]
lib.predator_error.argtypes = [ctypes.c_void_p]
lib.predator_error.restype = ctypes.c_char_p
lib.predator_reset.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p]
lib.predator_step.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.predator_frame_json.argtypes = [ctypes.c_void_p]
lib.predator_frame_json.restype = ctypes.c_char_p
lib.predator_copy_rgba.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
handle = lib.predator_create()
if not handle:
    raise RuntimeError('could not create native TLD session')
root = tk.Tk()
platform_name = 'macOS' if platform.system() == 'Darwin' else platform.system()
root.title(f'Predator · updated native TLD on {platform_name}')
root.geometry('1080x730+20+20')
source = tk.StringVar(value=a.source)
learning = tk.BooleanVar(value=bool(a.learning))
running = False
ready = False
live_source = None
frames = []
image_reference = None

shell = ttk.Frame(root, padding=18)
shell.pack(fill='both', expand=True)
ttk.Label(shell, text='Predator · updated native', font=('Sans', 23, 'bold')).pack(anchor='w')
ttk.Label(shell, text='Recovered custom TLD · computed locally by C++ / OpenCV core + imgproc').pack(anchor='w', pady=(0, 10))
columns = ttk.Frame(shell)
columns.pack(fill='both', expand=True)
image = ttk.Label(columns)
image.pack(side='left', anchor='n', padx=(0, 20))
controls = ttk.Frame(columns)
controls.pack(side='left', fill='both', expand=True)
status = tk.StringVar(value='Preparing native input…')
detail = tk.StringVar()
ttk.Label(controls, textvariable=status, font=('Monospace', 13, 'bold'), wraplength=330).pack(anchor='w', pady=(8, 14))
ttk.Label(controls, textvariable=detail, font=('Monospace', 11), justify='left').pack(anchor='w', pady=(0, 18))
ttk.Label(controls, text='Input').pack(anchor='w')
selector = ttk.Combobox(controls, textvariable=source, values=['synthetic', 'camel'], state='readonly')
selector.pack(anchor='w', fill='x', pady=(4, 10))
ttk.Checkbutton(controls, text='Online updates (initialization always trains)', variable=learning).pack(anchor='w', pady=8)
buttons = ttk.Frame(controls)
buttons.pack(anchor='w', pady=10)
play = ttk.Button(buttons, text='Run')
play.pack(side='left', padx=(0, 7))

def display():
    global image_reference
    f = json.loads(lib.predator_frame_json(handle))
    size = f['width'] * f['height'] * 4
    pixels = (ctypes.c_ubyte * size)()
    if lib.predator_copy_rgba(handle, pixels, size) != size:
        raise RuntimeError('native RGBA copy failed')
    rgba = bytes(pixels)
    rgb = bytearray(f['width'] * f['height'] * 3)
    rgb[0::3] = rgba[0::4]
    rgb[1::3] = rgba[1::4]
    rgb[2::3] = rgba[2::4]
    ppm = f"P6\n{f['width']} {f['height']}\n255\n".encode() + rgb
    image_reference = tk.PhotoImage(data=ppm, format='PPM').zoom(2, 2)
    image.configure(image=image_reference)
    status.set(f"{f['input']}\nFrame {f['frame'] + 1}/{f['total']} · " + ('DONE' if f['done'] else 'ready' if f['frame'] < 0 else 'estimate' if f['valid'] else 'target lost'))
    box = ', '.join(f'{n:.1f}' for n in f['box'])
    detail.set(f"box {box}\nconfidence {f['confidence']:.3f}\ncorrelation {f['correlation']:.3f}\nIoU {'unmeasured' if f['iou'] is None else format(f['iou'], '.3f')}\ndetector reinit {f['reinitialized']}\nonline +{f['positive_updates']} / −{f['negative_updates']}\ncompute {f['processing_ms']:.1f} ms")
    return f

def reset(event=None):
    global running, frames, ready, live_source
    running = False
    ready = False
    play.configure(text='Run')
    if not lib.predator_reset(handle, source.get() == 'synthetic', str(a.raw.resolve()).encode()):
        status.set(lib.predator_error(handle).decode())
        return False
    frames = []
    (a.output / 'native-metrics.json').unlink(missing_ok=True)
    (a.output / 'native-frames.json').write_text('[]\n')
    frame = display()
    live_source = 'synthetic' if frame['input'].startswith('Synthetic') else 'camel'
    ready = True
    return True

def step():
    global running
    if not ready:
        return
    result = lib.predator_step(handle, learning.get())
    if result < 0:
        running = False
        status.set(lib.predator_error(handle).decode())
        return
    f = display()
    if result:
        f['learning_enabled'] = learning.get()
        frames.append(f)
        (a.output / 'native-frames.json').write_text(json.dumps(frames, indent=2) + '\n')
    if result and f['done']:
        running = False
        play.configure(text='Done')
        visible = [x['iou'] for x in frames if x['visible']]
        modes = {x['learning_enabled'] for x in frames}
        summary = {'platform': platform_name + ' Tk / recovered native C++', 'source': live_source, 'input': f['input'], 'frames': len(frames),
                   'learning_mode': 'mixed' if len(modes)>1 else 'on' if True in modes else 'off',
                   'mean_visible_iou': sum(visible) / len(visible) if visible else None,
                   'visible_iou_at_least_half': sum(x >= .5 for x in visible) if visible else None,
                   'lost_frames': sum(not x['valid'] for x in frames),
                   'reinitializations': sum(x['reinitialized'] for x in frames),
                   'positive_updates': sum(x['positive_updates'] for x in frames),
                   'negative_updates': sum(x['negative_updates'] for x in frames)}
        (a.output / 'native-metrics.json').write_text(json.dumps(summary, indent=2) + '\n')
        print('PREDATOR_NATIVE_COMPLETE', json.dumps(summary), flush=True)

def toggle():
    global running
    if not ready:
        return
    running = not running
    play.configure(text='Pause' if running else 'Run')

def tick():
    if running:
        step()
    root.after(100, tick)

play.configure(command=toggle)
ttk.Button(buttons, text='Step', command=step).pack(side='left', padx=7)
ttk.Button(buttons, text='Restart', command=reset).pack(side='left', padx=7)
selector.bind('<<ComboboxSelected>>', reset)
ttk.Label(controls, text='Green: synthetic truth\nRed: current native estimate\nYellow: estimated-center path\n\nRecorded input is unannotated.\nNo recorded-video accuracy score.', justify='left').pack(anchor='w', pady=15)
ttk.Label(shell, text='Marian Mansour / Majd Srour · Kalal TLD (GPL)   |   DAVIS / emirkisa mirror · CC BY-NC 4.0   |   Updated frontend for Aaron Wetzler', wraplength=1030).pack(anchor='w', pady=(15, 0))
initial_ready = reset()
root.after(150, tick)
if a.autoplay and initial_ready:
    root.after(a.start_delay_ms, toggle)
root.update_idletasks()
(a.output / 'window.json').write_text(json.dumps({'pid': os.getpid(), 'window_id': root.winfo_id(), 'x': root.winfo_rootx(), 'y': root.winfo_rooty(), 'width': root.winfo_width(), 'height': root.winfo_height(), 'platform': platform_name}, indent=2) + '\n')
try:
    root.mainloop()
finally:
    lib.predator_destroy(handle)
