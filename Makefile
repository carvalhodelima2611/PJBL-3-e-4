CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -g
SRC_DIR = src
BUILD_DIR = build
TARGET = macaronica

SRCS = $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/semantic.c $(SRC_DIR)/main.c
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/lexer.h $(SRC_DIR)/parser.h $(SRC_DIR)/semantic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Executa o compilador contra todos os casos de teste, valido e invalido,
# imprimindo um resumo de PASSOU/FALHOU para cada arquivo.
test: $(TARGET)
	@echo "== Casos VALIDOS (devem ser aceitos, saida 0) =="
	@for f in tests/validos/*.mac; do \
		./$(TARGET) $$f > /dev/null 2>&1; \
		if [ $$? -eq 0 ]; then echo "  [OK]     $$f"; else echo "  [FALHOU] $$f"; fi; \
	done
	@echo "== Casos INVALIDOS (devem ser rejeitados, saida != 0) =="
	@for f in tests/invalidos/*.mac; do \
		./$(TARGET) $$f > /dev/null 2>&1; \
		if [ $$? -ne 0 ]; then echo "  [OK]     $$f"; else echo "  [FALHOU] $$f"; fi; \
	done

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
