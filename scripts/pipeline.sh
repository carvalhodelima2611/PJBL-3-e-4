#!/usr/bin/env bash
# pipeline.sh - Rebaixa os nucleos vetoriais (MLIR) gerados pelo compilador ate LLVM IR.
#
# Uso:  scripts/pipeline.sh entrada.mlir saida.ll [modo] [dir_niveis]
#   modo = base      sem vetorizacao (C0 do estudo; usado no make test)      [padrao]
#          vetor     vetorizacao explicita no MLIR com affine-super-vectorize (C2)
#          paralelo  paralelismo: affine-parallelize + scf -> OpenMP (C3)
#   dir_niveis (opcional): salva o nucleo em cada nivel do pipeline
#          (1_affine, 2_vector, 3_scf, 4_cf, 5_llvm) para o relatorio.
#
# Versao fixada: MLIR/LLVM 18 (mlir-opt e mlir-translate do pacote mlir-18-tools).
set -euo pipefail
ENTRADA="$1"; SAIDA="$2"; MODO="${3:-base}"; NIVEIS="${4:-}"
LLVM_BIN="${LLVM_BIN:-$(llvm-config-18 --bindir 2>/dev/null || echo /usr/lib/llvm-18/bin)}"
OPT="$LLVM_BIN/mlir-opt"; TRANSLATE="$LLVM_BIN/mlir-translate"
VV="${VV:-8}"     # tamanho do vetor virtual (8 x i32 = 256 bits, cabe num registrador AVX2)
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

nivel() { [ -n "$NIVEIS" ] && mkdir -p "$NIVEIS" && cp "$1" "$NIVEIS/$2.mlir" || true; }

cp "$ENTRADA" "$TMP/1.mlir"
nivel "$TMP/1.mlir" 1_affine

# ---------- Passo 1: transformacoes no nivel affine (dependem do modo) ----------
case "$MODO" in
  base)
    cp "$TMP/1.mlir" "$TMP/2.mlir" ;;
  vetor)
    # affine-super-vectorize: troca o laco escalar por um laco de passo 8 que usa
    # vector.transfer_read/write de vector<8xT>; vectorize-reductions=true tambem
    # vetoriza os lacos com iter_args (soma, maximo, produto escalar).
    "$OPT" "$TMP/1.mlir" --affine-super-vectorize="virtual-vector-size=$VV vectorize-reductions=true" \
           --canonicalize -o "$TMP/2.mlir" ;;
  paralelo)
    # affine-parallelize: marca como affine.parallel os lacos sem dependencia entre
    # iteracoes (max-nested=1: so o laco mais externo); parallel-reductions=true
    # tambem paraleliza as reducoes (cada thread acumula uma parte e no fim combina).
    #
    # Defeito do MLIR 18 (documentado em docs/relatorio_otimizacao.md): com
    # parallel-reductions=true, uma reducao de MAXIMO INTEIRO (arith.maxsi) gera um
    # affine.parallel invalido ("result type cannot match reduction attribute").
    # Contorno: cada nucleo e paralelizado separadamente; o que falhar e refeito com
    # parallel-reductions=false (a reducao fica sequencial, o resto continua paralelo).
    if ! "$OPT" "$TMP/1.mlir" --affine-parallelize="max-nested=1 parallel-reductions=true" -o "$TMP/2.mlir" 2>/dev/null; then
      awk -v d="$TMP" '/^  \/\/ nucleo /{n++} n>0 && !/^}$/ {print > (d "/k" n ".corpo")}' "$TMP/1.mlir"
      echo "module {" > "$TMP/2.mlir"
      for k in "$TMP"/k*.corpo; do
        { echo "module {"; cat "$k"; echo "}"; } > "$k.mlir"
        if ! "$OPT" "$k.mlir" --mlir-print-local-scope --affine-parallelize="max-nested=1 parallel-reductions=true" -o "$k.par" 2>/dev/null; then
          echo "aviso: $(grep -o '@dsl[^(]*' "$k" | head -1): reducao nao paralelizavel no MLIR 18 (fica sequencial)" >&2
          "$OPT" "$k.mlir" --mlir-print-local-scope --affine-parallelize="max-nested=1 parallel-reductions=false" -o "$k.par"
        fi
        sed '1d;$d' "$k.par" >> "$TMP/2.mlir"
      done
      echo "}" >> "$TMP/2.mlir"
    fi ;;
  *) echo "modo desconhecido: $MODO" >&2; exit 2 ;;
