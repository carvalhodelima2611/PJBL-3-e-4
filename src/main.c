/* main.c - Compilador Macaronica
 * Uso:
 *   ./macaronica entrada.mac                       so valida (comportamento do PjBL 1)
 *   ./macaronica entrada.mac -o saida.ll           valida e gera LLVM IR (PjBL 3)
 *   ./macaronica entrada.mac -o saida.ll --alvo=uno   alvo da placa (verificacao de memoria)
 * Se o programa usa operacoes vetoriais, gera tambem saida.mlir (nucleos da DSL, PjBL 4).
 * Para entrada invalida: reporta os erros e NAO gera nenhum arquivo de saida. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

// Le o arquivo inteiro para a memoria, sem limite de tamanho
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
            if (!novo) { free(buffer); return NULL; }
            buffer = novo;
        }
    }
    buffer[usado] = '\0';       // marca o fim do texto (o lexer para no '\0')
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
    free(no);
}

static int escrever_arquivo(const char *caminho, const Buf *b) {
    FILE *f = fopen(caminho, "wb");
    if (!f) { fprintf(stderr, "ERRO: nao foi possivel criar '%s'\n", caminho); return 0; }
    fwrite(b->dados, 1, b->usado, f);
    fclose(f);
    return 1;
}

static void uso(void) {
    fprintf(stderr, "uso: macaronica entrada.mac [-o saida.ll] [--alvo=pc|uno|rpi]\n");
}

int main(int argc, char *argv[]) {
    const char *entrada = NULL, *saida = NULL;
    Alvo alvo = ALVO_PC;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) saida = argv[++i];
        else if (strncmp(argv[i], "--alvo=", 7) == 0) {
            const char *a = argv[i] + 7;
            if (strcmp(a, "pc") == 0) alvo = ALVO_PC;
            else if (strcmp(a, "uno") == 0) alvo = ALVO_UNO;
            else if (strcmp(a, "rpi") == 0) alvo = ALVO_RPI;
            else { fprintf(stderr, "ERRO: alvo desconhecido '%s' (use pc, uno ou rpi)\n", a); return 1; }
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') { uso(); return 1; }
        else entrada = argv[i];
    }

    // Nome do .mlir (mesmo nome do .ll com outra extensao) e remocao de saidas antigas:
    // um programa invalido nunca deixa um .ll/.mlir velho para tras
    char saida_mlir[1024] = "";
    if (saida) {
        snprintf(saida_mlir, sizeof(saida_mlir), "%s", saida);
        char *ponto = strrchr(saida_mlir, '.');
        char *barra = strrchr(saida_mlir, '/');
        if (ponto && (!barra || ponto > barra)) *ponto = '\0';
        strncat(saida_mlir, ".mlir", sizeof(saida_mlir) - strlen(saida_mlir) - 1);
        remove(saida);
        remove(saida_mlir);
    }

    char *codigo = NULL;
    if (entrada) {
        FILE *f = fopen(entrada, "rb");             // "rb": le os bytes como estao (NÃO em UTF-8)
        if (!f) {
            fprintf(stderr, "ERRO: nao foi possivel abrir o arquivo '%s'\n", entrada);
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
    if (codigo[0] == '\0') {
        fprintf(stderr, "ERRO: entrada vazia\n");
        free(codigo);
        return 1;
    }

    printf("=== Front-end Macaronica ===\n");       // resumo vai para stdout; erros detalhados vao para stderr

    /* ---------- Fase 1: Analise Lexica ---------- */
    ListaTokens tokens = lexer_tokenizar(codigo);
    int erros_lexicos = 0;
    for (int i = 0; i < tokens.quantidade; i++) {
        if (tokens.itens[i].tipo == TOK_INVALIDO) erros_lexicos++;
    }
    if (erros_lexicos > 0) {
        printf("> Analise Lexica: FALHOU (%d erro(s) lexico(s) encontrado(s) acima).\n", erros_lexicos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase lexica.\n");
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;
    }
    printf("> Analise Lexica: OK (%d tokens reconhecidos).\n", tokens.quantidade - 1);

    /* ---------- Fase 2: Analise Sintatica ---------- */
    Parser parser;
    parser_iniciar(&parser, tokens);
    Node *ast = parser_analisar(&parser);
    if (parser.erros_sintaticos > 0) {
        printf("> Analise Sintatica: FALHOU (%d erro(s) sintatico(s) encontrado(s) acima).\n",
               parser.erros_sintaticos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase sintatica.\n");
        liberar_ast(ast);
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;
    }
    printf("> Analise Sintatica: OK (arvore sintatica construida sem erros).\n");

    /* ---------- Fase 3: Analise Semantica ---------- */
    Semantico sem;
    semantico_iniciar(&sem, alvo);
    semantico_verificar(&sem, ast);
    if (sem.erros_semanticos > 0) {
        printf("> Analise Semantica: FALHOU (%d erro(s) semantico(s) encontrado(s) acima).\n",
               sem.erros_semanticos);
        printf(">> Programa INVALIDO. Compilacao abortada na fase semantica.\n");
        semantico_liberar(&sem);
        liberar_ast(ast);
        lista_tokens_destruir(&tokens);
        free(codigo);
        return 1;
    }
    printf("> Analise Semantica: OK (tipos, escopo e assinaturas de funcao verificados).\n");
    printf(">> Programa Macaronica VALIDO (lexica, sintatica e semanticamente).\n");
    if (alvo == ALVO_UNO && sem.usa_duplo) {
        fprintf(stderr, "AVISO: o alvo 'uno' nao tem double de 64 bits; duplocarpado sera gerado como float (32 bits)\n");
    }

    /* ---------- Fase 4: Geracao de codigo (so com -o) ---------- */
    int rc = 0;
    if (saida) {
        Buf ll, mlir;
        buf_iniciar(&ll);
        buf_iniciar(&mlir);
        buf_printf(&mlir, "// Nucleos vetoriais gerados pelo compilador Macaronica a partir de '%s'\n"
                          "// Rebaixar com scripts/pipeline.sh (affine -> scf/cf -> dialeto llvm -> LLVM IR)\n"
                          "module {\n", entrada ? entrada : "<stdin>");
        int nucleos = gerar_llvm(ast, &sem, entrada ? entrada : "<stdin>", &ll, &mlir);
        buf_printf(&mlir, "}\n");
        if (!escrever_arquivo(saida, &ll)) rc = 1;
        else printf("> Geracao de codigo: LLVM IR escrito em '%s'\n", saida);
        if (rc == 0 && nucleos > 0) {
            if (!escrever_arquivo(saida_mlir, &mlir)) rc = 1;
            else printf("> Geracao de codigo: %d nucleo(s) vetorial(is) MLIR escrito(s) em '%s'\n", nucleos, saida_mlir);
        }
        buf_liberar(&ll);
        buf_liberar(&mlir);
    }

    semantico_liberar(&sem);
    liberar_ast(ast);
    lista_tokens_destruir(&tokens);
    free(codigo);
    return rc;
}
