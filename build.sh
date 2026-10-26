#!/bin/sh
set -eu
cd "$(dirname "$0")"
repo_root=$(pwd)
docker run --rm --user 0 -v "$repo_root:/workspace" --entrypoint /bin/bash   xiashj/logue-sdk@sha256:e4d85a16c38dc4d34b0e93cabb6378df21729688dbcd88808c84e8d41aa0c9ba -lc   'set -e; source /app/drumlogue/environment; cd /workspace/lpc-robot; make; make install'
mkdir -p dist
cp lpc-robot/lpc_robot7.drmlgunit dist/lpc_robot7.drmlgunit
sha256sum dist/lpc_robot7.drmlgunit
