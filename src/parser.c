/* parser.c - Analisador Sintatico da linguagem Macaronica
 * Descida recursiva: cada regra da gramatica vira uma funcao parse_xxx.
 * Gramatica completa em docs/decisoes.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Cria um no novo da arvore
Node *criar_no(NodeKind kind, int linha) {
    Node *no = calloc(1, sizeof(Node));     // calloc zera tudo: ponteiros ja nascem NULL
    no->kind = kind;
    no->linha = linha;
    no->tipo_declarado = TIPO_INDEFINIDO;   // precisa setar: zero seria TIPO_INT (primeiro do enum)
    no->tipo_resultado = TIPO_INDEFINIDO;
    return no;
}

const char *tipo_nome(Tipo t) {
    switch (t) {
        case TIPO_INT: return "inteira";
        case TIPO_FLOAT: return "flut";
        case TIPO_DOUBLE: return "duplocarpado";
        case TIPO_BOOL: return "bool";
        case TIPO_STRING: return "palavra";
        case TIPO_VOID: return "vazio";
        default: return "indefinido";
    }
}

void parser_iniciar(Parser *p, ListaTokens tokens) {
    p->tokens = tokens;
    p->pos = 0;
    p->erros_sintaticos = 0;
}

static Token *lookahead(Parser *p) { return &p->tokens.itens[p->pos]; }   // token atual (nao anda)
static Token *ver_frente(Parser *p, int n) {                               // token n posicoes a frente (nao anda)
    int idx = p->pos + n;
    if (idx >= p->tokens.quantidade) idx = p->tokens.quantidade - 1;       // passou do fim? devolve o TOK_FIM
    return &p->tokens.itens[idx];
}

static void avancar(Parser *p) {
    if (p->pos < p->tokens.quantidade - 1) p->pos++;   // nunca passa do ultimo token (TOK_FIM)
}

// Reporta erro sintatico no token atual e soma o contador (nao aborta)
static void erro_sintatico(Parser *p, const char *esperado) {
    Token *t = lookahead(p);
    p->erros_sintaticos++;
    fprintf(stderr, "[Linha %d] ERRO SINTATICO: esperava %s, encontrado '%s' (%s)\n",
            t->linha, esperado,
            t->lexema[0] ? t->lexema : token_nome(t->tipo),
            token_nome(t->tipo));
}

// Recuperacao de erro em "modo panico": joga tokens fora ate um ponto seguro.
// Ponto seguro = um ';' (fim de comando) ou o ']' que fecha o BLOCO atual.
// "prof" conta os '[' abertos DURANTE a recuperacao: assim um "[0]" escrito por engano
// nao e confundido com o fim do bloco.
static void sincronizar(Parser *p) {
    if (lookahead(p)->tipo == TOK_FIM) return;
    int prof = 0;
    for (;;) {
        TipoToken t = lookahead(p)->tipo;
        if (t == TOK_ABRE_COLCHETE) prof++;                   // abriu um '[' no meio do lixo
        else if (t == TOK_FECHA_COLCHETE && prof > 0) prof--; // fechou esse '[' (nao e o do bloco)
        avancar(p);                                           // sempre avanca: impede loop infinito
        t = lookahead(p)->tipo;
        if (t == TOK_FIM) return;
        if (prof == 0 && t == TOK_PONTO_VIRGULA) { avancar(p); return; }  // consome o ';' e recomeca
        if (prof == 0 && t == TOK_FECHA_COLCHETE) return;     // para ANTES do ']' do bloco: parse_bloco fecha ele
    }
}

// "Tem que ser este token aqui": se for, consome; se nao for, reporta erro e NAO consome
static Token casar(Parser *p, TipoToken esperado, const char *nome_amigavel) {
    Token atual = *lookahead(p);
    if (atual.tipo != esperado) {
        erro_sintatico(p, nome_amigavel);
        return atual;
    }
    avancar(p);
    return atual;       // devolve o token (util para pegar o nome de um ID, por exemplo)
}

static int eh_tipo(TipoToken t) {       // o token e um dos 6 tipos da linguagem?
    return t == TOK_INTEIRA || t == TOK_FLUT || t == TOK_BOOL ||
           t == TOK_PALAVRA || t == TOK_DUPLOCARPADO || t == TOK_VAZIO;
}

static Tipo mapear_tipo(TipoToken t) {  // converte token de tipo -> Tipo da AST
    switch (t) {
        case TOK_INTEIRA: return TIPO_INT;
        case TOK_FLUT: return TIPO_FLOAT;
        case TOK_DUPLOCARPADO: return TIPO_DOUBLE;
        case TOK_BOOL: return TIPO_BOOL;
        case TOK_PALAVRA: return TIPO_STRING;
        case TOK_VAZIO: return TIPO_VOID;
        default: return TIPO_INDEFINIDO;
    }
}

// Le um tipo; se o token atual nao for um tipo, e erro sintatico
static Tipo parse_tipo(Parser *p) {
    if (!eh_tipo(lookahead(p)->tipo)) {
        erro_sintatico(p, "um tipo (inteira/flut/bool/palavra/duplocarpado/vazio)");
        return TIPO_INDEFINIDO;
    }
    Tipo t = mapear_tipo(lookahead(p)->tipo);
    avancar(p);
    return t;
}

/* ---------- Prototipos (as funcoes se chamam em ciclo: expr -> termo -> fator -> expr) ---------- */
static Node *parse_bloco(Parser *p);
static Node *parse_comando(Parser *p);
static Node *parse_condicao(Parser *p);
static Node *parse_expr(Parser *p);
static Node *parse_args(Parser *p);

