#!/usr/bin/env bash
# roda_bench.sh - Estudo experimental de otimizacao (PjBL 4, Parte C).
#
# Para cada tamanho (10^6 e 10^7) e cada configuracao:
#   C0  pipeline sem vetorizacao + clang -O0 (linha de base)
#   C1  pipeline sem vetorizacao + clang -O2 -march=native (autovetorizacao do LLVM)
#   C1f igual ao C1, com as somas reais marcadas fastmath<reassoc> (equivalente ao
#       -ffast-math, que nao age sobre um .ll pronto): pergunta da reducao de ponto flutuante
#   C2  pipeline com affine-super-vectorize + clang -O2 com o vetorizador do LLVM
#       DESLIGADO (-fno-vectorize -fno-slp-vectorize): so a vetorizacao do MLIR conta
#   C3  pipeline com affine-parallelize + OpenMP + clang -O2, com 1, 2 e 4 threads
# mede: tempo de compilacao (pipeline + clang, mediana de 5) e tempo de execucao de
# cada nucleo (mediana de 7 chamadas dentro do processo, e mediana de 5 processos).
# Saidas em docs/resultados/: bench.csv, maquina.txt, rpass_*.txt, evidencias de IR.
set -euo pipefail
RAIZ="$(cd "$(dirname "$0")/../.." && pwd)"
LLVM_BIN="${LLVM_BIN:-$(llvm-config-18 --bindir 2>/dev/null || echo /usr/lib/llvm-18/bin)}"
export PATH="$LLVM_BIN:$PATH"
OUT="$RAIZ/docs/resultados"; B="$RAIZ/build/bench"
mkdir -p "$OUT" "$B"
TAMANHOS="${TAMANHOS:-1000000 10000000}"
EXECUCOES="${EXECUCOES:-5}"
THREADS="${THREADS:-1 2 4}"

mediana() { sort -g | awk '{v[NR]=$1} END{print v[int((NR+1)/2)]}'; }
agora() { date +%s.%N; }

# ---------- maquina ----------
{
  echo "CPU: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //')"
  echo "Nucleos: $(nproc)"
  echo "SIMD: $(grep -m1 -o -w 'sse4_2\|avx\|avx2\|avx512f\|avx512bw\|avx512vl\|fma\|neon\|asimd' /proc/cpuinfo | sort -u | tr '\n' ' ')"
  echo "Memoria: $(free -g | awk '/Mem/{print $2 " GB"}')"
  echo "SO: $(uname -sr)"
  echo "clang: $(clang --version | head -1)"
  echo "mlir-opt: $(mlir-opt --version 2>&1 | grep -i version | head -1 | sed 's/^ *//')"
} > "$OUT/maquina.txt"
cat "$OUT/maquina.txt"

echo "tamanho,config,threads,nucleo,exec_ms,compila_ms,resultado" > "$OUT/bench.csv"

compila() {   # $1=N $2=config $3=modo_pipeline $4=flags_clang  -> $B/prog_N_config, imprime tempo (ms)
  local n="$1" cfg="$2" modo="$3" flags="$4"
  local dir="$B/$n"; mkdir -p "$dir"
  local ts=()
  for _ in 1 2 3 4 5; do
    local t0; t0=$(agora)
    REASSOC=$([ "$cfg" = C1f ] && echo 1 || echo 0) \
      "$RAIZ/scripts/pipeline.sh" "$dir/bench.mlir" "$dir/nucleos_$cfg.ll" "$modo" > /dev/null 2> "$dir/pipeline_$cfg.err"
    # shellcheck disable=SC2086
    clang -c $flags -Wno-override-module -Wno-invalid-feature-combination "$dir/nucleos_$cfg.ll" -o "$dir/nucleos_$cfg.o" 2> "$dir/rpass_$cfg.txt"
    local t1; t1=$(agora)
    ts+=("$(echo "($t1 - $t0) * 1000" | bc -l)")
  done
  local omp=""; [ "$modo" = paralelo ] && omp="-fopenmp"
  clang -O2 -DN="$n" $omp "$RAIZ/scripts/bench/harness.c" "$dir/nucleos_$cfg.o" -o "$dir/prog_$cfg"
  printf '%s\n' "${ts[@]}" | mediana
}

