/* semantic.h - Analisador Semantico da linguagem Macaronica */
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"         // o semantico percorre a AST (Node, Tipo)

#define MAX_PARAMS 16       // maximo de parametros guardados por funcao
#define MAX_CAMPOS 32       // maximo de campos por registro

// Alvo de compilacao (--alvo=...). Muda so a verificacao de memoria e o duplocarpado.
typedef enum { ALVO_PC, ALVO_UNO, ALVO_RPI } Alvo;

// Funcoes embutidas: ja existem na tabela de simbolos antes do programa comecar
typedef enum {
    BI_NENHUM = 0,
    BI_MOSTRAR,         // mostrar(a, b, ...)  imprime e pula linha
    BI_ESCREVE,         // escreve(a, b, ...)  imprime sem pular linha
    BI_NOVALINHA,       // novalinha()
    BI_LEINTEIRA,       // leinteira()      -> inteira
    BI_LEFLUT,          // leflut()         -> flut
    BI_LEDUPLO,         // leduplocarpado() -> duplocarpado
    BI_PINO_MODO,       // pino_modo(pino, modo)      E/S fisica (PjBL 4, Parte D)
    BI_PINO_ESCREVE,    // pino_escreve(pino, valor)
    BI_PINO_LE,         // pino_le(pino)    -> inteira
    BI_ESPERA,          // espera(ms)
    BI_SOMAVET,         // somavet(v)       DSL vetorial (PjBL 4, Parte A)
    BI_MAXVET,          // maxvet(v)
    BI_PRODESC          // prodesc(a, b)
} Embutida;

Embutida embutida_por_nome(const char *nome);   // "mostrar" -> BI_MOSTRAR (BI_NENHUM se nao for embutida)

// Uma entrada da tabela de simbolos (variavel, vetor, registro ou funcao)
typedef struct Simbolo {
    char nome[64];
    Tipo tipo;                      // tipo da variavel (ou do ELEMENTO, se for vetor)
    int is_vetor;                   // 1 se foi declarado com vet{...}
    int tam_vetor;                  // tamanho do vetor (conhecido em compilacao)
    char nome_reg[64];              // se tipo == TIPO_REGISTRO: qual registro
    int is_funcao;                  // 1 se e funcao -> funcoes e variaveis moram na MESMA tabela
    Embutida embutida;              // != BI_NENHUM para funcoes embutidas
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

// Definicao de um tipo registro (tabela separada: registro e TIPO, nao variavel)
typedef struct DefRegistro {
    char nome[64];
    int n_campos;
    char campo_nome[MAX_CAMPOS][64];
    Tipo campo_tipo[MAX_CAMPOS];
    struct DefRegistro *proximo;
} DefRegistro;

typedef struct {
    Escopo *escopo_atual;           // topo da pilha
    Escopo *escopo_global;          // base da pilha
    int erros_semanticos;           // contador; quem decide abortar e o main
    Tipo tipo_retorno_funcao_atual; /* contexto da funcao sendo verificada (usado pelo respost) */
    int viu_principal;              // quantas funcoes principal existem (tem que ser exatamente 1)
    DefRegistro *registros;         // tipos registro declarados
    Alvo alvo;                      // alvo da compilacao
    long bytes_globais;             // memoria ocupada pelas variaveis globais no alvo
    long bytes_locais;              // memoria das variaveis locais da funcao sendo verificada
    int em_funcao;                  // 1 enquanto verifica o corpo de uma funcao
    int usa_duplo;                  // 1 se o programa usa duplocarpado (aviso no Uno)
} Semantico;

void semantico_iniciar(Semantico *sem, Alvo alvo);
void semantico_verificar(Semantico *sem, Node *raiz);   // unica funcao que o main chama
void semantico_liberar(Semantico *sem);                 // libera a tabela de registros

// Usados tambem pelo gerador de codigo (mesmas regras de tipo nos dois lugares)
int rank_numerico(Tipo t);
int eh_numerico(Tipo t);
Tipo tipo_mais_largo(Tipo a, Tipo b);
DefRegistro *buscar_registro(const DefRegistro *lista, const char *nome);
long ram_do_alvo(Alvo alvo);    // bytes de RAM (0 = sem limite verificado)

#endif
