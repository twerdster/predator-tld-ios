#!/usr/bin/env python3
"""Run the recovered C++ pipeline and encode its actual annotated frames.
FFmpeg handles decoding/encoding; OpenCV supplies only core/image primitives.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
g = p.add_mutually_exclusive_group(required=True)
g.add_argument('--synthetic', action='store_true')
g.add_argument('--video', type=pathlib.Path)
p.add_argument('--roi', help='x,y,width,height after resizing input to 320 pixels wide')
p.add_argument('--frames', default=120, type=int)
p.add_argument('--learning', default=1, choices=(0, 1), type=int)
p.add_argument('--fps', type=float, help='presentation frame rate; default 15 synthetic / 24 recorded')
a = p.parse_args()
if a.video and not a.roi:
    p.error('--video requires --roi')
if a.output.exists() and any(a.output.iterdir()):
    p.error('--output must be a new or empty directory, to exclude stale frames')
a.output.mkdir(parents=True, exist_ok=True)
exe = a.build_dir.resolve() / 'predator_demo'
cmd = [str(exe), '--output', str(a.output.resolve()), '--frames', str(a.frames), '--learning', str(a.learning)]
provenance = {'synthetic': a.synthetic, 'commands': [], 'source_commit': 'a63f005bf34b041ff0dd985a8e18083f2d3065eb'}

def run(command, log=None):
    provenance['commands'].append(command)
    if log:
        with log.open('w') as f:
            subprocess.run(command, stdout=f, stderr=subprocess.STDOUT, check=True)
    else:
        subprocess.run(command, check=True)

if a.synthetic:
    cmd.append('--synthetic')
else:
    raw = a.video.resolve()
    provenance['input_sha256'] = hashlib.sha256(raw.read_bytes()).hexdigest()
    frames = a.output / 'decoded'
    frames.mkdir(exist_ok=True)
    run(['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error', '-i', str(raw), '-vf', 'scale=320:-2', '-pix_fmt', 'gray', '-frames:v', str(a.frames), str(frames / '%05d.pgm')])
    cmd += ['--frames-dir', str(frames.resolve()), '--roi', a.roi]
run(cmd, a.output / 'run.log')
fps = a.fps or (15 if a.synthetic else 24)
input_args = ['ffmpeg', '-y', '-hide_banner', '-loglevel', 'error', '-framerate', str(fps), '-i', str(a.output / '%05d.ppm')]
run(input_args + ['-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '23', '-movflags', '+faststart', str(a.output / 'demo.mp4')])
run(input_args + ['-vf', 'fps=8,scale=280:-2,split[a][b];[a]palettegen=max_colors=48[p];[b][p]paletteuse=dither=bayer', str(a.output / 'demo.gif')])
for name, cap in [('demo.gif', 2_000_000), ('demo.mp4', 10_000_000)]:
    f = a.output / name
    if f.stat().st_size >= cap:
        raise RuntimeError(f'{name} exceeds the publication size cap; shorten the demonstration')
    provenance[name] = {'bytes': f.stat().st_size, 'sha256': hashlib.sha256(f.read_bytes()).hexdigest()}
(a.output / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
print((a.output / 'metrics.json').read_text())
