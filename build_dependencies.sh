#!/bin/bash -x

SCRIPT=$(realpath -s "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd "$SCRIPTPATH"
# Download emscripten 3.1.74
git clone --branch 3.1.74 https://github.com/emscripten-core/emsdk.git
cd emsdk
# Download and install the SDK tools.
./emsdk install 3.1.74
# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate 3.1.74
# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh

cd "$SCRIPTPATH"
# Download raylib 5.5
git clone --branch 5.5 git@github.com:raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_WEB \
  EMSDK_PATH="$SCRIPTPATH/emsdk" \
  EMSCRIPTEN_PATH="$SCRIPTPATH/emsdk/3.1.74/emscripten" \
  CLANG_PATH="$SCRIPTPATH/emsdk/3.1.74/bin"
