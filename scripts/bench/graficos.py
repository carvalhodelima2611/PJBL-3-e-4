#!/usr/bin/env python3
"""graficos.py - tabelas e graficos do estudo de otimizacao (PjBL 4, Parte C).

Le docs/resultados/bench.csv (gerado por roda_bench.sh) e escreve:
  tabela.md               tempos (ms), speedup sobre C0, resultados numericos
  speedup_configs.png     speedup de cada configuracao sobre C0, por nucleo (maior N)
  speedup_threads.png     escalabilidade do C3 com o numero de threads
Uso: python3 scripts/bench/graficos.py docs/resultados
"""
import csv
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

OUT = sys.argv[1] if len(sys.argv) > 1 else "docs/resultados"

# Paleta categorica em ordem fixa (cada configuracao sempre com a mesma cor)
CORES = {"C0": "#2a78d6", "C1": "#eb6834", "C1f": "#1baf7a", "C2": "#eda100", "C3": "#e87ba4"}
TEXTO, TEXTO2, GRADE, FUNDO = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
NUCLEOS = ["soma_int", "somavet_int", "soma_flut", "somavet_flut", "prodesc_flut"]
NOMES = {"soma_int": "soma\ninteira", "somavet_int": "somavet\ninteira", "soma_flut": "soma\nflut",
         "somavet_flut": "somavet\nflut", "prodesc_flut": "prodesc\nflut"}
DESC = {"C0": "C0 base (-O0)", "C1": "C1 LLVM -O2", "C1f": "C1f -O2 + reassoc",
        "C2": "C2 MLIR vetor", "C3": "C3 OpenMP"}

linhas = list(csv.DictReader(open(f"{OUT}/bench.csv")))
dados = defaultdict(dict)          # (n, cfg, th) -> nucleo -> (ms, res)
compila = {}
for l in linhas:
    k = (int(l["tamanho"]), l["config"], int(l["threads"]))
    dados[k][l["nucleo"]] = (float(l["exec_ms"]), l["resultado"])
    compila[k] = float(l["compila_ms"])
tamanhos = sorted({k[0] for k in dados})
threads = sorted({k[2] for k in dados if k[1] == "C3"})
tmax = max(threads)


def estilo(ax):
    ax.set_facecolor(FUNDO)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRADE)
    ax.tick_params(colors=TEXTO2, labelsize=9)
    ax.yaxis.grid(True, color=GRADE, linewidth=0.8)
    ax.set_axisbelow(True)


# ---------- tabela ----------
with open(f"{OUT}/tabela.md", "w") as f:
    for n in tamanhos:
        f.write(f"\n### N = {n:,}".replace(",", ".") + " elementos\n\n")
        cfgs = [("C0", 1), ("C1", 1), ("C1f", 1), ("C2", 1)] + [("C3", t) for t in threads]
        f.write("| núcleo | " + " | ".join(c if c != "C3" else f"C3 ({t} th)" for c, t in cfgs) + " |\n")
        f.write("|---|" + "---:|" * len(cfgs) + "\n")
        for nuc in NUCLEOS:
            base = dados[(n, "C0", 1)][nuc][0]
            cel = []
            for c, t in cfgs:
                ms = dados[(n, c, t)][nuc][0]
                cel.append(f"{ms:.3f} ms ({base / ms:.1f}×)")
            f.write(f"| {nuc} | " + " | ".join(cel) + " |\n")
        f.write("| *compilação* | " + " | ".join(f"{compila[(n, c, t)]:.0f} ms" for c, t in cfgs) + " |\n")
    n = tamanhos[-1]
    f.write(f"\n### Resultado numérico das reduções `flut` (N = {n:,})\n\n".replace(",", "."))
    f.write("| configuração | somavet_flut | prodesc_flut |\n|---|---:|---:|\n")
    for c, t in [("C0", 1), ("C1", 1), ("C1f", 1), ("C2", 1), ("C3", tmax)]:
        d = dados[(n, c, t)]
        f.write(f"| {DESC[c]} | {d['somavet_flut'][1]} | {d['prodesc_flut'][1]} |\n")