/* ---------- Expressoes ---------- */

// fator: a "folha" de uma expressao (nivel de MAIOR prioridade)
static Node *parse_fator(Parser *p) {
    Token *t = lookahead(p);
    int linha = t->linha;

    if (t->tipo == TOK_NUMERO) {
        Node *no = criar_no(ND_NUM, linha);
        no->num_valor = t->valor_num;
        no->tipo_resultado = (strchr(t->lexema, '.') != NULL) ? TIPO_FLOAT : TIPO_INT;  // tem ponto? flut : inteira
        avancar(p);
        return no;
    }
    if (t->tipo == TOK_STRING) {
        Node *no = criar_no(ND_STRING_LIT, linha);
        strncpy(no->str_valor, t->lexema, sizeof(no->str_valor) - 1);
        no->tipo_resultado = TIPO_STRING;
        avancar(p);
        return no;
    }
    if (t->tipo == TOK_VER || t->tipo == TOK_FAL) {
        Node *no = criar_no(ND_BOOL_LIT, linha);
        no->num_valor = (t->tipo == TOK_VER) ? 1 : 0;
        no->tipo_resultado = TIPO_BOOL;
        avancar(p);
        return no;
    }
    if (t->tipo == TOK_RAIZ) {                       // raiz ( expr )
        avancar(p);
        casar(p, TOK_ABRE_PARENTESE, "'('");
        Node *no = criar_no(ND_RAIZ_EXPR, linha);
        no->filho1 = parse_expr(p);                  // RECURSAO: o argumento e uma expressao inteira
        casar(p, TOK_FECHA_PARENTESE, "')'");
        return no;
    }
    if (t->tipo == TOK_ABRE_PARENTESE) {             // ( expr ) -> parenteses mudam a precedencia
        avancar(p);
        Node *no = parse_expr(p);                    // RECURSAO: volta para o nivel mais baixo
        casar(p, TOK_FECHA_PARENTESE, "')'");
        return no;                                   // nao cria no para o parentese: a arvore ja expressa a ordem
    }
    if (t->tipo == TOK_MENOS) {                      // menos unario: -x vira 0 - x
        avancar(p);
        Node *no = criar_no(ND_BINOP, linha);
        strcpy(no->nome, "-unario");
        no->filho1 = criar_no(ND_NUM, linha);        // lado esquerdo = 0
        no->filho1->num_valor = 0;
        no->filho1->tipo_resultado = TIPO_INT;
        no->filho2 = parse_fator(p);                 // lado direito = o que vem depois do '-'
        return no;
    }
    if (t->tipo == TOK_IDENTIFICADOR) {              // ID pode ser: variavel, v{i} ou chamada f(...)
        char nome[64];
        strncpy(nome, t->lexema, sizeof(nome) - 1);
        nome[sizeof(nome)-1] = '\0';
        avancar(p);

        if (lookahead(p)->tipo == TOK_ABRE_PARENTESE) {     // ID ( ... ) -> chamada de funcao
            avancar(p);
            Node *no = criar_no(ND_CHAMADA_EXPR, linha);
            strcpy(no->nome, nome);
            if (lookahead(p)->tipo != TOK_FECHA_PARENTESE) {
                no->lista = parse_args(p);                  // lista de argumentos
            }
            casar(p, TOK_FECHA_PARENTESE, "')'");
            return no;
        }
        if (lookahead(p)->tipo == TOK_ABRE_CHAVE) {         // ID { ... } -> acesso a vetor (ex: notas{i})
            avancar(p);
            Node *no = criar_no(ND_ACESSO_VETOR, linha);
            strcpy(no->nome, nome);
            no->filho1 = parse_expr(p);                     // o indice
            casar(p, TOK_FECHA_CHAVE, "'}'");
            return no;
        }
        if (lookahead(p)->tipo == TOK_PONTO) {              // ID . campo -> leitura de campo de registro
            avancar(p);
            Node *no = criar_no(ND_CAMPO_EXPR, linha);
            strcpy(no->nome, nome);
            Token c = casar(p, TOK_IDENTIFICADOR, "o nome do campo");
            strncpy(no->campo, c.lexema, sizeof(no->campo) - 1);
            return no;
        }
        Node *no = criar_no(ND_VAR, linha);                 // so o ID -> variavel simples
        strcpy(no->nome, nome);
        return no;
    }

    erro_sintatico(p, "uma expressao valida");
    Node *no = criar_no(ND_NUM, linha);          // devolve um no "falso" para o parser nao quebrar
    no->tipo_resultado = TIPO_INDEFINIDO;
    return no;
}