mede() {   # $1=N $2=config $3=threads $4=compila_ms
  local n="$1" cfg="$2" th="$3" cms="$4" dir="$B/$1"
  for r in $(seq "$EXECUCOES"); do OMP_NUM_THREADS="$th" "$dir/prog_$cfg" > "$dir/exec_${cfg}_${th}_$r.txt"; done
  for nuc in soma_int somavet_int soma_flut somavet_flut prodesc_flut; do
    local med res
    med=$(cat "$dir"/exec_"${cfg}"_"${th}"_*.txt | awk -v k="$nuc" '$1==k{print $2}' | mediana)
    res=$(awk -v k="$nuc" '$1==k{print $3}' "$dir/exec_${cfg}_${th}_1.txt")
    echo "$n,$cfg,$th,$nuc,$med,$cms,$res" >> "$OUT/bench.csv"
  done
  echo "  N=$n $cfg threads=$th: $(awk -F, -v n="$n" -v c="$cfg" -v t="$th" '$1==n&&$2==c&&$3==t{printf "%s=%sms ", $4, $5}' "$OUT/bench.csv")"
}

for n in $TAMANHOS; do
  dir="$B/$n"; mkdir -p "$dir"
  sed "s/TAM/$n/g" "$RAIZ/scripts/bench/bench.mac.modelo" > "$dir/bench.mac"
  "$RAIZ/macaronica" "$dir/bench.mac" -o "$dir/bench.ll" > /dev/null
  RP="-Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"
  c=$(compila "$n" C0  base     "-O0");                                    mede "$n" C0  1 "$c"
  c=$(compila "$n" C1  base     "-O2 -march=native $RP");                  mede "$n" C1  1 "$c"
  c=$(compila "$n" C1f base     "-O2 -march=native $RP");      mede "$n" C1f 1 "$c"
  c=$(compila "$n" C2  vetor    "-O2 -march=native -fno-vectorize -fno-slp-vectorize"); mede "$n" C2 1 "$c"
  c=$(compila "$n" C3  paralelo "-O2 -march=native -fopenmp")
  for th in $THREADS; do mede "$n" C3 "$th" "$c"; done
done

# ---------- evidencias (IR e relatorios -Rpass) do maior tamanho ----------
n=$(echo "$TAMANHOS" | awk '{print $NF}'); dir="$B/$n"
cp "$dir/rpass_C1.txt" "$OUT/rpass_C1.txt"; cp "$dir/rpass_C1f.txt" "$OUT/rpass_C1f.txt"
for cfg in C1 C1f C2; do
  fl="-O2 -march=native"
  [ $cfg = C2 ] && fl="$fl -fno-vectorize -fno-slp-vectorize"
  # shellcheck disable=SC2086
  clang -S -emit-llvm $fl -Wno-override-module -Wno-invalid-feature-combination "$dir/nucleos_$cfg.ll" -o "$OUT/ir_final_$cfg.ll"
done
# A Aula 12: sem target triple o opt nao vetoriza nada (nao sabe a largura dos registradores)
opt -O2 -S "$dir/nucleos_C0.ll" -o "$OUT/ir_opt_O2_sem_triple.ll"
opt -O2 -S -mtriple=x86_64-pc-linux-gnu -mcpu=native "$dir/nucleos_C0.ll" -o "$OUT/ir_opt_O2_com_triple.ll"
{
  echo "Tipos vetoriais na IR final de cada configuracao (ocorrencias):"
  for f in ir_final_C1 ir_final_C1f ir_final_C2 ir_opt_O2_sem_triple ir_opt_O2_com_triple; do
    printf '  %-24s %s\n' "$f" "$(grep -o '<[0-9]* x [a-z0-9]*>' "$OUT/$f.ll" | sort | uniq -c | tr '\n' ' ' | sed 's/  */ /g')"
  done
} > "$OUT/tipos_vetoriais.txt"
cat "$OUT/tipos_vetoriais.txt"

# medicoes brutas (uma linha por nucleo por execucao) para quem quiser conferir a variancia
rm -rf "$OUT/bruto"; mkdir -p "$OUT/bruto"
for n in $TAMANHOS; do for f in "$B/$n"/exec_*.txt; do cp "$f" "$OUT/bruto/${n}_$(basename "$f")"; done; done

python3 "$RAIZ/scripts/bench/graficos.py" "$OUT"
echo "resultados em $OUT"
