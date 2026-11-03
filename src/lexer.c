/* lexer.c - Analisador Lexico da linguagem Macaronica
 * Decisoes de projeto: ver docs/decisoes.md (virgula e @ no alfabeto,
 * aspas duplas para strings, "!" como diferente, "NAO"/"NÃO"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // isspace, isalpha, isdigit, isalnum
#include "lexer.h"

// Prepara o lexer para comecar a ler do inicio do texto
void lexer_iniciar(Lexer *lx, const char *texto) {
    lx->texto = texto;
    lx->pos = 0;
    lx->linha_atual = 1;        // linhas e colunas comecam em 1 (como no editor)
    lx->coluna_atual = 1;
    lx->erros_lexicos = 0;
}

static char lx_atual(Lexer *lx) { return lx->texto[lx->pos]; }                  // devolve o CARACTERE atual (nao anda)
static char lx_ver(Lexer *lx, int offset) { return lx->texto[lx->pos + offset]; } // espia um caractere a frente (nao anda)

// Unica funcao que anda no texto -> por isso e a unica que atualiza linha/coluna
static void lx_avancar(Lexer *lx) {
    if (lx->texto[lx->pos] == '\0') return;   // nunca passa do fim do texto
    if (lx->texto[lx->pos] == '\n') {         // pulou de linha:
        lx->linha_atual++;                    //   soma uma linha
        lx->coluna_atual = 1;                 //   e volta para a coluna 1
    } else {
        lx->coluna_atual++;
    }
    lx->pos++;
}

// Caracteres que podem CONTINUAR um identificador: letra, digito, '_' ou byte >= 0x80.
// O ">= 0x80" existe para aceitar o "Ã" de "NÃO", que em UTF-8 ocupa dois bytes.
static int eh_continuacao_id(unsigned char c) {
    return isalnum(c) || c == '_' || c >= 0x80;
}

// Pula espacos e comentarios de bloco (barra-asterisco ... asterisco-barra).
// Nao usamos barra-barra como comentario porque "//" e a divisao inteira da linguagem.
static void pular_espacos_e_comentarios(Lexer *lx) {
    for (;;) {                                                    // repete porque espaco e comentario podem se alternar
        while (isspace((unsigned char)lx_atual(lx))) lx_avancar(lx);  // espacos, tabs, quebras de linha
        if (lx_atual(lx) == '/' && lx_ver(lx, 1) == '*') {        // inicio de comentario
            lx_avancar(lx); lx_avancar(lx);                       // consome a abertura (2 caracteres)
            while (lx_atual(lx) != '\0' && !(lx_atual(lx) == '*' && lx_ver(lx, 1) == '/')) {
                lx_avancar(lx);                                   // pula o conteudo do comentario
            }
            if (lx_atual(lx) != '\0') { lx_avancar(lx); lx_avancar(lx); }  // consome o fechamento
            // ⚠️ se o comentario nunca fechar, o resto do arquivo e ignorado sem gerar erro
            continue;                                             // pode ter mais espaco/comentario depois
        }
        break;                                                    // nem espaco nem comentario: acabou
    }
}

typedef struct { const char *palavra; TipoToken tipo; } EntradaPalavraChave;

// "Dicionario" de palavras reservadas: texto -> tipo de token
static const EntradaPalavraChave TABELA_PALAVRAS[] = {
    {"inteira", TOK_INTEIRA}, {"flut", TOK_FLUT}, {"bool", TOK_BOOL},
    {"palavra", TOK_PALAVRA}, {"duplocarpado", TOK_DUPLOCARPADO}, {"vazio", TOK_VAZIO},
    {"receba", TOK_RECEBA}, {"repete", TOK_REPETE}, {"durante", TOK_DURANTE},
    {"cond", TOK_COND}, {"casocontrario", TOK_CASOCONTRARIO},
    {"outrafuncao", TOK_OUTRAFUNCAO}, {"principal", TOK_PRINCIPAL},
    {"respost", TOK_RESPOST}, {"registro", TOK_REGISTRO}, {"edai", TOK_EDAI},
    {"vet", TOK_VET}, {"raiz", TOK_RAIZ}, {"func", TOK_FUNC},
    {"VER", TOK_VER}, {"FAL", TOK_FAL},
    {"OU", TOK_OU}, {"E", TOK_E}, {"XOR", TOK_XOR},
    {"NAO", TOK_NAO}, {"N\xC3\x83O", TOK_NAO}, /* "NÃO" em UTF-8 (bytes C3 83 = Ã) */
    {NULL, TOK_INVALIDO}    // sentinela: marca o fim da tabela
};