// termo: multiplicacao, divisao, divisao inteira e potencia (prioridade media)
static Node *parse_termo(Parser *p) {
    Node *esq = parse_fator(p);
    while (lookahead(p)->tipo == TOK_VEZES || lookahead(p)->tipo == TOK_DIVIDE ||
           lookahead(p)->tipo == TOK_DIV_INT || lookahead(p)->tipo == TOK_POTENCIA) {
        // ⚠️ '^' tem a mesma prioridade de '*' e e associativo a esquerda: 2^3^2 = (2^3)^2
        Token op = *lookahead(p);
        avancar(p);
        Node *no = criar_no(ND_BINOP, op.linha);
        strcpy(no->nome, token_nome(op.tipo));  // guarda qual operador e ("*", "/", ...)
        no->filho1 = esq;
        no->filho2 = parse_fator(p);
        esq = no;                               // o no novo vira o lado esquerdo -> associatividade a esquerda
    }
    return esq;
}

// expr: soma e subtracao (MENOR prioridade -> fica mais no topo da arvore)
static Node *parse_expr(Parser *p) {
    Node *esq = parse_termo(p);                 // termo e resolvido primeiro -> '*' fica abaixo do '+'
    while (lookahead(p)->tipo == TOK_MAIS || lookahead(p)->tipo == TOK_MENOS) {
        Token op = *lookahead(p);
        avancar(p);
        Node *no = criar_no(ND_BINOP, op.linha);
        strcpy(no->nome, token_nome(op.tipo));
        no->filho1 = esq;
        no->filho2 = parse_termo(p);
        esq = no;                               // 10 - 3 - 2 vira (10 - 3) - 2
    }
    return esq;
}

// args: expr , expr , expr ...  -> lista ligada por ->prox
static Node *parse_args(Parser *p) {
    Node *cabeca = parse_expr(p);
    Node *atual = cabeca;
    while (lookahead(p)->tipo == TOK_VIRGULA) {
        avancar(p);
        atual->prox = parse_expr(p);            // pendura o proximo argumento
        atual = atual->prox;
    }
    return cabeca;
}

/* ---------- Condicoes ----------
 * A especificacao so define VAR/NUM COMPARADOR VAR/NUM. O grupo estendeu para
 * combinar comparacoes com OU/E/XOR e negar com NAO (decisoes.md, item 2.2). */
