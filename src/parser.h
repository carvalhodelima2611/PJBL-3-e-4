/* parser.h - AST e Analisador Sintatico da linguagem Macaronica */
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"      // o parser usa Token, TipoToken e ListaTokens do lexer

// Tipos da linguagem como o parser/semantico os enxergam
// (TOK_INTEIRA e o TOKEN; TIPO_INT e o TIPO guardado na arvore)
typedef enum {
    TIPO_INT, TIPO_FLOAT, TIPO_DOUBLE, TIPO_BOOL, TIPO_STRING, TIPO_VOID,
    TIPO_REGISTRO,      // variavel de um tipo registro (o nome do registro fica em Node.nome_reg)
    TIPO_INDEFINIDO     // "ainda nao sei" ou "deu erro antes" -- evita erros em cascata no semantico
} Tipo;

// Todos os tipos de no que a arvore sintatica (AST) pode ter
typedef enum {
    ND_PROGRAMA,                        // raiz da arvore
    ND_REGISTRO, ND_CAMPO,
    ND_FUNCAO, ND_PARAM,
    ND_DECL_VAR, ND_DECL_VETOR,         // declaracoes (dentro de funcao = local; no topo do arquivo = global)
    ND_DECL_REG,                        // variavel de registro:  Aluno a;
    ND_ATRIB_CAMPO,                     // a.idade receba ...;
    ND_ATRIB, ND_ATRIB_VETOR,           // x receba ... / v{i} receba ...
    ND_COND, ND_REPETE, ND_DURANTE,     // if / for / while
    ND_RESPOST, ND_CHAMADA_STMT,        // return / chamada usada como comando

    /* nos de expressao */
    ND_NUM, ND_STRING_LIT, ND_BOOL_LIT, ND_VAR, ND_ACESSO_VETOR,
    ND_CHAMADA_EXPR, ND_RAIZ_EXPR, ND_BINOP,
    ND_CAMPO_EXPR,                      // leitura de campo: a.idade
    ND_COMPARACAO, ND_LOGICO_BIN, ND_LOGICO_NOT   // nos de condicao
} NodeKind;

// Um UNICO tipo de no para a arvore toda; o significado dos campos muda conforme o kind
typedef struct Node {
    NodeKind kind;          // que tipo de no e este
    int linha;              // linha do codigo-fonte (para mensagens de erro)
    char nome[64];          // identificador, nome de funcao, ou operador ("+", "<", "OU")
    Tipo tipo_declarado;    // tipo ESCRITO no codigo (declaracao ou retorno de funcao)
    Tipo tipo_resultado;    // tipo do VALOR (so preenchido para literais; o resto o semantico calcula)
    double num_valor;       // valor de numero / tamanho do vetor / 1 se for a funcao principal
    char str_valor[256];    // conteudo de string literal
    char nome_reg[64];      // nome do tipo registro (ND_DECL_REG, ND_CAMPO_EXPR, ND_ATRIB_CAMPO)
    char campo[64];         // nome do campo (ND_CAMPO_EXPR, ND_ATRIB_CAMPO)
    int aux;                // anotado pelo semantico: indice do campo no registro
    int eh_vetorial;        // anotado pelo semantico: ND_ATRIB que e uma operacao vetorial da DSL
    int id_nucleo;          // anotado pelo gerador: numero do nucleo MLIR desta operacao vetorial
    struct Node *filho1, *filho2, *filho3;   // filhos fixos (ex: os dois lados de um "+")
    struct Node *lista;     // inicio de uma lista (comandos do bloco, parametros, argumentos)
    struct Node *prox;      // proximo elemento da lista em que ESTE no esta
} Node;

// Estado do parser
typedef struct {
    ListaTokens tokens;     // tokens que vieram do lexer
    int pos;                // indice do TOKEN atual (no lexer, pos era indice de caractere)
    int erros_sintaticos;   // contador; quem decide abortar e o main
} Parser;

void parser_iniciar(Parser *p, ListaTokens tokens);
Node *parser_analisar(Parser *p);           // entrada principal: devolve a raiz da AST
Node *criar_no(NodeKind kind, int linha);
const char *tipo_nome(Tipo t);              // TIPO_INT -> "inteira" (mensagens de erro)

#endif
