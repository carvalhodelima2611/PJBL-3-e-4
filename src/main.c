/* main.c - Front-end do compilador Macaronica
 * Uso: ./macaronica arquivo.mac   ou   ./macaronica < arquivo.mac
 * Roda lexico -> sintatico -> semantico e diz onde e por que falhou.
 * Nao ha geracao de codigo. */
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

// Le o arquivo inteiro para a memoria, sem limite de tamanho
// (o codigo original tinha um buffer fixo de 1024 bytes e cortava programas maiores)
static char *ler_entrada(FILE *f) {
    size_t capacidade = 4096;
    size_t usado = 0;
    char *buffer = malloc(capacidade);
    if (!buffer) return NULL;

    size_t lidos;
    while ((lidos = fread(buffer + usado, 1, capacidade - usado, f)) > 0) {   // le o que cabe
        usado += lidos;
        if (usado == capacidade) {                  // encheu: dobra o buffer
            capacidade *= 2;
            char *novo = realloc(buffer, capacidade);
            if (!novo) { free(buffer); return NULL; }   // sem memoria: libera e desiste
            buffer = novo;
        }
    }
    buffer[usado] = '\0';       // marca o fim do texto (o lexer para no '\0'); sempre sobra espaco para ele
    return buffer;
}

// Libera a AST inteira: primeiro todos os filhos, depois o proprio no
static void liberar_ast(Node *no) {
    if (!no) return;
    liberar_ast(no->filho1);
    liberar_ast(no->filho2);
    liberar_ast(no->filho3);
    liberar_ast(no->lista);
    liberar_ast(no->prox);
    free(no);                   // se liberasse antes, perderia os ponteiros para os filhos
}

int main(int argc, char *argv[]) {
    char *codigo = NULL;

    if (argc >= 2) {                                // recebeu nome de arquivo?
        FILE *f = fopen(argv[1], "rb");             // "rb": le os bytes como estao (importante para o NÃO em UTF-8)
        if (!f) {
            fprintf(stderr, "ERRO: nao foi possivel abrir o arquivo '%s'\n", argv[1]);
            return 1;
        }
        codigo = ler_entrada(f);
        fclose(f);
    } else {
        codigo = ler_entrada(stdin);                // senao, le da entrada padrao
    }

    if (!codigo) {
        fprintf(stderr, "ERRO: falha ao ler a entrada (memoria insuficiente?)\n");
        return 1;
    }
    if (codigo[0] == '\0') {                        // arquivo vazio: para antes do lexer
        fprintf(stderr, "ERRO: entrada vazia\n");
        free(codigo);
        return 1;
    }

    printf("=== Front-end Macaronica ===\n");       // resumo vai para stdout; erros detalhados vao para stderr

    /* ---------- Fase 1: Analise Lexica ---------- */
    ListaTokens tokens = lexer_tokenizar(codigo);
    if (tokens.itens[0].tipo != TOK_FIM) { /* garante array valido mesmo se vazio */ }   // (nao faz nada; sobra do codigo)

    // Conta erros lexicos procurando TOK_INVALIDO na lista
    // (o Lexer era variavel local de lexer_tokenizar e ja nao existe mais aqui)
    int erros_lexicos = 0;
    for (int i = 0; i < tokens.quantidade; i++) {
        if (tokens.itens[i].tipo == TOK_INVALIDO) erros_lexicos++;
    }

    if (erros_lexicos > 0) {                        // erro lexico: nem tenta o parser
        printf("> Analise Lexica: FALHOU (%d erro(s) lexico(s) encontrado(s) acima).\n", erros_lexicos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase lexica.\n");
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;                                   // codigo de saida 1 = invalido (usado pelo make test)
    }
    printf("> Analise Lexica: OK (%d tokens reconhecidos).\n", tokens.quantidade - 1);   // -1: nao conta o TOK_FIM

    /* ---------- Fase 2: Analise Sintatica ---------- */
    Parser parser;
    parser_iniciar(&parser, tokens);
    Node *ast = parser_analisar(&parser);           // monta a arvore

    if (parser.erros_sintaticos > 0) {              // erro sintatico: nao roda o semantico
        printf("> Analise Sintatica: FALHOU (%d erro(s) sintatico(s) encontrado(s) acima).\n",
               parser.erros_sintaticos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase sintatica.\n");
        liberar_ast(ast);                           // (arvore incompleta geraria erros semanticos falsos)
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;
    }
    printf("> Analise Sintatica: OK (arvore sintatica construida sem erros).\n");

    /* ---------- Fase 3: Analise Semantica ---------- */
    Semantico sem;
    semantico_iniciar(&sem);
    semantico_verificar(&sem, ast);                 // percorre a arvore

    if (sem.erros_semanticos > 0) {
        printf("> Analise Semantica: FALHOU (%d erro(s) semantico(s) encontrado(s) acima).\n",
               sem.erros_semanticos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase semantica.\n");
        liberar_ast(ast);
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;
    }
    printf("> Analise Semantica: OK (tipos, escopo e assinaturas de funcao verificados).\n");
    printf(">> Programa Macaronica VALIDO (lexica, sintatica e semanticamente).\n");

    liberar_ast(ast);                               // libera tudo antes de sair
    lista_tokens_destruir(&tokens);
    free(codigo);
    return 0;                                       // 0 = programa valido
}
