/* zl_runtime_avr.c - Biblioteca de execucao para o Arduino Uno (ATmega328P, 16 MHz, sem SO)
 *
 * Escrita "a mao" sobre os registradores do microcontrolador (sem o nucleo do
 * Arduino), so com avr-libc. E a UNICA parte especifica da placa: o resto do
 * executavel e o .ll gerado pelo compilador, compilado com
 *     llc -mtriple=avr -mcpu=atmega328p
 *
 * Saida e entrada: serial (USART0), 9600 baud, 8N1 - o Monitor Serial da IDE do
 *   Arduino, ou "screen /dev/ttyACM0 9600".
 * Erro de execucao: mensagem na serial + LED do pino 13 piscando rapido para sempre.
 * Numeracao dos pinos: a mesma impressa na placa (0-13 digitais, 14-19 = A0-A5).
 *
 * -DZL_SIMULADOR (usado no simavr): em vez de ficar para sempre no laco final ou
 * piscando, desliga a CPU (cli + sleep), o que faz o simavr encerrar a simulacao. */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <math.h>
#include <stdint.h>
#include "zl_runtime.h"
#include "zl_comum.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#define BAUD 9600

/* ---------- serial ---------- */
static void serial_iniciar(void) {
    uint16_t ubrr = (uint16_t)(F_CPU / 16 / BAUD - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (uint8_t)((1 << TXEN0) | (1 << RXEN0));
    UCSR0C = (uint8_t)((1 << UCSZ01) | (1 << UCSZ00));      /* 8 bits, sem paridade, 1 stop */
}
static void serial_byte(char c) {
    while (!(UCSR0A & (1 << UDRE0))) { }                     /* espera o buffer de envio esvaziar */
    UDR0 = (uint8_t)c;
}
static void escreve(const char *s) { while (*s) { if (*s == '\n') serial_byte('\r'); serial_byte(*s++); } }
static char serial_le(void) {
    while (!(UCSR0A & (1 << RXC0))) { }
    return (char)UDR0;
}

/* ---------- pinos digitais (numeracao do Arduino) ---------- */
static volatile uint8_t *registrador(int32_t pino, uint8_t *bit, int qual) {
    /* qual: 0 = DDR (direcao), 1 = PORT (saida/pull-up), 2 = PIN (leitura) */
    if (pino >= 0 && pino <= 7) { *bit = (uint8_t)pino; return qual == 0 ? &DDRD : qual == 1 ? &PORTD : &PIND; }
    if (pino >= 8 && pino <= 13) { *bit = (uint8_t)(pino - 8); return qual == 0 ? &DDRB : qual == 1 ? &PORTB : &PINB; }
    if (pino >= 14 && pino <= 19) { *bit = (uint8_t)(pino - 14); return qual == 0 ? &DDRC : qual == 1 ? &PORTC : &PINC; }
    return 0;
}

void zl_pino_modo(int32_t pino, int32_t modo) {
    uint8_t b; volatile uint8_t *ddr = registrador(pino, &b, 0), *port = registrador(pino, &b, 1);
    if (!ddr) return;
    if (modo == 1) *ddr |= (uint8_t)(1 << b);
    else { *ddr &= (uint8_t)~(1 << b); if (modo == 2) *port |= (uint8_t)(1 << b); else *port &= (uint8_t)~(1 << b); }
}
void zl_pino_escreve(int32_t pino, int32_t valor) {
    uint8_t b; volatile uint8_t *port = registrador(pino, &b, 1);
    if (!port) return;
    if (valor) *port |= (uint8_t)(1 << b); else *port &= (uint8_t)~(1 << b);
}
int32_t zl_pino_le(int32_t pino) {
    uint8_t b; volatile uint8_t *pin = registrador(pino, &b, 2);
    return (pin && (*pin & (1 << b))) ? 1 : 0;
}
void zl_espera(int32_t ms) { while (ms-- > 0) _delay_ms(1); }

/* ---------- saida ---------- */
void zl_imprime_int(int32_t v) { char b[16]; zl_formata_int(v, b); escreve(b); }
void zl_imprime_flut(float v) { char b[48]; zl_formata_real((double)v, b); escreve(b); }
void zl_imprime_duplo(double v) { char b[48]; zl_formata_real(v, b); escreve(b); }   /* double = float no avr-gcc */
void zl_imprime_bool(int32_t v) { escreve(v ? "VER" : "FAL"); }
void zl_imprime_texto(const char *s) { if (s) escreve(s); }
void zl_imprime_nl(void) { escreve("\n"); }

/* ---------- entrada: le um numero da serial (ignora espacos/quebras antes) ---------- */
static double le_numero(int aceita_ponto) {
    char c;
    do { c = serial_le(); } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    int neg = 0; double v = 0, casa = 0; int algum = 0;
    if (c == '-') { neg = 1; c = serial_le(); }
    for (;;) {
        if (c >= '0' && c <= '9') { algum = 1; if (casa > 0) { v += (c - '0') / casa; casa *= 10; } else v = v * 10 + (c - '0'); }
        else if (c == '.' && aceita_ponto && casa == 0) casa = 10;
        else break;
        c = serial_le();
    }
    if (!algum) zl_erro_execucao(ZL_ERRO_LEITURA, 0);
    return neg ? -v : v;
}
int32_t zl_le_int(void) { return (int32_t)le_numero(0); }
float zl_le_flut(void) { return (float)le_numero(1); }
double zl_le_duplo(void) { return le_numero(1); }

/* ---------- fim e erros ---------- */
static void parar(void) {
#ifdef ZL_SIMULADOR
    cli();
    sleep_enable();
    sleep_cpu();                       /* simavr: "sleeping with interrupts off" -> encerra */
#endif
    for (;;) { }
}

static void imprime_codigo(int32_t codigo) {
    char b[16];
    escreve("[codigo de saida: ");
    zl_formata_int(codigo, b);
    escreve(b);
    escreve("]\n");
}

void zl_erro_execucao(int32_t codigo, int32_t linha) {
    char b[16];
    escreve("\n[ERRO DE EXECUCAO] ");
    escreve(zl_mensagem_erro(codigo));
    if (linha > 0) { escreve(" (linha "); zl_formata_int(linha, b); escreve(b); escreve(")"); }
    escreve("\n");
    imprime_codigo(100 + codigo);      /* mesma linha final que o PC mostra */
    DDRB |= (1 << PB5);                /* sinal fisico: LED do pino 13 piscando rapido */
#ifdef ZL_SIMULADOR
    for (int i = 0; i < 3; i++) {
#else
    for (;;) {
#endif
        PORTB ^= (1 << PB5);
        _delay_ms(100);
    }
    parar();
    for (;;) { }
}

/* avr-libc tem sqrtf na libm, mas powf e so uma MACRO para pow (float e double
 * tem 32 bits la); o llc chama o simbolo powf, entao ele e definido aqui. */
#undef powf
float powf(float x, float y) { return (float)pow((double)x, (double)y); }

int main(void) {
    serial_iniciar();
    int32_t r = zl_principal();        /* o programa Macaronica */
    imprime_codigo(r & 0xFF);          /* no PC o "respost" da principal vira o codigo de saida */
    parar();
    return 0;
}
