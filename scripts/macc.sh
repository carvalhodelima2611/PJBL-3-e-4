#!/usr/bin/env bash
# macc.sh - caminho completo de um programa Macaronica ate um executavel no PC:
#   .mac --(macaronica)--> .ll [+ .mlir] --(pipeline.sh + llvm-link)--> .ll --(clang + runtime)--> executavel
# Uso: scripts/macc.sh programa.mac saida [modo_pipeline]
#      modo_pipeline: base (padrao), vetor ou paralelo (ver scripts/pipeline.sh)
set -euo pipefail
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
FONTE="$1"; SAIDA="$2"; MODO="${3:-base}"
LLVM_BIN="${LLVM_BIN:-$(llvm-config-18 --bindir 2>/dev/null || echo /usr/lib/llvm-18/bin)}"
export PATH="$LLVM_BIN:$PATH"
BASE="${SAIDA%.*}"
"$RAIZ/macaronica" "$FONTE" -o "$BASE.ll" > /dev/null || { "$RAIZ/macaronica" "$FONTE" >&2 || true; exit 1; }
PROG="$BASE.ll"
if [ -f "$BASE.mlir" ]; then
    "$RAIZ/scripts/pipeline.sh" "$BASE.mlir" "$BASE.nucleos.ll" "$MODO" > /dev/null
    llvm-link -S "$BASE.ll" "$BASE.nucleos.ll" -o "$BASE.junto.ll"
    PROG="$BASE.junto.ll"
fi
OMP=""; [ "$MODO" = paralelo ] && OMP="-fopenmp"
clang -Wno-override-module -O2 $OMP "$PROG" "$RAIZ/runtime/zl_runtime_posix.c" -lm -o "$SAIDA"
