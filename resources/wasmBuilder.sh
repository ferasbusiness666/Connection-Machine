#!/usr/bin/env bash

set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source> <target>" >&2
    exit 1
fi

source="$1"
target="$2"

# Get file extension
source_ext="${source##*.}"
target_ext="${target##*.}"

# Validate extensions
case "$source_ext" in
    zig|rs|c|cpp|cc) ;;
    *)
        echo "Error: Source file must have extension .zig, .rs, .c, .cpp, or .cc" >&2
        exit 1
        ;;
esac

if [ "$target_ext" != "wasm" ]; then
    echo "Error: Target file must have .wasm extension" >&2
    exit 1
fi

# Get source directory and change to it
source_dir="$(dirname "$source")"
cd "$source_dir"

# Build based on extension
case "$source_ext" in
    zig)
        zig build-exe "$source" -target wasm32-freestanding -fno-entry -rdynamic -O ReleaseSmall -femit-bin="$target"
        ;;
    rs)
        echo "Error: Rust builds are currently disabled" >&2
        exit 1
        ;;
    c|cpp|cc)
        emcc "$source" -Os -flto -s STANDALONE_WASM -s EXPORTED_FUNCTIONS="['_generateCircuit', '_getUUID', '_getName', '_getDefaultParameters']" --no-entry -o "$target"
        ;;
esac
