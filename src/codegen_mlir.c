/* codegen_mlir.c - Emissao dos nucleos vetoriais da DSL em MLIR (PjBL 4, Parte B)
 *
 * Estrategia HIBRIDA (docs/dsl.md, secao 4): o programa inteiro continua em
 * LLVM IR (codegen_llvm.c) e so cada operacao vetorial vira uma funcao MLIR
 * escrita com dialetos padrao (func, arith, memref, affine). O script
 * scripts/pipeline.sh rebaixa esse arquivo ate LLVM IR e o llvm-link junta
 * os dois modulos. No LLVM IR principal o nucleo aparece so como uma chamada:
 *     call void @dsl.soma.1(ptr %a.addr, ptr %b.addr, ptr %c.addr)
 * (convencao "bare pointer": cada memref de tamanho fixo vira um ptr simples). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"

/* ---------- Buf ---------- */
void buf_iniciar(Buf *b) {
    b->capacidade = 4096;
    b->usado = 0;
    b->dados = malloc(b->capacidade);
    b->dados[0] = '\0';
}

void buf_printf(Buf *b, const char *fmt, ...) {
    va_list ap;
    for (;;) {
        size_t livre = b->capacidade - b->usado;
        va_start(ap, fmt);
        int n = vsnprintf(b->dados + b->usado, livre, fmt, ap);
        va_end(ap);
        if (n < 0) return;
        if ((size_t)n < livre) { b->usado += (size_t)n; return; }   // coube
        while (b->capacidade - b->usado <= (size_t)n) b->capacidade *= 2;   // nao coube: cresce e tenta de novo
        b->dados = realloc(b->dados, b->capacidade);
    }
}

void buf_anexar(Buf *b, const Buf *outro) { buf_printf(b, "%s", outro->dados); }

void buf_liberar(Buf *b) { free(b->dados); b->dados = NULL; b->usado = b->capacidade = 0; }

/* ---------- Tipos ---------- */
const char *mlir_tipo(Tipo t, int rebaixa_duplo) {
    switch (t) {
        case TIPO_INT: return "i32";
        case TIPO_FLOAT: return "f32";
        case TIPO_DOUBLE: return rebaixa_duplo ? "f32" : "f64";   // Uno: sem double de 64 bits
        default: return "i32";
    }
}

static int eh_real(Tipo t) { return t == TIPO_FLOAT || t == TIPO_DOUBLE; }

// Converte o valor %de (tipo 'de') para o tipo 'para'; escreve o novo nome em 'saida'
static void converter(Buf *b, const char *de_val, Tipo de, Tipo para, const char *saida, int rd) {
    const char *td = mlir_tipo(de, rd), *tp = mlir_tipo(para, rd);
    if (!eh_real(de)) buf_printf(b, "      %s = arith.sitofp %s : %s to %s\n", saida, de_val, td, tp);  // inteira -> real
    else buf_printf(b, "      %s = arith.extf %s : %s to %s\n", saida, de_val, td, tp);                // flut -> duplocarpado
}

// Garante que o valor 'val' (tipo 'de') esteja no tipo 'para'; devolve o nome a usar
static const char *promover(Buf *b, const char *val, Tipo de, Tipo para, const char *novo, int rd) {
    if (strcmp(mlir_tipo(de, rd), mlir_tipo(para, rd)) == 0) return val;
    converter(b, val, de, para, novo, rd);
    return novo;
}

static const char *op_arith(char op, Tipo t) {
    int r = eh_real(t);
    switch (op) {
        case '+': return r ? "arith.addf" : "arith.addi";
        case '-': return r ? "arith.subf" : "arith.subi";
        default:  return r ? "arith.mulf" : "arith.muli";
    }
}

