/* zl_runtime.h - Interface da biblioteca de execucao da linguagem Macaronica
 *
 * E a UNICA ponte entre o codigo gerado (LLVM IR portavel) e a plataforma.
 * Todas as funcoes tem assinatura fixa (nada de printf/scanf variadicos no IR)
 * e tipos de largura fixa. Cada plataforma tem a sua implementacao:
 *   zl_runtime_posix.c  -> PC e Raspberry Pi (Linux): stdout/stdin
 *   zl_runtime_avr.c    -> Arduino Uno (ATmega328P, sem SO): serial + LED
 * O ponto de entrada real (main) fica na implementacao, que chama zl_principal(). */
#ifndef ZL_RUNTIME_H
#define ZL_RUNTIME_H

#include <stdint.h>

/* Codigos de erro de execucao (zl_erro_execucao) */
#define ZL_ERRO_PARIDADE      1   /* (Zolang; nao usado em Macaronica) */
#define ZL_ERRO_INDICE        2   /* acesso a vetor fora dos limites */
#define ZL_ERRO_DIVISAO_ZERO  3   /* divisao inteira (//) por zero */
#define ZL_ERRO_SEM_RETORNO   4   /* funcao nao-vazio terminou sem respost */
#define ZL_ERRO_LEITURA       5   /* leinteira()/leflut() sem um numero valido na entrada */

/* Gerada pelo compilador: a funcao principal do programa (devolve o codigo de saida) */
int32_t zl_principal(void);

/* Saida */
void zl_imprime_int(int32_t v);
void zl_imprime_flut(float v);        /* 4 casas decimais */
void zl_imprime_duplo(double v);      /* 4 casas decimais (no AVR, double = float) */
void zl_imprime_bool(int32_t v);      /* VER / FAL */
void zl_imprime_texto(const char *s); /* NULL imprime texto vazio */
void zl_imprime_nl(void);

/* Entrada */
int32_t zl_le_int(void);
float   zl_le_flut(void);
double  zl_le_duplo(void);

/* Erro de execucao: mostra mensagem com a linha do fonte e encerra (nao retorna) */
void zl_erro_execucao(int32_t codigo, int32_t linha)
#if defined(__GNUC__)
    __attribute__((noreturn))
#endif
    ;

/* Aritmetica sem instrucao direta na IR */
int32_t zl_pot_inteira(int32_t base, int32_t expoente);

/* E/S fisica (funcoes embutidas pino_modo, pino_escreve, pino_le, espera) */
#define ZL_SAIDA   1
#define ZL_ENTRADA 0
void    zl_pino_modo(int32_t pino, int32_t modo);       /* modo: 1 = saida, 0 = entrada, 2 = entrada com pull-up */
void    zl_pino_escreve(int32_t pino, int32_t valor);   /* 0 = BAIXO, outro = ALTO */
int32_t zl_pino_le(int32_t pino);
void    zl_espera(int32_t ms);

#endif