// Recebe uma palavra ja lida inteira e diz se ela e reservada.
// Ler a palavra toda antes evita que "condicao" seja cortado em "cond" + "icao".
static TipoToken identificar_palavra(const char *lexema) {
    for (int i = 0; TABELA_PALAVRAS[i].palavra != NULL; i++) {   // percorre ate a sentinela
        if (strcmp(lexema, TABELA_PALAVRAS[i].palavra) == 0) {
            return TABELA_PALAVRAS[i].tipo;                       // achou: e palavra reservada
        }
    }
    return TOK_IDENTIFICADOR;                                     // nao achou: e nome de variavel/funcao
}

static void lista_tokens_iniciar(ListaTokens *lst) {
    lst->capacidade = 64;                                         // comeca com espaco para 64 tokens
    lst->quantidade = 0;
    lst->itens = malloc(sizeof(Token) * lst->capacidade);
}

static void lista_tokens_adicionar(ListaTokens *lst, Token t) {
    if (lst->quantidade >= lst->capacidade) {                     // encheu?
        lst->capacidade *= 2;                                     // dobra (poucas realocacoes no total)
        lst->itens = realloc(lst->itens, sizeof(Token) * lst->capacidade);
    }
    lst->itens[lst->quantidade++] = t;                            // guarda e soma 1 na quantidade
}

void lista_tokens_destruir(ListaTokens *lst) {
    free(lst->itens);
    lst->itens = NULL;          // evita usar ponteiro ja liberado
    lst->quantidade = 0;
    lst->capacidade = 0;
}