// Um '(' no inicio de uma condicao pode abrir um GRUPO DE CONDICOES "(a < 1 OU b > 2)"
// ou uma EXPRESSAO "(a + 1) < b". Decide olhando a frente (sem consumir): se dentro
// desse parentese, no primeiro nivel, aparece um comparador ou operador logico, e condicao.
static int parentese_e_condicao(Parser *p) {
    int prof = 0;
    for (int n = 0; ; n++) {
        TipoToken t = ver_frente(p, n)->tipo;
        if (t == TOK_FIM) return 0;
        if (t == TOK_ABRE_PARENTESE) prof++;
        else if (t == TOK_FECHA_PARENTESE) { if (--prof == 0) return 0; }
        else if (prof == 1 && (t == TOK_MENOR || t == TOK_MAIOR || t == TOK_IGUAL || t == TOK_DIFERENTE ||
                               t == TOK_OU || t == TOK_E || t == TOK_XOR || t == TOK_NAO)) return 1;
    }
}

static Node *parse_cond_simples(Parser *p) {
    int linha = lookahead(p)->linha;

    if (lookahead(p)->tipo == TOK_NAO) {            // NAO condicao
        avancar(p);
        Node *no = criar_no(ND_LOGICO_NOT, linha);
        no->filho1 = parse_cond_simples(p);         // RECURSAO: nega o que vem depois
        return no;
    }
    if (lookahead(p)->tipo == TOK_ABRE_PARENTESE && parentese_e_condicao(p)) {   // ( condicao )
        avancar(p);
        Node *no = parse_condicao(p);
        casar(p, TOK_FECHA_PARENTESE, "')'");
        return no;
    }

    // operando COMPARADOR operando. A especificacao so mostra VAR/NUM dos dois lados;
    // no PjBL 3 generalizamos para EXPRESSOES aritmeticas (a forma original continua
    // valida, ver docs/decisoes.md 2.2):  cond ( (n // 2) * 2 == n )
    Node *op_esq = parse_expr(p);

    Token *cmp = lookahead(p);
    if (cmp->tipo != TOK_MENOR && cmp->tipo != TOK_MAIOR &&
        cmp->tipo != TOK_IGUAL && cmp->tipo != TOK_DIFERENTE) {
        erro_sintatico(p, "um comparador (< > == !)");
    }
    Node *no = criar_no(ND_COMPARACAO, linha);
    strcpy(no->nome, token_nome(cmp->tipo));        // guarda o comparador ("<", "==", ...)
    avancar(p);
    no->filho1 = op_esq;
    no->filho2 = parse_expr(p);
    return no;
}

// condicao: cond_simples (OU|E|XOR cond_simples)*  -- os tres com a MESMA prioridade
static Node *parse_condicao(Parser *p) {
    Node *esq = parse_cond_simples(p);
    while (lookahead(p)->tipo == TOK_OU || lookahead(p)->tipo == TOK_E ||
           lookahead(p)->tipo == TOK_XOR) {
        Token op = *lookahead(p);
        avancar(p);
        Node *no = criar_no(ND_LOGICO_BIN, op.linha);
        strcpy(no->nome, token_nome(op.tipo));
        no->filho1 = esq;
        no->filho2 = parse_cond_simples(p);
        esq = no;
    }
    return esq;
}

/* ---------- Comandos ---------- */

