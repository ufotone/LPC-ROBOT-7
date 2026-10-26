#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p .test-output
flags="-O2"
if [ "${SANITIZE:-0}" = "1" ]; then
  flags="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all"
fi
# Intentional splitting: flags contains compiler options controlled by this script.
gcc -std=c11 $flags -fPIC -Icommon -c lpc-robot/header.c -o .test-output/header.o
g++ -std=c++14 $flags -fPIC -shared -Icommon -Ilpc-robot lpc-robot/unit.cc .test-output/header.o -o .test-output/lpc_robot_host.so
g++ -std=c++14 $flags -Icommon -Ilpc-robot lpc-robot/test_unit.cc -ldl -o .test-output/test_host
.test-output/test_host ./.test-output/lpc_robot_host.so .test-output
g++ -std=c++14 $flags -Icommon -Ilpc-robot tests/safety_test.cc -o .test-output/test_safety
.test-output/test_safety