// Le e devolve UM token. A ordem dos testes abaixo importa.
static Token proximo_token_bruto(Lexer *lx) {
    pular_espacos_e_comentarios(lx);           // 1) ignora espacos e comentarios

    Token t;
    memset(&t, 0, sizeof(Token));              // zera o token (lexema vazio, valor 0)
    t.linha = lx->linha_atual;                 // guarda ONDE o token comeca
    t.coluna = lx->coluna_atual;
    t.tipo = TOK_INVALIDO;

    unsigned char c = (unsigned char)lx_atual(lx);
    if (c == '\0') { t.tipo = TOK_FIM; strcpy(t.lexema, "FIM"); return t; }   // 2) fim do texto

    /* 3) Identificadores e palavras reservadas */
    if (isalpha(c) || c == '_' || c >= 0x80) {        // comeca com letra, '_' ou byte UTF-8
        int len = 0;
        while (eh_continuacao_id((unsigned char)lx_atual(lx))) {
            if (len < LEXEMA_MAX - 1) t.lexema[len++] = lx_atual(lx);   // copia sem estourar o vetor
            lx_avancar(lx);
        }
        t.lexema[len] = '\0';
        t.tipo = identificar_palavra(t.lexema);       // reservada ou identificador?
        return t;
    }

    /* 4) Numeros (inteiros e de ponto flutuante, um unico '.') */
    if (isdigit(c)) {
        int len = 0;
        int pontos = 0;
        // aceita digitos, e UM ponto so se vier um digito logo depois (3.14 sim, "3." nao)
        while (isdigit((unsigned char)lx_atual(lx)) ||
               (lx_atual(lx) == '.' && pontos == 0 && isdigit((unsigned char)lx_ver(lx, 1)))) {
            if (lx_atual(lx) == '.') pontos++;
            if (len < LEXEMA_MAX - 1) t.lexema[len++] = lx_atual(lx);
            lx_avancar(lx);
        }
        t.lexema[len] = '\0';
        t.tipo = TOK_NUMERO;
        t.valor_num = atof(t.lexema);                 // converte o texto em double
        return t;                                     // quem decide inteira/flut e o parser
    }

    /* 5) Literais de texto (tipo "palavra"): "..." */
    if (c == '"') {
        lx_avancar(lx);                               // pula a aspa de abertura
        int len = 0;
        int fechou = 0;
        while (lx_atual(lx) != '\0' && lx_atual(lx) != '\n') {   // string nao pode atravessar linha
            if (lx_atual(lx) == '"') { lx_avancar(lx); fechou = 1; break; }  // achou a aspa de fechamento
            if (len < LEXEMA_MAX - 1) t.lexema[len++] = lx_atual(lx);        // aspas NAO entram no lexema
            lx_avancar(lx);
        }
        t.lexema[len] = '\0';
        if (!fechou) {                                // chegou no fim da linha/arquivo sem fechar
            t.tipo = TOK_INVALIDO;
            lx->erros_lexicos++;
            fprintf(stderr, "[Linha %d, Coluna %d] ERRO LEXICO: string nao terminada: \"%s\n",
                    t.linha, t.coluna, t.lexema);
            return t;
        }
        t.tipo = TOK_STRING;
        return t;
    }

    /* 6) Operadores de dois caracteres -- testados ANTES dos de um caractere,
     *    senao "//" viraria duas divisoes "/" "/" */
    if (c == '=' && lx_ver(lx, 1) == '=') {
        strcpy(t.lexema, "=="); lx_avancar(lx); lx_avancar(lx); t.tipo = TOK_IGUAL; return t;
    }
    if (c == '/' && lx_ver(lx, 1) == '/') {
        strcpy(t.lexema, "//"); lx_avancar(lx); lx_avancar(lx); t.tipo = TOK_DIV_INT; return t;
    }

    /* 7) Operadores/pontuacao de um caractere */
    t.lexema[0] = (char)c; t.lexema[1] = '\0';
    switch (c) {
        case ';': t.tipo = TOK_PONTO_VIRGULA; lx_avancar(lx); return t;
        case ',': t.tipo = TOK_VIRGULA; lx_avancar(lx); return t;
        case '@': t.tipo = TOK_ARROBA; lx_avancar(lx); return t;
        case '[': t.tipo = TOK_ABRE_COLCHETE; lx_avancar(lx); return t;
        case ']': t.tipo = TOK_FECHA_COLCHETE; lx_avancar(lx); return t;
        case '{': t.tipo = TOK_ABRE_CHAVE; lx_avancar(lx); return t;
        case '}': t.tipo = TOK_FECHA_CHAVE; lx_avancar(lx); return t;
        case '(': t.tipo = TOK_ABRE_PARENTESE; lx_avancar(lx); return t;
        case ')': t.tipo = TOK_FECHA_PARENTESE; lx_avancar(lx); return t;
        case '+': t.tipo = TOK_MAIS; lx_avancar(lx); return t;
        case '-': t.tipo = TOK_MENOS; lx_avancar(lx); return t;
        case '*': t.tipo = TOK_VEZES; lx_avancar(lx); return t;
        case '/': t.tipo = TOK_DIVIDE; lx_avancar(lx); return t;
        case '^': t.tipo = TOK_POTENCIA; lx_avancar(lx); return t;
        case '>': t.tipo = TOK_MAIOR; lx_avancar(lx); return t;
        case '<': t.tipo = TOK_MENOR; lx_avancar(lx); return t;
        case '!': t.tipo = TOK_DIFERENTE; lx_avancar(lx); return t;   // "!" sozinho = diferente
        default: break;       // '=' sozinho, '%', '#', etc. nao estao aqui -> caem no erro abaixo
    }

    /* 8) Caractere nao pertence ao alfabeto da linguagem: erro lexico */
    t.tipo = TOK_INVALIDO;
    lx->erros_lexicos++;
    fprintf(stderr, "[Linha %d, Coluna %d] ERRO LEXICO: caractere invalido '%c' (0x%02X)\n",
            t.linha, t.coluna, (char)c, c);
    lx_avancar(lx);           // avanca mesmo com erro -> continua e acha os proximos erros
    return t;
}