// Declaracao:  tipo ID (, ID)* ;   ou   tipo vet { NUM } ID (, ID)* ;
// Devolve uma LISTA de nos (um por nome): "inteira a, b;" gera 2 ND_DECL_VAR
static Node *parse_decl(Parser *p) {
    int linha = lookahead(p)->linha;
    Tipo tipo = parse_tipo(p);

    int eh_vetor = 0;
    double tamanho = 0;
    if (lookahead(p)->tipo == TOK_VET) {            // e vetor?
        avancar(p);
        eh_vetor = 1;
        casar(p, TOK_ABRE_CHAVE, "'{'");            // chaves DEPOIS de 'vet' = TAMANHO do vetor
        Token tam = casar(p, TOK_NUMERO, "o tamanho do vetor (numero)");
        tamanho = tam.valor_num;
        casar(p, TOK_FECHA_CHAVE, "'}'");
    }

    Node *cabeca = NULL, *atual = NULL;
    for (;;) {
        Token id = casar(p, TOK_IDENTIFICADOR, "um identificador");
        Node *no = criar_no(eh_vetor ? ND_DECL_VETOR : ND_DECL_VAR, linha);
        strcpy(no->nome, id.lexema);
        no->tipo_declarado = tipo;                  // todos os nomes da linha recebem o mesmo tipo
        no->num_valor = tamanho;
        if (!cabeca) cabeca = no; else atual->prox = no;   // monta a lista
        atual = no;

        if (lookahead(p)->tipo == TOK_VIRGULA) { avancar(p); continue; }   // tem mais nomes
        break;
    }
    casar(p, TOK_PONTO_VIRGULA, "';'");             // "inteira x receba 7;" da erro AQUI: nao existe
    return cabeca;                                  // declarar-e-inicializar na mesma linha
}

// Atribuicao SEM o ';' (reaproveitada no cabecalho do repete):  ID ({ expr })? receba expr
static Node *parse_atrib_sem_pv(Parser *p) {
    int linha = lookahead(p)->linha;
    Token id = casar(p, TOK_IDENTIFICADOR, "um identificador");

    Node *no;
    if (lookahead(p)->tipo == TOK_PONTO) {          // a.campo receba ...
        avancar(p);
        no = criar_no(ND_ATRIB_CAMPO, linha);
        strcpy(no->nome, id.lexema);
        Token c = casar(p, TOK_IDENTIFICADOR, "o nome do campo");
        strncpy(no->campo, c.lexema, sizeof(no->campo) - 1);
    } else if (lookahead(p)->tipo == TOK_ABRE_CHAVE) {     // v{i} receba ...  (chaves DEPOIS de um ID = INDICE)
        avancar(p);
        no = criar_no(ND_ATRIB_VETOR, linha);
        strcpy(no->nome, id.lexema);
        no->filho2 = parse_expr(p);                 // filho2 = indice
        casar(p, TOK_FECHA_CHAVE, "'}'");
    } else {                                        // x receba ...
        no = criar_no(ND_ATRIB, linha);
        strcpy(no->nome, id.lexema);
    }
    casar(p, TOK_RECEBA, "'receba'");
    no->filho1 = parse_expr(p);                     // filho1 = valor atribuido
    return no;
}

// Declaracao de variavel de registro:  NomeRegistro nome (, nome)* ;   ex: Aluno a, b;
static Node *parse_decl_reg(Parser *p) {
    int linha = lookahead(p)->linha;
    Token tipo_reg = casar(p, TOK_IDENTIFICADOR, "o nome do registro");
    Node *cabeca = NULL, *atual = NULL;
    for (;;) {
        Token id = casar(p, TOK_IDENTIFICADOR, "um identificador");
        Node *no = criar_no(ND_DECL_REG, linha);
        strcpy(no->nome, id.lexema);
        strncpy(no->nome_reg, tipo_reg.lexema, sizeof(no->nome_reg) - 1);
        no->tipo_declarado = TIPO_REGISTRO;
        if (!cabeca) cabeca = no; else atual->prox = no;
        atual = no;
        if (lookahead(p)->tipo == TOK_VIRGULA) { avancar(p); continue; }
        break;
    }
    casar(p, TOK_PONTO_VIRGULA, "';'");
    return cabeca;
}

static Node *parse_atrib(Parser *p) {               // atribuicao normal = sem_pv + ';'
    Node *no = parse_atrib_sem_pv(p);
    casar(p, TOK_PONTO_VIRGULA, "';'");
    return no;
}