# ---------- grafico 1: speedup sobre C0, por nucleo ----------
n = tamanhos[-1]
cfgs = [("C1", 1), ("C1f", 1), ("C2", 1), ("C3", tmax)]
fig, ax = plt.subplots(figsize=(9, 4.6), dpi=150)
fig.patch.set_facecolor(FUNDO)
estilo(ax)
larg = 0.19
for i, (c, t) in enumerate(cfgs):
    xs = [j + (i - 1.5) * (larg + 0.01) for j in range(len(NUCLEOS))]
    ys = [dados[(n, "C0", 1)][nuc][0] / dados[(n, c, t)][nuc][0] for nuc in NUCLEOS]
    rot = DESC[c] + (f" ({t} threads)" if c == "C3" else "")
    ax.bar(xs, ys, width=larg, color=CORES[c], label=rot, edgecolor=FUNDO, linewidth=1)
    for x, y in zip(xs, ys):
        if y >= 10:
            ax.text(x, y, f"{y:.0f}×", ha="center", va="bottom", fontsize=7, color=TEXTO2)
ax.axhline(1, color=TEXTO2, linewidth=1, linestyle=(0, (3, 3)))
ax.text(len(NUCLEOS) - 0.5, 1.15, "C0 = 1×", color=TEXTO2, fontsize=8, ha="right")
ax.set_xticks(range(len(NUCLEOS)))
ax.set_xticklabels([NOMES[k] for k in NUCLEOS], color=TEXTO)
ax.set_ylabel("speedup sobre C0 (×)", color=TEXTO2)
ax.set_title(f"Speedup de cada configuração sobre a linha de base, N = {n:,}".replace(",", "."),
             color=TEXTO, fontsize=11, loc="left")
ax.legend(frameon=False, fontsize=8, ncol=4, loc="upper left", bbox_to_anchor=(0, -0.16))
fig.tight_layout()
fig.savefig(f"{OUT}/speedup_configs.png", facecolor=FUNDO)

# ---------- grafico 2: escalabilidade do C3 ----------
fig, axs = plt.subplots(1, len(tamanhos), figsize=(9, 3.8), dpi=150, sharey=True)
fig.patch.set_facecolor(FUNDO)
if len(tamanhos) == 1:
    axs = [axs]
ordem = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4"]
for ax, n in zip(axs, tamanhos):
    estilo(ax)
    for cor, nuc in zip(ordem, NUCLEOS):
        t1 = dados[(n, "C3", 1)][nuc][0]
        ys = [t1 / dados[(n, "C3", t)][nuc][0] for t in threads]
        ax.plot(threads, ys, color=cor, linewidth=2, marker="o", markersize=5, label=nuc)
    ax.plot(threads, [min(t, 2) for t in threads], color=TEXTO2, linewidth=1, linestyle=(0, (3, 3)))
    ax.set_xticks(threads)
    ax.set_xlabel("threads (OMP_NUM_THREADS)", color=TEXTO2)
    ax.set_title(f"N = {n:,}".replace(",", "."), color=TEXTO, fontsize=10, loc="left")
axs[0].set_ylabel("speedup sobre C3 com 1 thread", color=TEXTO2)
axs[-1].text(threads[-1], 2.03, "ideal (2 núcleos)", color=TEXTO2, fontsize=7, ha="right", va="bottom")
axs[0].legend(frameon=False, fontsize=7, loc="upper left")
fig.suptitle("Escalabilidade do C3 (affine-parallelize + OpenMP)", color=TEXTO, fontsize=11, x=0.01, ha="left")
fig.tight_layout()
fig.savefig(f"{OUT}/speedup_threads.png", facecolor=FUNDO)
print(f"tabela e graficos escritos em {OUT}")
