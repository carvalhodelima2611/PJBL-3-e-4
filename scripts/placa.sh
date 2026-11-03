#!/usr/bin/env bash
# placa.sh - gera o executavel da placa a partir de um programa Macaronica (make placa).
#   uno : .mac -> .ll (--alvo=uno) -> llc -mtriple=avr -mcpu=atmega328p -> .o
#         + runtime/zl_runtime_avr.c (avr-gcc) -> .elf -> placa/<nome>.hex
#   rpi : .mac -> .ll -> clang --target=aarch64-linux-gnu + runtime POSIX -> placa/<nome>_rpi (ELF ARM64)
# Uso: scripts/placa.sh programa.mac [uno|rpi] [-DZL_SIMULADOR]
# O .ll e o MESMO do PC (o --alvo=uno so liga a verificacao de memoria e rebaixa duplocarpado).
set -euo pipefail
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
FONTE="$1"; PLACA="${2:-uno}"; EXTRA="${3:-}"
LLVM_BIN="${LLVM_BIN:-$(llvm-config-18 --bindir 2>/dev/null || echo /usr/lib/llvm-18/bin)}"
export PATH="$LLVM_BIN:$PATH"
NOME="$(basename "$FONTE" .mac)"
B="$RAIZ/build/placa/$PLACA"; mkdir -p "$B" "$RAIZ/placa"
ALVO="--alvo=$PLACA"
"$RAIZ/macaronica" "$FONTE" -o "$B/$NOME.ll" $ALVO > "$B/$NOME.compilacao" 2>&1 || { cat "$B/$NOME.compilacao" >&2; exit 1; }
PROG="$B/$NOME.ll"
if [ -f "$B/$NOME.mlir" ]; then              # nucleos vetoriais tambem vao para a placa
    "$RAIZ/scripts/pipeline.sh" "$B/$NOME.mlir" "$B/$NOME.nucleos.ll" base > /dev/null
    llvm-link -S "$B/$NOME.ll" "$B/$NOME.nucleos.ll" -o "$B/$NOME.junto.ll"
    PROG="$B/$NOME.junto.ll"
fi
case "$PLACA" in
  uno)
    MCU=atmega328p
    llc -mtriple=avr -mcpu=$MCU -O2 -filetype=obj "$PROG" -o "$B/$NOME.o"
    avr-gcc -mmcu=$MCU -Os -DF_CPU=16000000UL $EXTRA -I"$RAIZ/runtime" \
        "$RAIZ/runtime/zl_runtime_avr.c" "$B/$NOME.o" -lm -o "$B/$NOME.elf"
    avr-objcopy -O ihex -R .eeprom "$B/$NOME.elf" "$B/$NOME.hex"
    if [ -z "$EXTRA" ]; then                 # so a versao da placa de verdade vai para placa/
        cp "$B/$NOME.hex" "$B/$NOME.elf" "$RAIZ/placa/"
    fi
    avr-size --mcu=$MCU -C "$B/$NOME.elf" | grep -E "Program|Data"
    if [ -z "$EXTRA" ]; then echo "-> placa/$NOME.hex   (gravar: avrdude -p m328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:placa/$NOME.hex)"; fi
    ;;
  rpi)
    clang --target=aarch64-linux-gnu -O2 -static -Wno-override-module $EXTRA "$PROG" \
        "$RAIZ/runtime/zl_runtime_posix.c" -lm -o "$RAIZ/placa/${NOME}_rpi"
    echo "-> placa/${NOME}_rpi   (copiar para o Raspberry Pi 3/4/5 com SO de 64 bits e executar)"
    ;;
  *) echo "placa desconhecida: $PLACA (use uno ou rpi)" >&2; exit 2 ;;
esac
