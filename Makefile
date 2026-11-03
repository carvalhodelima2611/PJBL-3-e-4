# Compilador Macaronica - PjBL 1, 3 e 4
#   make                 compila o compilador (./macaronica)
#   make test            testes do PjBL 1 + execucao (PjBL 3) + DSL (PjBL 4) + placa no simulador
#   make run PROG=x.mac  compila e executa um programa no PC
#   make bench           estudo de otimizacao C0-C3 (PjBL 4, Parte C) + graficos
#   make placa PROG=x.mac          executavel para o Arduino Uno (placa/x.hex)
#   make placa PROG=x.mac PLACA=rpi  executavel ELF para Raspberry Pi 64 bits
#   make clean
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -g -O1
SRC_DIR = src
BUILD_DIR = build
TARGET = macaronica

SRCS = $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/semantic.c \
       $(SRC_DIR)/codegen_llvm.c $(SRC_DIR)/codegen_mlir.c $(SRC_DIR)/main.c
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
HDRS = $(wildcard $(SRC_DIR)/*.h)

# Ferramentas LLVM/MLIR 18 (versao fixada; ver README)
LLVM_BIN ?= $(shell llvm-config-18 --bindir 2>/dev/null || echo /usr/lib/llvm-18/bin)
export PATH := $(LLVM_BIN):$(PATH)

PROG ?= tests/execucao/04_recursao.mac
PLACA ?= uno

.PHONY: all clean test test-pjbl1 test-execucao test-dsl test-placa run bench placa

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---------- Testes ----------
test: test-pjbl1 test-execucao test-dsl test-placa

# PjBL 1: validos tem que ser aceitos (saida 0), invalidos rejeitados SEM gerar arquivo
test-pjbl1: $(TARGET)
	@echo "== PjBL 1: casos VALIDOS (devem ser aceitos) =="
	@fal=0; for f in tests/validos/*.mac; do \
		if ./$(TARGET) $$f -o $(BUILD_DIR)/v.ll > /dev/null 2>&1; then echo "  [OK]     $$f"; \
		else echo "  [FALHOU] $$f"; fal=1; fi; \
	done; \
	echo "== PjBL 1: casos INVALIDOS (devem ser rejeitados, sem .ll) =="; \
	for f in tests/invalidos/*.mac tests/dsl/invalidos/*.mac tests/placa/invalidos/*.mac; do \
		rm -f $(BUILD_DIR)/i.ll; alvo=""; case $$f in tests/placa/*) alvo="--alvo=uno";; esac; \
		if ! ./$(TARGET) $$f -o $(BUILD_DIR)/i.ll $$alvo > /dev/null 2>&1 && [ ! -f $(BUILD_DIR)/i.ll ]; then \
			echo "  [OK]     $$f"; else echo "  [FALHOU] $$f"; fal=1; fi; \
	done; [ $$fal -eq 0 ]

test-execucao: $(TARGET)
	@echo "== PjBL 3: execucao (lli e nativo) comparada com .esperado =="
	@scripts/testa_execucao.sh tests/execucao

test-dsl: $(TARGET)
	@echo "== PjBL 4: DSL vetorial (MLIR -> LLVM IR) comparada com .esperado =="
	@scripts/testa_execucao.sh tests/dsl
	@echo "   (mesmos testes com os nucleos VETORIZADOS no MLIR)"
	@PIPELINE_MODO=vetor scripts/testa_execucao.sh tests/dsl
	@echo "   (mesmos testes com os nucleos PARALELOS via OpenMP)"
	@PIPELINE_MODO=paralelo scripts/testa_execucao.sh tests/dsl

test-placa: $(TARGET)
	@echo "== PjBL 4: programas da placa no simulador (simavr / qemu-aarch64) =="
	@scripts/testa_placa.sh

# ---------- Execucao no PC ----------
run: $(TARGET)
	@scripts/macc.sh $(PROG) $(BUILD_DIR)/prog && echo "-- executando $(PROG) --" && ./$(BUILD_DIR)/prog

# ---------- PjBL 4 ----------
bench: $(TARGET)
	@scripts/bench/roda_bench.sh

placa: $(TARGET)
	@scripts/placa.sh $(PROG) $(PLACA)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