// cond (IF):  cond ( condicao ) edai [ ... ]  (casocontrario cond ... | casocontrario edai [ ... ])?
// filho1 = condicao (NULL no "senao" final), lista = bloco "entao", filho2 = proximo ramo
static Node *parse_cond(Parser *p) {
    int linha = lookahead(p)->linha;
    casar(p, TOK_COND, "'cond'");
    casar(p, TOK_ABRE_PARENTESE, "'('");
    Node *no = criar_no(ND_COND, linha);
    no->filho1 = parse_condicao(p);
    casar(p, TOK_FECHA_PARENTESE, "')'");
    casar(p, TOK_EDAI, "'edai'");
    no->lista = parse_bloco(p);                     // comandos do "entao"

    if (lookahead(p)->tipo == TOK_CASOCONTRARIO) {
        avancar(p);
        if (lookahead(p)->tipo == TOK_COND) {       // "casocontrario cond" = ELSE IF
            no->filho2 = parse_cond(p);             // RECURSAO: cadeias de qualquer tamanho
        } else {                                    // "casocontrario edai" = ELSE final
            casar(p, TOK_EDAI, "'edai'");
            Node *senao = criar_no(ND_COND, lookahead(p)->linha);
            senao->filho1 = NULL;                   // sem condicao -> e assim que o semantico sabe que e o else
            senao->lista = parse_bloco(p);
            no->filho2 = senao;
        }
    }
    return no;
}

// repete (FOR):  repete ( inicializacao , condicao , incremento ) edai [ ... ]
// filho1 = inicializacao, filho2 = condicao, filho3 = incremento, lista = corpo
static Node *parse_repete(Parser *p) {
    int linha = lookahead(p)->linha;
    casar(p, TOK_REPETE, "'repete'");
    casar(p, TOK_ABRE_PARENTESE, "'('");
    Node *no = criar_no(ND_REPETE, linha);
    no->filho1 = parse_atrib_sem_pv(p);             // i receba 0   (sem ';', separado por virgula)
    casar(p, TOK_VIRGULA, "','");
    no->filho2 = parse_condicao(p);                 // i < 10
    casar(p, TOK_VIRGULA, "','");
    no->filho3 = parse_atrib_sem_pv(p);             // i receba i + 1
    casar(p, TOK_FECHA_PARENTESE, "')'");
    casar(p, TOK_EDAI, "'edai'");
    no->lista = parse_bloco(p);
    return no;
}

// durante (WHILE):  durante ( condicao ) edai [ ... ]
static Node *parse_durante(Parser *p) {
    int linha = lookahead(p)->linha;
    casar(p, TOK_DURANTE, "'durante'");
    casar(p, TOK_ABRE_PARENTESE, "'('");
    Node *no = criar_no(ND_DURANTE, linha);
    no->filho1 = parse_condicao(p);
    casar(p, TOK_FECHA_PARENTESE, "')'");
    casar(p, TOK_EDAI, "'edai'");
    no->lista = parse_bloco(p);
    return no;
}

// respost (RETURN):  respost expr? ;
// O parser so registra SE tem valor. Se isso e permitido depende do tipo da funcao,
// e quem sabe o tipo e o semantico (ambiguidade 4.3).
static Node *parse_respost(Parser *p) {
    int linha = lookahead(p)->linha;
    casar(p, TOK_RESPOST, "'respost'");
    Node *no = criar_no(ND_RESPOST, linha);
    if (lookahead(p)->tipo != TOK_PONTO_VIRGULA) {  // tem algo antes do ';' -> tem valor
        no->filho1 = parse_expr(p);
    }
    casar(p, TOK_PONTO_VIRGULA, "';'");
    return no;
}

// Chamada usada como comando isolado:  nome ( args ) ;
static Node *parse_chamada_stmt(Parser *p) {
    int linha = lookahead(p)->linha;
    Token id = casar(p, TOK_IDENTIFICADOR, "um identificador");
    casar(p, TOK_ABRE_PARENTESE, "'('");
    Node *no = criar_no(ND_CHAMADA_STMT, linha);
    strcpy(no->nome, id.lexema);
    if (lookahead(p)->tipo != TOK_FECHA_PARENTESE) {
        no->lista = parse_args(p);
    }
    casar(p, TOK_FECHA_PARENTESE, "')'");
    casar(p, TOK_PONTO_VIRGULA, "';'");
    return no;
}

