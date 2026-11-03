#!/usr/bin/env bash
# testa_placa.sh - roda os programas de tests/placa nas duas arquiteturas, em emulador,
# e compara com a MESMA saida esperada do PC:
#   Arduino Uno (AVR 8 bits)   -> simavr, lendo o que o programa manda pela serial
#   Raspberry Pi (ARM 64 bits) -> qemu-aarch64 (ELF estatico)
# Tambem confere que a verificacao de memoria do alvo rejeita tests/placa/invalidos.
set -u
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
B="$RAIZ/build/placa"; mkdir -p "$B"
ok=0; falhou=0
compara() {   # $1=rotulo $2=esperado $3=obtido
    if diff -q "$2" "$3" > /dev/null; then echo "  [OK]     $1"; ok=$((ok+1));
    else echo "  [FALHOU] $1"; diff "$2" "$3" | head -8 | sed 's/^/           /'; falhou=$((falhou+1)); fi
}
for f in "$RAIZ"/tests/placa/*.mac; do
    n="$(basename "$f" .mac)"; esp="$RAIZ/tests/placa/$n.esperado"
    # --- Arduino Uno no simavr ---
    if "$RAIZ/scripts/placa.sh" "$f" uno -DZL_SIMULADOR > "$B/$n.uno.build" 2>&1; then
        # o simavr mostra cada linha da serial em verde e troca o "\r\n" do fim por ".."
        timeout 120 simavr -m atmega328p -f 16000000 "$B/uno/$n.elf" 2>&1 \
          | grep $'\e\[32m' | sed -e $'s/\e\[[0-9;]*m//g' -e 's/\.\.$//' > "$B/$n.uno.saida"
        compara "uno  $n (simavr)" "$esp" "$B/$n.uno.saida"
    else echo "  [FALHOU] uno  $n (nao gerou o executavel)"; cat "$B/$n.uno.build"; falhou=$((falhou+1)); fi
    # --- Raspberry Pi 64 bits no qemu ---
    if "$RAIZ/scripts/placa.sh" "$f" rpi > "$B/$n.rpi.build" 2>&1; then
        { qemu-aarch64 "$RAIZ/placa/${n}_rpi" 2>/dev/null < /dev/null; echo "[codigo de saida: $?]"; } > "$B/$n.rpi.saida"
        compara "rpi  $n (qemu-aarch64)" "$esp" "$B/$n.rpi.saida"
    else echo "  [FALHOU] rpi  $n (nao gerou o executavel)"; cat "$B/$n.rpi.build"; falhou=$((falhou+1)); fi
done
for f in "$RAIZ"/tests/placa/invalidos/*.mac; do
    if "$RAIZ/macaronica" "$f" --alvo=uno > "$B/inv.txt" 2>&1; then
        echo "  [FALHOU] $(basename "$f"): deveria ser rejeitado no alvo uno"; falhou=$((falhou+1))
    elif ! "$RAIZ/macaronica" "$f" > /dev/null 2>&1; then
        echo "  [FALHOU] $(basename "$f"): deveria ser valido no PC"; falhou=$((falhou+1))
    else echo "  [OK]     $(basename "$f"): rejeitado no uno, aceito no PC ($(grep -o 'excedem.*' "$B/inv.txt" | head -1 | cut -c1-60)...)"; ok=$((ok+1)); fi
done
echo "  -> $ok passaram, $falhou falharam"
[ "$falhou" -eq 0 ]
