/* codegen.h - Geracao de codigo (PjBL 3: LLVM IR; PjBL 4: nucleos vetoriais em MLIR)
 *
 * O gerador so roda DEPOIS que lexico, sintatico e semantico aprovaram o
 * programa: ele percorre a AST ja ANOTADA pelo semantico (tipo_resultado de
 * cada expressao, indice de campo, flag eh_vetorial) e nao refaz nenhuma
 * verificacao de tipos. Por isso nunca falha: se chegou aqui, o programa e valido. */
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"
#include "semantic.h"

/* ---------- Buffer de texto que cresce sozinho (a IR e montada em memoria) ---------- */
typedef struct {
    char *dados;
    size_t usado;
    size_t capacidade;
} Buf;

void buf_iniciar(Buf *b);
void buf_printf(Buf *b, const char *fmt, ...);      // como fprintf, mas para o buffer
void buf_anexar(Buf *b, const Buf *outro);
void buf_liberar(Buf *b);

/* ---------- Nucleos vetoriais (DSL) emitidos em MLIR ---------- */
typedef enum { NUC_ELEMENTO, NUC_SOMA, NUC_MAXIMO, NUC_PRODESC } TipoNucleo;

typedef struct {
    int id;
    TipoNucleo tipo;
    char op;                // '+', '-' ou '*' (so NUC_ELEMENTO)
    int n;                  // tamanho dos vetores (sempre conhecido em compilacao)
    int esq_vetor;          // 1 se o operando esquerdo e vetor (0 = escalar k)
    int dir_vetor;          // 1 se o operando direito e vetor
    Tipo te, td;            // tipo do elemento (ou do escalar) de cada lado
    Tipo tres;              // tipo do resultado da operacao (promocao)
    Tipo tdest;             // tipo do elemento do vetor destino (NUC_ELEMENTO)
    char nome[64];          // nome do simbolo: dsl.soma.3
} Nucleo;

void mlir_nucleo(Buf *b, const Nucleo *k, int rebaixa_duplo);   // emite um func.func
const char *mlir_tipo(Tipo t, int rebaixa_duplo);               // i32 / f32 / f64

/* ---------- Ponto de entrada do gerador ---------- */
// Gera o modulo LLVM IR em 'll' e, se houver operacoes vetoriais, os nucleos em 'mlir'.
// Devolve o numero de nucleos MLIR gerados.
int gerar_llvm(Node *raiz, const Semantico *sem, const char *arquivo_fonte, Buf *ll, Buf *mlir);

#endif
