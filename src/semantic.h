/* semantic.h - Analisador Semantico da linguagem Macaronica */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"         // o semantico percorre a AST (Node, Tipo)

#define MAX_PARAMS 16       // maximo de parametros guardados por funcao

// Uma entrada da tabela de simbolos (variavel, vetor ou funcao)
typedef struct Simbolo {
    char nome[64];
    Tipo tipo;
    int is_vetor;                   // 1 se foi declarado com vet{...}
    int is_funcao;                  // 1 se e funcao -> funcoes e variaveis moram na MESMA tabela
    int n_params;                   // quantidade de parametros (so para funcoes)
    Tipo param_tipos[MAX_PARAMS];   // tipo de cada parametro, na ordem
    Tipo tipo_retorno;
    struct Simbolo *proximo;        // proximo simbolo do mesmo escopo (lista encadeada)
} Simbolo;

// Um nivel de escopo; os niveis formam uma PILHA ligada pelo ponteiro pai
typedef struct Escopo {
    Simbolo *simbolos;              // simbolos declarados neste nivel
    struct Escopo *pai;             // escopo "de fora" (NULL no global)
} Escopo;

typedef struct {
    Escopo *escopo_atual;           // topo da pilha
    Escopo *escopo_global;          // base da pilha
    int erros_semanticos;           // contador; quem decide abortar e o main
    Tipo tipo_retorno_funcao_atual; /* contexto da funcao sendo verificada (usado pelo respost) */
    int viu_principal;              // quantas funcoes principal existem (tem que ser exatamente 1)
} Semantico;

void semantico_iniciar(Semantico *sem);
void semantico_verificar(Semantico *sem, Node *raiz);   // unica funcao que o main chama

#endif
