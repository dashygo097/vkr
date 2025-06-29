#!/bin/bash

CURRENT_DIR=$(pwd)
BUILD_DIR="build"

# Build script for a CMake project
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake $CURRENT_DIR
cd $CURRENT_DIR

# Compile shader files
GLSL_COMPILER=$(which glslc)
echo "[INFO] Compiling shaders with glslc at $GLSL_COMPILER"

mkdir -p build/shaders
$GLSL_COMPILER -o $BUILD_DIR/shaders/vert.spv $CURRENT_DIR/shaders/shader.vert
$GLSL_COMPILER -o $BUILD_DIR/shaders/frag.spv $CURRENT_DIR/shaders/shader.frag