// Decide qual comando e olhando o PRIMEIRO token (e, no caso do ID, o segundo)
static Node *parse_comando(Parser *p) {
    TipoToken t = lookahead(p)->tipo;

    if (eh_tipo(t)) return parse_decl(p);           // comeca com tipo -> declaracao
    if (t == TOK_COND) return parse_cond(p);
    if (t == TOK_REPETE) return parse_repete(p);
    if (t == TOK_DURANTE) return parse_durante(p);
    if (t == TOK_RESPOST) return parse_respost(p);
    if (t == TOK_IDENTIFICADOR) {
        // x receba 5;  e  f(x);  comecam os dois com ID -> olha UM token a frente para decidir
        if (ver_frente(p, 1)->tipo == TOK_ABRE_PARENTESE) return parse_chamada_stmt(p);
        if (ver_frente(p, 1)->tipo == TOK_IDENTIFICADOR) return parse_decl_reg(p);   // Aluno a;  (ID ID)
        return parse_atrib(p);
    }

    erro_sintatico(p, "um comando valido (declaracao, atribuicao, cond, repete, durante, respost ou chamada de funcao)");
    sincronizar(p);                                 // pula ate um ponto seguro e continua
    return NULL;
}

// Bloco:  [ comando* ]
static Node *parse_bloco(Parser *p) {
    casar(p, TOK_ABRE_COLCHETE, "'['");
    Node *cabeca = NULL, *atual = NULL;
    int pos_antes;
    while (lookahead(p)->tipo != TOK_FECHA_COLCHETE && lookahead(p)->tipo != TOK_FIM) {
        pos_antes = p->pos;                         // guarda onde estava
        Node *no = parse_comando(p);
        if (no) {
            Node *fim_lista = no;
            while (fim_lista->prox) fim_lista = fim_lista->prox;   // "inteira a, b;" devolve 2 nos: vai ate o ultimo
            if (!cabeca) cabeca = no; else atual->prox = no;       // encaixa na lista de comandos
            atual = fim_lista;
        }
        // Cinto de seguranca: se esta volta nao consumiu nenhum token, forca o avanco
        // -> impossivel ficar preso em loop infinito (bug do codigo original)
        if (p->pos == pos_antes) {
            if (lookahead(p)->tipo == TOK_FIM) break;
            avancar(p);
        }
    }
    casar(p, TOK_FECHA_COLCHETE, "']'");            // se faltar o ']' -> "esperava ']', encontrado FIM"
    return cabeca;
}

// registro Nome @ [ tipo campo; tipo campo; ... ]
static Node *parse_registro(Parser *p) {
    int linha = lookahead(p)->linha;
    casar(p, TOK_REGISTRO, "'registro'");
    Node *no = criar_no(ND_REGISTRO, linha);
    Token id = casar(p, TOK_IDENTIFICADOR, "o nome do registro");
    strcpy(no->nome, id.lexema);
    casar(p, TOK_ARROBA, "'@'");
    casar(p, TOK_ABRE_COLCHETE, "'['");             // registro usa [ ], nao { }

    Node *cabeca = NULL, *atual = NULL;
    int pos_antes;
    while (lookahead(p)->tipo != TOK_FECHA_COLCHETE && lookahead(p)->tipo != TOK_FIM) {
        pos_antes = p->pos;
        if (eh_tipo(lookahead(p)->tipo)) {          // cada campo: tipo nome ;
            int linha_campo = lookahead(p)->linha;
            Tipo tipo_campo = parse_tipo(p);
            for (;;) {                              // aceita varios nomes: inteira x, y;  (igual a declaracao)
                Token nome_campo = casar(p, TOK_IDENTIFICADOR, "o nome do campo");
                Node *campo = criar_no(ND_CAMPO, linha_campo);
                strcpy(campo->nome, nome_campo.lexema);
                campo->tipo_declarado = tipo_campo;
                if (!cabeca) cabeca = campo; else atual->prox = campo;
                atual = campo;
                if (lookahead(p)->tipo == TOK_VIRGULA) { avancar(p); continue; }
                break;
            }
            casar(p, TOK_PONTO_VIRGULA, "';'");
        } else {
            erro_sintatico(p, "um campo do registro (tipo nome;)");
            sincronizar(p);
        }
        if (p->pos == pos_antes) avancar(p);        // mesmo cinto de seguranca do parse_bloco
    }
    casar(p, TOK_FECHA_COLCHETE, "']'");
    no->lista = cabeca;                             // lista de campos
    return no;
}

