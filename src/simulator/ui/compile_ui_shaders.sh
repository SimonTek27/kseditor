#!/usr/bin/env bash
# Compile UI GLSL → SPIR-V (requires glslangValidator or glslc)
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="${DIR}/shaders"
mkdir -p "$OUT"

if command -v glslc >/dev/null 2>&1; then
  glslc -fshader-stage=vert "$DIR/shaders/ui.vert.glsl" -o "$OUT/ui.vert.spv"
  glslc -fshader-stage=frag "$DIR/shaders/ui.frag.glsl" -o "$OUT/ui.frag.spv"
elif command -v glslangValidator >/dev/null 2>&1; then
  glslangValidator -V "$DIR/shaders/ui.vert.glsl" -o "$OUT/ui.vert.spv"
  glslangValidator -V "$DIR/shaders/ui.frag.glsl" -o "$OUT/ui.frag.spv"
else
  echo "Install glslc (shaderc) or glslangValidator" >&2
  exit 1
fi
echo "Wrote $OUT/ui.vert.spv and $OUT/ui.frag.spv"