esac
nivel "$TMP/2.mlir" 2_vector

# ---------- Passo 2: affine -> scf (lacos estruturados com indices calculados) ----------
# lower-affine: affine.for/affine.load viram scf.for/memref.load (some o "affine",
# que e a informacao que permitia provar que os acessos sao lineares).
"$OPT" "$TMP/2.mlir" --lower-affine -o "$TMP/3.mlir"
if [ "$MODO" = vetor ]; then
  # convert-vector-to-scf: transfer_read/write de tamanho fixo viram load/store de
  # vetor (com mascara so quando necessario; aqui N e multiplo de 8, entao sem mascara).
  "$OPT" "$TMP/3.mlir" --convert-vector-to-scf --lower-affine -o "$TMP/3b.mlir" && mv "$TMP/3b.mlir" "$TMP/3.mlir"
fi
if [ "$MODO" = paralelo ]; then
  # convert-scf-to-openmp: scf.parallel vira omp.parallel + omp.wsloop (laco dividido entre threads);
  # canonicalize e necessario no MLIR 18 para limpar o resultado antes de ir para o dialeto llvm.
  "$OPT" "$TMP/3.mlir" --convert-scf-to-openmp --canonicalize -o "$TMP/3b.mlir" && mv "$TMP/3b.mlir" "$TMP/3.mlir"
fi
nivel "$TMP/3.mlir" 3_scf

# ---------- Passo 3: scf -> cf (lacos viram blocos basicos com desvios) ----------
# convert-scf-to-cf: scf.for vira blocos com cf.br/cf.cond_br (como o LLVM IR "a mao").
"$OPT" "$TMP/3.mlir" --convert-scf-to-cf -o "$TMP/4.mlir"
nivel "$TMP/4.mlir" 4_cf

# ---------- Passo 4: tudo para o dialeto llvm ----------
# convert-vector-to-llvm   : vector<8xi32> vira o tipo de vetor do LLVM <8 x i32>
# convert-arith-to-llvm    : arith.addi/addf/... viram llvm.add/llvm.fadd/...
# finalize-memref-to-llvm  : memref vira descritor {ptr, ptr, offset, tamanho, passo}
# convert-func-to-llvm     : func.func vira llvm.func; use-bare-ptr-memref-call-conv=1
#                            faz cada memref de tamanho fixo virar UM ptr simples no
#                            parametro (e assim que o LLVM IR do PjBL 3 chama o nucleo)
# convert-cf-to-llvm       : cf.br/cf.cond_br viram llvm.br/llvm.cond_br
# convert-openmp-to-llvm   : (modo paralelo) regioes omp com tipos do dialeto llvm
# reconcile-unrealized-casts: remove as conversoes temporarias que sobraram entre os passes
EXTRA=""
[ "$MODO" = paralelo ] && EXTRA="--convert-openmp-to-llvm"
"$OPT" "$TMP/4.mlir" --convert-vector-to-llvm --convert-arith-to-llvm --finalize-memref-to-llvm \
       "--convert-func-to-llvm=use-bare-ptr-memref-call-conv=1" --convert-cf-to-llvm $EXTRA \
       --reconcile-unrealized-casts -o "$TMP/5.mlir"
nivel "$TMP/5.mlir" 5_llvm

# ---------- Passo 5: dialeto llvm -> LLVM IR textual ----------
# mlir-translate: gera o .ll (sem target triple, igual ao do PjBL 3) para o llvm-link.
"$TRANSLATE" --mlir-to-llvmir "$TMP/5.mlir" -o "$SAIDA"
echo "pipeline ($MODO): $ENTRADA -> $SAIDA"
