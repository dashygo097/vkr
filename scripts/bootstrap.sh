#!/bin/bash
set -euo pipefail

BASE_DIR=$(dirname $(cd "$(dirname "$0")" && pwd))
BUILD_DIR=${BASE_DIR}/build

# Build script for a CMake project
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake $BASE_DIR
cd $BASE_DIR

# Compile shader files
GLSL_COMPILER=$(which glslc)
SHADER_SRC_ROOT="${BASE_DIR}/shaders"
SHADER_OUT_ROOT="${BUILD_DIR}/outputs/demos/shaders"

[[ -x $GLSL_COMPILER ]] || { echo >&2 "[ERROR] glslc not found"; exit 1; }
echo "[INFO] Compiling shaders with glslc at $GLSL_COMPILER"

find "$SHADER_SRC_ROOT" -type f \( -name "*.vert" -o -name "*.frag" \) | while read -r src; do
  rel="${src#$SHADER_SRC_ROOT/}"              
  base="${rel%.*}"                     
  ext="${rel##*.}"                  

  out_dir="$SHADER_OUT_ROOT/$(dirname "$rel")"
  mkdir -p "$out_dir"

  out="$out_dir/${ext}.spv"           
  echo "[INFO] Compiling $src → $out"
  "$GLSL_COMPILER" -o "$out" "$src"
done
