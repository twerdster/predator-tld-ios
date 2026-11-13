#!/usr/bin/env python3
"""Compile unchanged original components and verify three negative controls.
Public OpenCV 4 headers provide removed umbrella/type declarations. Original
source bytes are unchanged. Build failures are errors, never defect evidence.
"""
import argparse
import json
import pathlib
import shlex
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--work-dir', required=True, type=pathlib.Path)
p.add_argument('--opencv-prefix', type=pathlib.Path)
p.add_argument('--cxx', default=shutil.which('clang++') or 'g++')
a = p.parse_args()
a.work_dir.mkdir(parents=True, exist_ok=True)
r = pathlib.Path(__file__).resolve().parents[2]
shim = a.work_dir / 'compat/opencv2'
shim.mkdir(parents=True, exist_ok=True)
(shim / 'opencv.hpp').write_text('#include <opencv2/core.hpp>\n#include <opencv2/imgproc.hpp>\n')
if a.opencv_prefix:
    flags = ['-I' + str(a.opencv_prefix / 'include/opencv4')]
    libdir = a.opencv_prefix / 'lib/x86_64-linux-gnu'
else:
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'opencv4'], text=True))
    libdir = subprocess.check_output(['pkg-config', '--variable=libdir', 'opencv4'], text=True).strip()
original_files = ['Classifier.cpp', 'Fern.cpp', 'PointTest.cpp', 'TldImage.cpp']
base = [a.cxx, '-std=c++17', '-fno-omit-frame-pointer', '-g', '-I' + str(a.work_dir / 'compat'), *flags, '-I' + str(r / 'source'),
        '-I' + str(r / 'source/Predator'), '-include', 'opencv2/core/core_c.h',
        '-include', 'opencv2/imgproc/imgproc_c.h',
        *[str(r / 'source' / x) for x in original_files],
        str(r / 'updated/tests/original_witness.cpp'), '-L' + str(libdir),
        '-lopencv_core', '-lopencv_imgproc']
executables = {}
for kind in ['plain', 'asan']:
    executable = a.work_dir / ('original_witness_' + kind)
    command = base + (['-fsanitize=address,undefined'] if kind == 'asan' else []) + ['-o', str(executable)]
    built = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (a.work_dir / ('build-' + kind + '.log')).write_text(built.stdout)
    built.check_returncode()
    executables[kind] = executable
results = []
for case in ['negative', 'borrowed', 'endpoint']:
    executable = executables['plain' if case == 'negative' else 'asan']
    tested = subprocess.run([str(executable), case], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (a.work_dir / (case + '.log')).write_text(tested.stdout)
    if case == 'negative':
        observed = tested.returncode == 1 and 'after negative update=1' in tested.stdout
    elif case == 'borrowed':
        observed = tested.returncode != 0 and 'AddressSanitizer' in tested.stdout and any(x in tested.stdout for x in ['double-free', 'alloc-dealloc-mismatch'])
    else:
        observed = tested.returncode != 0 and 'heap-buffer-overflow' in tested.stdout
    result = {'case': case, 'returncode': tested.returncode, 'expected_diagnostic_observed': observed}
    results.append(result)
    print(result, flush=True)
    if not observed:
        raise RuntimeError(tested.stdout)
(a.work_dir / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
