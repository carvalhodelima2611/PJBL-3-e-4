/* zl_comum.h - Codigo compartilhado por TODAS as implementacoes do runtime.
 * Garante que PC e placa imprimam exatamente o mesmo texto (mesma formatacao
 * de numeros reais e mesmas mensagens de erro), sem depender do printf de cada libc
 * (o printf do avr-libc, por exemplo, nem imprime float por padrao). */
#ifndef ZL_COMUM_H
#define ZL_COMUM_H

#include <stdint.h>
#include "zl_runtime.h"

/* Escreve 'v' com 4 casas decimais em 'buf' (>= 40 bytes). Arredonda a metade para longe do zero. */
static void zl_formata_real(double v, char *buf) {
    char *p = buf;
    if (v != v) { buf[0] = 'n'; buf[1] = 'a'; buf[2] = 'n'; buf[3] = 0; return; }
    if (v < 0) { *p++ = '-'; v = -v; }
    if (v > 1e15) {                                  /* grande demais para 64 bits: notacao cientifica */
        if (v > 1e300 * 1e300 || v - v != 0) { p[0] = 'i'; p[1] = 'n'; p[2] = 'f'; p[3] = 0; return; }
        int e = 0;
        while (v >= 10.0) { v /= 10.0; e++; }
        uint32_t m = (uint32_t)(v * 10000.0 + 0.5);
        if (m >= 100000) { m /= 10; e++; }
        *p++ = (char)('0' + m / 10000); *p++ = '.';
        for (uint32_t d = 1000; d > 0; d /= 10) *p++ = (char)('0' + (m / d) % 10);
        *p++ = 'e'; *p++ = '+';
        char tmp[8]; int n = 0;
        do { tmp[n++] = (char)('0' + e % 10); e /= 10; } while (e);
        while (n) *p++ = tmp[--n];
        *p = 0;
        return;
    }
    uint64_t x = (uint64_t)(v * 10000.0 + 0.5);      /* valor em decimos de milesimo */
    uint64_t inteiro = x / 10000;
    uint32_t frac = (uint32_t)(x % 10000);
    char tmp[24]; int n = 0;
    do { tmp[n++] = (char)('0' + (int)(inteiro % 10)); inteiro /= 10; } while (inteiro);
    while (n) *p++ = tmp[--n];
    *p++ = '.';
    *p++ = (char)('0' + frac / 1000);
    *p++ = (char)('0' + (frac / 100) % 10);
    *p++ = (char)('0' + (frac / 10) % 10);
    *p++ = (char)('0' + frac % 10);
    *p = 0;
}

/* Escreve um inteiro de 32 bits com sinal (sem printf) */
static void zl_formata_int(int32_t v, char *buf) {
    char tmp[16]; int n = 0; char *p = buf;
    uint32_t u = (uint32_t)v;
    if (v < 0) { *p++ = '-'; u = 0u - u; }           /* funciona ate para INT32_MIN */
    do { tmp[n++] = (char)('0' + (int)(u % 10)); u /= 10; } while (u);
    while (n) *p++ = tmp[--n];
    *p = 0;
}

static const char *zl_mensagem_erro(int32_t codigo) {
    switch (codigo) {
        case ZL_ERRO_PARIDADE: return "violacao de paridade";
        case ZL_ERRO_INDICE: return "indice de vetor fora dos limites";
        case ZL_ERRO_DIVISAO_ZERO: return "divisao inteira por zero";
        case ZL_ERRO_SEM_RETORNO: return "funcao terminou sem 'respost'";
        case ZL_ERRO_LEITURA: return "leitura invalida (esperava um numero)";
        default: return "erro desconhecido";
    }
}

/* Potencia inteira por quadrados sucessivos (estouro da a volta, como o resto da aritmetica) */
int32_t zl_pot_inteira(int32_t base, int32_t expoente) {
    if (expoente < 0) {                              /* 1/b^n truncado: so +-1 sobrevivem */
        if (base == 1) return 1;
        if (base == -1) return (expoente % 2) ? -1 : 1;
        return 0;
    }
    uint32_t r = 1, b = (uint32_t)base;
    uint32_t e = (uint32_t)expoente;
    while (e) { if (e & 1u) r *= b; b *= b; e >>= 1; }
    return (int32_t)r;
}

#endif