// funcao:  outrafuncao tipo Nome ( params ) @ [ ... ]   ou   tipo principal ( params ) @ [ ... ]
static Node *parse_funcao(Parser *p) {
    int linha = lookahead(p)->linha;
    int eh_principal = 0;
    Tipo tipo_retorno;
    char nome[64] = "";

    if (lookahead(p)->tipo == TOK_OUTRAFUNCAO) {    // funcao comum: outrafuncao TIPO NOME
        avancar(p);
        tipo_retorno = parse_tipo(p);
        Token id = casar(p, TOK_IDENTIFICADOR, "o nome da funcao");
        strcpy(nome, id.lexema);
    } else {                                        // principal: TIPO principal (a ordem e invertida)
        tipo_retorno = parse_tipo(p);
        casar(p, TOK_PRINCIPAL, "'principal'");
        strcpy(nome, "principal");
        eh_principal = 1;
    }

    Node *no = criar_no(ND_FUNCAO, linha);
    strcpy(no->nome, nome);
    no->tipo_declarado = tipo_retorno;              // tipo de retorno
    no->num_valor = eh_principal;                   // 1 = principal (o semantico conta quantas existem)

    casar(p, TOK_ABRE_PARENTESE, "'('");
    Node *cab_params = NULL, *atual_p = NULL;
    if (lookahead(p)->tipo != TOK_FECHA_PARENTESE) {    // tem parametros?
        for (;;) {
            int linha_param = lookahead(p)->linha;
            Tipo tipo_param = parse_tipo(p);            // cada parametro: tipo nome
            Token id_param = casar(p, TOK_IDENTIFICADOR, "o nome do parametro");
            Node *param = criar_no(ND_PARAM, linha_param);
            strcpy(param->nome, id_param.lexema);
            param->tipo_declarado = tipo_param;
            if (!cab_params) cab_params = param; else atual_p->prox = param;
            atual_p = param;
            if (lookahead(p)->tipo == TOK_VIRGULA) { avancar(p); continue; }
            break;
        }
    }
    casar(p, TOK_FECHA_PARENTESE, "')'");
    no->filho1 = cab_params;                        // filho1 = lista de parametros

    casar(p, TOK_ARROBA, "'@'");                    // @ separa cabecalho do corpo
    no->lista = parse_bloco(p);                     // lista = corpo da funcao
    return no;
}

// Entrada principal do parser: programa = sequencia de registros e funcoes
Node *parser_analisar(Parser *p) {
    Node *raiz = criar_no(ND_PROGRAMA, 0);          // raiz da AST
    Node *atual = NULL;
    int pos_antes;

    while (lookahead(p)->tipo != TOK_FIM) {
        pos_antes = p->pos;
        Node *no = NULL;
        if (lookahead(p)->tipo == TOK_REGISTRO) {
            no = parse_registro(p);
        } else if (lookahead(p)->tipo == TOK_OUTRAFUNCAO ||
                   (eh_tipo(lookahead(p)->tipo) && ver_frente(p, 1)->tipo == TOK_PRINCIPAL)) {
            no = parse_funcao(p);                   // "tipo principal" -> e a principal
        } else if (eh_tipo(lookahead(p)->tipo)) {
            no = parse_decl(p);                     // "tipo nome;" no topo -> variavel GLOBAL [PjBL 3]
        } else if (lookahead(p)->tipo == TOK_IDENTIFICADOR &&
                   ver_frente(p, 1)->tipo == TOK_IDENTIFICADOR) {
            no = parse_decl_reg(p);                 // "Aluno a;" no topo -> registro GLOBAL
        } else {
            erro_sintatico(p, "'registro', 'outrafuncao', 'tipo principal' ou uma declaracao global");
            sincronizar(p);
        }
        if (no) {
            if (!raiz->lista) raiz->lista = no; else atual->prox = no;   // pendura na raiz
            atual = no;
            while (atual->prox) atual = atual->prox;    // "inteira a, b;" global devolve uma lista
        }
        if (p->pos == pos_antes) {                  // cinto de seguranca de novo
            if (lookahead(p)->tipo == TOK_FIM) break;
            avancar(p);
        }
    }
    return raiz;
}
