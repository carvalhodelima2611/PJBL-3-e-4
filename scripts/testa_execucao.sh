#!/usr/bin/env bash
# Compila e executa cada programa de um diretorio de testes, comparando a saida
# com o arquivo .esperado ao lado. Cada programa roda de DUAS formas:
#   1) lli (interpretador/JIT do LLVM), com o runtime ligado como bitcode
#   2) executavel nativo: clang saida.ll zl_runtime_posix.c -o prog
# Antes, a IR passa por llvm-as e opt -passes=verify.
# Se existir NOME.entrada, ela vai para a entrada padrao. A ultima linha da
# saida comparada e "[codigo de saida: N]".
# Uso: scripts/testa_execucao.sh DIRETORIO [DIRETORIO...]
set -u
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
COMP="$RAIZ/macaronica"
B="$RAIZ/build/testes"
mkdir -p "$B"
LLVM_BIN="${LLVM_BIN:-$(llvm-config-18 --bindir 2>/dev/null || dirname "$(command -v opt)")}"
export PATH="$LLVM_BIN:$PATH"

clang -O1 -c "$RAIZ/runtime/zl_runtime_posix.c" -o "$B/rt.o" || exit 1
clang -O1 -S -emit-llvm "$RAIZ/runtime/zl_runtime_posix.c" -o "$B/rt.ll" || exit 1

roda() {   # roda um comando com a entrada certa e anexa o codigo de saida
    local entrada="$1"; shift
    if [ -f "$entrada" ]; then "$@" < "$entrada" 2>/dev/null; else "$@" < /dev/null 2>/dev/null; fi
    echo "[codigo de saida: $?]"
}

ok=0; falhou=0
for dir in "$@"; do
  for f in "$dir"/*.mac; do
    n="$(basename "$f" .mac)"
    esp="$dir/$n.esperado"
    [ -f "$esp" ] || continue
    ll="$B/$n.ll"; mlir="$B/$n.mlir"
    if ! "$COMP" "$f" -o "$ll" > "$B/$n.compilacao" 2>&1; then
        echo "  [FALHOU] $f (nao compilou)"; cat "$B/$n.compilacao"; falhou=$((falhou+1)); continue
    fi
    if ! llvm-as "$ll" -o /dev/null || ! opt -passes=verify "$ll" -o /dev/null; then
        echo "  [FALHOU] $f (IR rejeitada por llvm-as/verify)"; falhou=$((falhou+1)); continue
    fi
    prog_ll="$ll"
    if [ -f "$mlir" ]; then                       # tem nucleos vetoriais: rebaixa e junta
        if ! "$RAIZ/scripts/pipeline.sh" "$mlir" "$B/$n.nucleos.ll" > /dev/null 2> "$B/$n.pipeline.err"; then
            echo "  [FALHOU] $f (pipeline MLIR)"; cat "$B/$n.pipeline.err"; falhou=$((falhou+1)); continue
        fi
        llvm-link -S "$ll" "$B/$n.nucleos.ll" -o "$B/$n.junto.ll" || { falhou=$((falhou+1)); continue; }
        prog_ll="$B/$n.junto.ll"
    fi
    llvm-link "$prog_ll" "$B/rt.ll" -o "$B/$n.bc" 2>/dev/null
    roda "$dir/$n.entrada" lli "$B/$n.bc" > "$B/$n.saida.lli"
    clang -Wno-override-module -O0 "$prog_ll" "$B/rt.o" -lm -o "$B/$n.exe" 2>/dev/null
    roda "$dir/$n.entrada" "$B/$n.exe" > "$B/$n.saida.nativo"
    if diff -q "$esp" "$B/$n.saida.lli" > /dev/null && diff -q "$esp" "$B/$n.saida.nativo" > /dev/null; then
        echo "  [OK]     $f"; ok=$((ok+1))
    else
        echo "  [FALHOU] $f (saida diferente da esperada)"
        diff "$esp" "$B/$n.saida.lli" | head -10 | sed 's/^/           lli:    /'
        diff "$esp" "$B/$n.saida.nativo" | head -10 | sed 's/^/           nativo: /'
        falhou=$((falhou+1))
    fi
  done
done
echo "  -> $ok passaram, $falhou falharam"
[ "$falhou" -eq 0 ]