// Chama proximo_token_bruto() ate chegar no fim, guardando tudo na lista
ListaTokens lexer_tokenizar(const char *texto) {
    Lexer lx;                          // variavel LOCAL: deixa de existir quando a funcao retorna
    lexer_iniciar(&lx, texto);         // (por isso o main conta erros lexicos pela lista, nao por lx)

    ListaTokens lst;
    lista_tokens_iniciar(&lst);

    for (;;) {
        Token t = proximo_token_bruto(&lx);
        lista_tokens_adicionar(&lst, t);        // TOK_FIM tambem entra -> o parser sempre tem um ultimo token
        if (t.tipo == TOK_FIM) break;
    }
    return lst;
}

// Converte o enum em texto, para as mensagens de erro do parser
const char *token_nome(TipoToken tipo) {
    switch (tipo) {
        case TOK_INTEIRA: return "inteira";
        case TOK_FLUT: return "flut";
        case TOK_BOOL: return "bool";
        case TOK_PALAVRA: return "palavra";
        case TOK_DUPLOCARPADO: return "duplocarpado";
        case TOK_VAZIO: return "vazio";
        case TOK_RECEBA: return "receba";
        case TOK_REPETE: return "repete";
        case TOK_DURANTE: return "durante";
        case TOK_COND: return "cond";
        case TOK_CASOCONTRARIO: return "casocontrario";
        case TOK_OUTRAFUNCAO: return "outrafuncao";
        case TOK_PRINCIPAL: return "principal";
        case TOK_RESPOST: return "respost";
        case TOK_REGISTRO: return "registro";
        case TOK_EDAI: return "edai";
        case TOK_VET: return "vet";
        case TOK_RAIZ: return "raiz";
        case TOK_FUNC: return "func";
        case TOK_VER: return "VER";
        case TOK_FAL: return "FAL";
        case TOK_OU: return "OU";
        case TOK_E: return "E";
        case TOK_NAO: return "NAO";
        case TOK_XOR: return "XOR";
        case TOK_IDENTIFICADOR: return "identificador";
        case TOK_NUMERO: return "numero";
        case TOK_STRING: return "string";
        case TOK_PONTO_VIRGULA: return ";";
        case TOK_VIRGULA: return ",";
        case TOK_ARROBA: return "@";
        case TOK_ABRE_CHAVE: return "{";
        case TOK_FECHA_CHAVE: return "}";
        case TOK_ABRE_COLCHETE: return "[";
        case TOK_FECHA_COLCHETE: return "]";
        case TOK_ABRE_PARENTESE: return "(";
        case TOK_FECHA_PARENTESE: return ")";
        case TOK_MAIS: return "+";
        case TOK_MENOS: return "-";
        case TOK_VEZES: return "*";
        case TOK_DIVIDE: return "/";
        case TOK_DIV_INT: return "//";
        case TOK_POTENCIA: return "^";
        case TOK_IGUAL: return "==";
        case TOK_DIFERENTE: return "!";
        case TOK_MAIOR: return ">";
        case TOK_MENOR: return "<";
        case TOK_FIM: return "FIM";
        case TOK_INVALIDO: return "token invalido";
    }
    return "?";     // nunca deveria chegar aqui (evita warning do compilador)
}
