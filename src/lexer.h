/* lexer.h - Analisador Lexico da linguagem Macaronica */
#ifndef LEXER_H          // include guard: impede que o arquivo seja incluido duas vezes
#define LEXER_H          // (parser.h e main.c incluem lexer.h; sem isso os tipos seriam redefinidos)

// Enum com TODOS os tipos de token que o lexer pode produzir.
// Cada valor e so um numero inteiro (TOK_INTEIRA = 0, TOK_FLUT = 1, ...).
typedef enum {
    /* Tipos de dado */
    TOK_INTEIRA, TOK_FLUT, TOK_BOOL, TOK_PALAVRA, TOK_DUPLOCARPADO, TOK_VAZIO,

    /* Palavras reservadas de estrutura */
    TOK_RECEBA, TOK_REPETE, TOK_DURANTE, TOK_COND, TOK_CASOCONTRARIO,
    TOK_OUTRAFUNCAO, TOK_PRINCIPAL, TOK_RESPOST, TOK_REGISTRO, TOK_EDAI,
    TOK_VET, TOK_RAIZ, TOK_FUNC,     // TOK_FUNC e reservada mas nenhuma regra da gramatica usa

    /* Literais booleanos */
    TOK_VER, TOK_FAL,

    /* Operadores logicos */
    TOK_OU, TOK_E, TOK_NAO, TOK_XOR,

    /* Identificador e literais */
    TOK_IDENTIFICADOR, TOK_NUMERO, TOK_STRING,   // IDENTIFICADOR = qualquer nome que nao seja reservado

    /* Pontuacao */
    TOK_PONTO_VIRGULA, TOK_VIRGULA, TOK_ARROBA,
    TOK_ABRE_CHAVE, TOK_FECHA_CHAVE,             // { } -> tamanho do vetor (vet{10}) e indice (notas{i})
    TOK_ABRE_COLCHETE, TOK_FECHA_COLCHETE,       // [ ] -> blocos de comando (funcao, registro, cond...)
    TOK_ABRE_PARENTESE, TOK_FECHA_PARENTESE,

    /* Operadores aritmeticos (SINAL) */
    TOK_MAIS, TOK_MENOS, TOK_VEZES, TOK_DIVIDE, TOK_DIV_INT, TOK_POTENCIA,  // DIVIDE = "/", DIV_INT = "//"

    /* Comparadores */
    TOK_IGUAL, TOK_DIFERENTE, TOK_MAIOR, TOK_MENOR,  // IGUAL = "==", DIFERENTE = "!" sozinho

    TOK_FIM, TOK_INVALIDO   // FIM = fim do arquivo; INVALIDO = caractere que gerou erro lexico
} TipoToken;

#define LEXEMA_MAX 256      // tamanho maximo do texto de um token

// Um token = uma "palavra" reconhecida do codigo-fonte
typedef struct {
    TipoToken tipo;             // O QUE ele e (TOK_NUMERO, TOK_RECEBA, ...)
    char lexema[LEXEMA_MAX];    // o texto original, ex: "5.64" ou "soma"
    double valor_num;           // valor numerico ja convertido (so usado quando tipo == TOK_NUMERO)
    int linha;                  // onde o token comeca (para mensagens de erro)
    int coluna;
} Token;

// Estado do lexer enquanto ele le o texto
typedef struct {
    const char *texto;      // o codigo-fonte inteiro
    int pos;                // indice do CARACTERE sendo lido agora (no parser, pos e indice de TOKEN)
    int linha_atual;
    int coluna_atual;
    int erros_lexicos;
} Lexer;

/* Lista dinamica de tokens (resultado da tokenizacao completa) */
typedef struct {
    Token *itens;       // vetor alocado com malloc/realloc
    int quantidade;     // quantos tokens ja foram guardados
    int capacidade;     // quantos cabem antes de precisar realocar
} ListaTokens;

void lexer_iniciar(Lexer *lx, const char *texto);

// Funcao principal do lexer: tokeniza o texto inteiro de uma vez.
// Erros lexicos sao reportados com linha/coluna, mas NAO interrompem a tokenizacao.
ListaTokens lexer_tokenizar(const char *texto);
void lista_tokens_destruir(ListaTokens *lst);

const char *token_nome(TipoToken tipo);   // converte o enum em texto (usado nas mensagens de erro)

#endif
