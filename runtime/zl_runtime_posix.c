/* zl_runtime_posix.c - Biblioteca de execucao para PC e Raspberry Pi (Linux/POSIX)
 * Compilar junto com o .ll gerado:  clang saida.ll runtime/zl_runtime_posix.c -o prog -lm
 * Saida no stdout; mensagens de simulacao dos pinos vao para o stderr (nao
 * atrapalham a comparacao com a saida esperada). */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "zl_runtime.h"
#include "zl_comum.h"

static void escreve(const char *s) { fputs(s, stdout); }

void zl_imprime_int(int32_t v) { char b[16]; zl_formata_int(v, b); escreve(b); }
void zl_imprime_flut(float v) { char b[48]; zl_formata_real((double)v, b); escreve(b); }
void zl_imprime_duplo(double v) { char b[48]; zl_formata_real(v, b); escreve(b); }
void zl_imprime_bool(int32_t v) { escreve(v ? "VER" : "FAL"); }
void zl_imprime_texto(const char *s) { if (s) escreve(s); }
void zl_imprime_nl(void) { escreve("\n"); }

int32_t zl_le_int(void) {
    long v;
    fflush(stdout);
    if (scanf("%ld", &v) != 1) zl_erro_execucao(ZL_ERRO_LEITURA, 0);
    return (int32_t)v;
}
float zl_le_flut(void) {
    float v;
    fflush(stdout);
    if (scanf("%f", &v) != 1) zl_erro_execucao(ZL_ERRO_LEITURA, 0);
    return v;
}
double zl_le_duplo(void) {
    double v;
    fflush(stdout);
    if (scanf("%lf", &v) != 1) zl_erro_execucao(ZL_ERRO_LEITURA, 0);
    return v;
}

void zl_erro_execucao(int32_t codigo, int32_t linha) {
    char b[16];
    escreve("\n[ERRO DE EXECUCAO] ");
    escreve(zl_mensagem_erro(codigo));
    if (linha > 0) { escreve(" (linha "); zl_formata_int(linha, b); escreve(b); escreve(")"); }
    escreve("\n");
    fflush(stdout);
    exit(100 + codigo);                 /* codigo de saida 102, 103, ... identifica o erro */
}

/* Sem pinos de verdade no PC: simula, registrando no stderr */
static int pinos[64];
void zl_pino_modo(int32_t pino, int32_t modo) {
    fprintf(stderr, "[simulacao] pino %d em modo %s\n", (int)pino, modo == 1 ? "SAIDA" : "ENTRADA");
}
void zl_pino_escreve(int32_t pino, int32_t valor) {
    if (pino >= 0 && pino < 64) pinos[pino] = valor != 0;
    fprintf(stderr, "[simulacao] pino %d <- %s\n", (int)pino, valor ? "ALTO" : "BAIXO");
}
int32_t zl_pino_le(int32_t pino) { return (pino >= 0 && pino < 64) ? pinos[pino] : 0; }
void zl_espera(int32_t ms) {
    if (ms <= 0) return;
    struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

int main(void) {
    int32_t r = zl_principal();         /* o respost da principal vira o codigo de saida */
    fflush(stdout);
    return (int)(r & 0xFF);
}
