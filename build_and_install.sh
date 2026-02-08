#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
STEAM_APPS="$HOME/Library/Application Support/Steam/steamapps/common"

ETS_PLUGIN_DIR="$STEAM_APPS/Euro Truck Simulator 2/Euro Truck Simulator 2.app/Contents/MacOS/plugins"
ATS_PLUGIN_DIR="$STEAM_APPS/American Truck Simulator/American Truck Simulator.app/Contents/MacOS/plugins"

GAME="ets"

for arg in "$@"; do
        case "$arg" in
                --ets) GAME="ets" ;;
                --ats) GAME="ats" ;;
                *)
                        echo "usage: $0 [--ets | --ats]"
                        exit 1
                        ;;
        esac
done

if [ "$GAME" = "ats" ]; then
        PLUGIN_DIR="$ATS_PLUGIN_DIR"
        GAME_NAME="American Truck Simulator"
else
        PLUGIN_DIR="$ETS_PLUGIN_DIR"
        GAME_NAME="Euro Truck Simulator 2"
fi

echo "==> Cleaning build..."
cmake --build "$BUILD_DIR" --target clean

echo "==> Building..."
cmake --build "$BUILD_DIR"

mkdir -p "$PLUGIN_DIR"

echo "==> Copying libfffb.dylib to $GAME_NAME plugins directory..."
cp "$BUILD_DIR/libfffb.dylib" "$PLUGIN_DIR/libfffb.dylib"

echo "==> Done!"
