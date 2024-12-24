#!/bin/bash -x

SCRIPT=$(realpath -s "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd "$SCRIPTPATH"

# Add emscripten environment variables
source emsdk/emsdk_env.sh

emcc -o jolly.html src/main.c -O2 -Wall raylib/src/libraylib.a \
  -I. -Iraylib/src/ -L. -Lraylib/src/ -s USE_GLFW=3 -s ASYNCIFY \
  --shell-file minshell.html -DPLATFORM_WEB \
  -s EXPORTED_RUNTIME_METHODS=['setValue'] -lidbfs.js

mv jolly.html index.html
zip index.zip index.html jolly.js jolly.wasm
