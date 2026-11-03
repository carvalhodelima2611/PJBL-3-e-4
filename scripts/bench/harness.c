/* harness.c - cronometra os nucleos vetoriais gerados pelo compilador Macaronica.
 * Os nucleos vem do programa scripts/bench/bench.mac (via macaronica + pipeline.sh);
 * este arquivo so prepara os dados, chama cada nucleo e mede o tempo.
 * Saida (uma linha por nucleo): nome  mediana_ms  resultado
 * Compilar com -DN=<tamanho>. Os nomes dos nucleos tem ponto (dsl.soma.1), por isso
 * a declaracao usa __asm__ para dar o nome do simbolo. */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifndef N
#define N 1000000
#endif
#ifndef REP
#define REP 7            /* chamadas cronometradas por nucleo (mais 1 de aquecimento) */
#endif

extern void    soma_int(int32_t *a, int32_t *b, int32_t *c) __asm__("dsl.soma.1");
extern int32_t somavet_int(int32_t *a)                       __asm__("dsl.somavet.2");
extern void    soma_flut(float *a, float *b, float *c)      __asm__("dsl.soma.3");
extern float   somavet_flut(float *a)                        __asm__("dsl.somavet.4");
extern float   prodesc_flut(float *a, float *b)              __asm__("dsl.prodesc.5");

static double agora_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}
static int cmp(const void *x, const void *y) {
    double a = *(const double *)x, b = *(const double *)y; return (a > b) - (a < b);
}
static double mediana(double *v, int n) { qsort(v, n, sizeof(double), cmp); return v[n / 2]; }

static void *aloca(size_t bytes) {
    void *p = NULL;
    if (posix_memalign(&p, 64, bytes) != 0) { fprintf(stderr, "sem memoria\n"); exit(1); }
    return p;
}

#define MEDE(nome, chamada, fmt, res)                                        \
    do {                                                                     \
        double t[REP]; chamada;                                              \
        for (int r = 0; r < REP; r++) { double t0 = agora_ms(); chamada; t[r] = agora_ms() - t0; } \
        printf("%-14s %10.4f  " fmt "\n", nome, mediana(t, REP), res);       \
    } while (0)

int main(void) {
    int32_t *a = aloca(N * 4), *b = aloca(N * 4), *c = aloca(N * 4);
    float *fa = aloca(N * 4), *fb = aloca(N * 4), *fc = aloca(N * 4);
    for (long i = 0; i < N; i++) {
        a[i] = (int32_t)(i % 1000); b[i] = (int32_t)(i % 7);
        fa[i] = 0.1f * (float)(i % 100); fb[i] = 1.0f / (float)(1 + i % 13);
    }
    volatile int32_t si = 0; volatile float sf = 0;
    MEDE("soma_int",     soma_int(a, b, c),           "%d", (int)c[N - 1]);
    MEDE("somavet_int",  si = somavet_int(a),         "%d", (int)si);
    MEDE("soma_flut",    soma_flut(fa, fb, fc),       "%.6f", (double)fc[N - 1]);
    MEDE("somavet_flut", sf = somavet_flut(fa),       "%.6f", (double)sf);
    MEDE("prodesc_flut", sf = prodesc_flut(fa, fb),   "%.6f", (double)sf);
    return 0;
}