/* ---------- Emissao de um nucleo ---------- */
void mlir_nucleo(Buf *b, const Nucleo *k, int rd) {
    int n = k->n;
    const char *tres = mlir_tipo(k->tres, rd);
    buf_printf(b, "  // nucleo %d: ", k->id);

    if (k->tipo == NUC_ELEMENTO) {
        // c{i} = a{i} op b{i}   (ou k op a{i} / a{i} op k)
        buf_printf(b, "elemento a elemento '%c' com %d elementos\n", k->op, n);
        buf_printf(b, "  func.func @%s(", k->nome);
        if (k->esq_vetor) buf_printf(b, "%%a: memref<%dx%s>", n, mlir_tipo(k->te, rd));
        else buf_printf(b, "%%a: %s", tres);                         // escalar ja chega no tipo do resultado
        if (k->dir_vetor) buf_printf(b, ", %%b: memref<%dx%s>", n, mlir_tipo(k->td, rd));
        else buf_printf(b, ", %%b: %s", tres);
        buf_printf(b, ", %%c: memref<%dx%s>) attributes {passthrough = [\"nounwind\"]} {\n", n, mlir_tipo(k->tdest, rd));
        buf_printf(b, "    affine.for %%i = 0 to %d {\n", n);
        const char *x = "%a", *y = "%b";
        if (k->esq_vetor) {
            buf_printf(b, "      %%x = affine.load %%a[%%i] : memref<%dx%s>\n", n, mlir_tipo(k->te, rd));
            x = promover(b, "%x", k->te, k->tres, "%xc", rd);
        }
        if (k->dir_vetor) {
            buf_printf(b, "      %%y = affine.load %%b[%%i] : memref<%dx%s>\n", n, mlir_tipo(k->td, rd));
            y = promover(b, "%y", k->td, k->tres, "%yc", rd);
        }
        buf_printf(b, "      %%r = %s %s, %s : %s\n", op_arith(k->op, k->tres), x, y, tres);
        const char *r = promover(b, "%r", k->tres, k->tdest, "%rc", rd);
        buf_printf(b, "      affine.store %s, %%c[%%i] : memref<%dx%s>\n", r, n, mlir_tipo(k->tdest, rd));
        buf_printf(b, "    }\n    return\n  }\n\n");
        return;
    }

    if (k->tipo == NUC_PRODESC) {
        // soma de a{i} * b{i}, com acumulador carregado pelo laco (iter_args)
        buf_printf(b, "produto escalar com %d elementos\n", n);
        buf_printf(b, "  func.func @%s(%%a: memref<%dx%s>, %%b: memref<%dx%s>) -> %s attributes {passthrough = [\"nounwind\"]} {\n",
                   k->nome, n, mlir_tipo(k->te, rd), n, mlir_tipo(k->td, rd), tres);
        buf_printf(b, "    %%zero = arith.constant %s : %s\n", eh_real(k->tres) ? "0.0" : "0", tres);
        buf_printf(b, "    %%s = affine.for %%i = 0 to %d iter_args(%%acc = %%zero) -> (%s) {\n", n, tres);
        buf_printf(b, "      %%x = affine.load %%a[%%i] : memref<%dx%s>\n", n, mlir_tipo(k->te, rd));
        buf_printf(b, "      %%y = affine.load %%b[%%i] : memref<%dx%s>\n", n, mlir_tipo(k->td, rd));
        const char *x = promover(b, "%x", k->te, k->tres, "%xc", rd);
        const char *y = promover(b, "%y", k->td, k->tres, "%yc", rd);
        buf_printf(b, "      %%p = %s %s, %s : %s\n", op_arith('*', k->tres), x, y, tres);
        buf_printf(b, "      %%novo = %s %%acc, %%p : %s\n", op_arith('+', k->tres), tres);
        buf_printf(b, "      affine.yield %%novo : %s\n    }\n    return %%s : %s\n  }\n\n", tres, tres);
        return;
    }

    // Reducoes de um vetor: soma (comeca em 0) ou maximo (comeca no primeiro elemento)
    const char *te = mlir_tipo(k->te, rd);
    int maximo = (k->tipo == NUC_MAXIMO);
    buf_printf(b, "%s de %d elementos\n", maximo ? "maximo" : "soma", n);
    buf_printf(b, "  func.func @%s(%%a: memref<%dx%s>) -> %s attributes {passthrough = [\"nounwind\"]} {\n",
               k->nome, n, te, te);
    if (maximo) {
        buf_printf(b, "    %%c0 = arith.constant 0 : index\n");
        buf_printf(b, "    %%primeiro = memref.load %%a[%%c0] : memref<%dx%s>\n", n, te);
        buf_printf(b, "    %%s = affine.for %%i = 1 to %d iter_args(%%acc = %%primeiro) -> (%s) {\n", n, te);
    } else {
        buf_printf(b, "    %%zero = arith.constant %s : %s\n", eh_real(k->te) ? "0.0" : "0", te);
        buf_printf(b, "    %%s = affine.for %%i = 0 to %d iter_args(%%acc = %%zero) -> (%s) {\n", n, te);
    }
    buf_printf(b, "      %%x = affine.load %%a[%%i] : memref<%dx%s>\n", n, te);
    if (maximo) buf_printf(b, "      %%novo = %s %%acc, %%x : %s\n", eh_real(k->te) ? "arith.maximumf" : "arith.maxsi", te);
    else buf_printf(b, "      %%novo = %s %%acc, %%x : %s\n", op_arith('+', k->te), te);
    buf_printf(b, "      affine.yield %%novo : %s\n    }\n    return %%s : %s\n  }\n\n", te, te);
}
