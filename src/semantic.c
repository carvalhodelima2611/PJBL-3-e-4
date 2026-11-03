/* semantic.c - Analisador Semantico da linguagem Macaronica
 * Tabela de simbolos com escopos, declaracao antes do uso, tipos,
 * assinatura de funcao, exatamente uma principal, regra do respost.
 * PjBL 3/4: funcoes embutidas (E/S, pinos, DSL), registros usaveis,
 * variaveis globais, regras de / e //, operacoes vetoriais e
 * verificacao de memoria por alvo. A AST sai ANOTADA (tipo_resultado
 * em toda expressao, indice de campo, flag de operacao vetorial), e e
 * essa arvore anotada que o gerador de codigo percorre. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

// Toda mensagem de erro semantico passa por aqui. NUNCA aborta: so conta e imprime.
static void erro_sem(Semantico *sem, int linha, const char *msg, const char *id) {
    sem->erros_semanticos++;
    if (id && id[0]) {                      // tem um nome envolvido? mostra ele entre aspas
        fprintf(stderr, "[Linha %d] ERRO SEMANTICO: %s '%s'\n", linha, msg, id);
    } else {
        fprintf(stderr, "[Linha %d] ERRO SEMANTICO: %s\n", linha, msg);
    }
}

/* ---------- Funcoes embutidas ---------- */

typedef struct { const char *nome; Embutida id; Tipo ret; int n; Tipo p[2]; } DefEmbutida;

// n = -1 -> numero variavel de argumentos (checagem especial)
static const DefEmbutida EMBUTIDAS[] = {
    {"mostrar",         BI_MOSTRAR,      TIPO_VOID,   -1, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"escreve",         BI_ESCREVE,      TIPO_VOID,   -1, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"novalinha",       BI_NOVALINHA,    TIPO_VOID,    0, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"leinteira",       BI_LEINTEIRA,    TIPO_INT,     0, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"leflut",          BI_LEFLUT,       TIPO_FLOAT,   0, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"leduplocarpado",  BI_LEDUPLO,      TIPO_DOUBLE,  0, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"pino_modo",       BI_PINO_MODO,    TIPO_VOID,    2, {TIPO_INT, TIPO_INT}},
    {"pino_escreve",    BI_PINO_ESCREVE, TIPO_VOID,    2, {TIPO_INT, TIPO_INT}},
    {"pino_le",         BI_PINO_LE,      TIPO_INT,     1, {TIPO_INT, TIPO_INDEFINIDO}},
    {"espera",          BI_ESPERA,       TIPO_VOID,    1, {TIPO_INT, TIPO_INDEFINIDO}},
    {"somavet",         BI_SOMAVET,      TIPO_INDEFINIDO, -1, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"maxvet",          BI_MAXVET,       TIPO_INDEFINIDO, -1, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {"prodesc",         BI_PRODESC,      TIPO_INDEFINIDO, -1, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}},
    {NULL, BI_NENHUM, TIPO_INDEFINIDO, 0, {TIPO_INDEFINIDO, TIPO_INDEFINIDO}}
};

Embutida embutida_por_nome(const char *nome) {
    for (int i = 0; EMBUTIDAS[i].nome; i++)
        if (strcmp(EMBUTIDAS[i].nome, nome) == 0) return EMBUTIDAS[i].id;
    return BI_NENHUM;
}

/* ---------- Ciclo de vida ---------- */

void semantico_iniciar(Semantico *sem, Alvo alvo) {
    memset(sem, 0, sizeof(*sem));           // zera tudo (ponteiros NULL, contadores 0)
    sem->tipo_retorno_funcao_atual = TIPO_INDEFINIDO;
    sem->alvo = alvo;
}

void semantico_liberar(Semantico *sem) {
    DefRegistro *r = sem->registros;
    while (r) { DefRegistro *t = r; r = r->proximo; free(t); }
    sem->registros = NULL;
}

long ram_do_alvo(Alvo alvo) {
    switch (alvo) {
        // ATmega328P: 2048 bytes de SRAM, mas o runtime e os textos ja ocupam ~256 bytes
        // (medido com avr-size) e a pilha precisa de folga para as chamadas: sobram 1600
        // bytes para as variaveis do programa (experimento em docs/placa.md, secao 5).
        case ALVO_UNO: return 1600;
        default: return 0;                  // PC e Raspberry Pi (Linux): sem verificacao
    }
}

// EMPILHA um escopo novo (entrando numa funcao ou bloco)
static void entrar_escopo(Semantico *sem) {
    Escopo *novo = malloc(sizeof(Escopo));
    novo->simbolos = NULL;
    novo->pai = sem->escopo_atual;          // o novo aponta para o escopo de fora
    sem->escopo_atual = novo;               // e passa a ser o topo
    if (!sem->escopo_global) sem->escopo_global = novo;   // o primeiro criado e o global
}

// DESEMPILHA o escopo atual e libera a memoria dos simbolos dele
static void sair_escopo(Semantico *sem) {
    if (!sem->escopo_atual) return;
    Escopo *velho = sem->escopo_atual;
    sem->escopo_atual = velho->pai;         // volta para o escopo de fora
    Simbolo *s = velho->simbolos;
    while (s) { Simbolo *tmp = s; s = s->proximo; free(tmp); }   // libera cada simbolo (sem memory leak)
    free(velho);
}

// Busca para USO de um nome: sobe a pilha inteira (local -> ... -> global)
static Simbolo *buscar_simbolo(Semantico *sem, const char *nome) {
    for (Escopo *e = sem->escopo_atual; e; e = e->pai) {        // cada nivel, de dentro para fora
        for (Simbolo *s = e->simbolos; s; s = s->proximo) {     // cada simbolo do nivel
            if (strcmp(s->nome, nome) == 0) return s;           // o mais interno "ganha" (shadowing)
        }
    }
    return NULL;                                                // nao declarado em lugar nenhum
}

// Busca para DECLARACAO: olha SO o escopo atual (checar redeclaracao)
static Simbolo *buscar_simbolo_local(Semantico *sem, const char *nome) {
    if (!sem->escopo_atual) return NULL;
    for (Simbolo *s = sem->escopo_atual->simbolos; s; s = s->proximo) {
        if (strcmp(s->nome, nome) == 0) return s;
    }
    return NULL;
}

// Cadastra um nome novo no escopo atual (ou da erro se ja existe NESTE escopo)
static Simbolo *inserir_simbolo(Semantico *sem, int linha, const char *nome, Tipo tipo, int is_vetor) {
    if (buscar_simbolo_local(sem, nome)) {
        erro_sem(sem, linha, "identificador ja declarado neste escopo", nome);
        return NULL;                                    // quem chama precisa checar o NULL
    }
    Simbolo *s = calloc(1, sizeof(Simbolo));            // calloc: is_funcao, n_params... ja nascem 0
    strncpy(s->nome, nome, sizeof(s->nome) - 1);
    s->tipo = tipo;
    s->is_vetor = is_vetor;
    s->proximo = sem->escopo_atual->simbolos;           // insere no INICIO da lista (mais rapido)
    sem->escopo_atual->simbolos = s;
    if (tipo == TIPO_DOUBLE) sem->usa_duplo = 1;
    return s;
}

DefRegistro *buscar_registro(const DefRegistro *lista, const char *nome) {
    for (const DefRegistro *r = lista; r; r = r->proximo)
        if (strcmp(r->nome, nome) == 0) return (DefRegistro *)r;
    return NULL;
}

/* ---------- Regras de tipo ---------- */

// "Regua" de amplitude numerica: inteira(1) < flut(2) < duplocarpado(3)
// Decisao do grupo (decisoes.md 2.5): so pode converter "para cima"
int rank_numerico(Tipo t) {
    switch (t) {
        case TIPO_INT: return 1;
        case TIPO_FLOAT: return 2;
        case TIPO_DOUBLE: return 3;
        default: return -1;         // bool, palavra, vazio, registro: nao sao numericos
    }
}
int eh_numerico(Tipo t) { return rank_numerico(t) > 0; }

Tipo tipo_mais_largo(Tipo a, Tipo b) {
    return (rank_numerico(a) >= rank_numerico(b)) ? a : b;
}

// Pode guardar um valor do tipo "origem" numa variavel do tipo "destino"?
static int atribuicao_compativel(Tipo destino, Tipo origem) {
    if (destino == TIPO_REGISTRO || origem == TIPO_REGISTRO) return 0;   // registro inteiro: nao
    if (destino == origem) return 1;                            // mesmo tipo: sempre pode
    if (eh_numerico(destino) && eh_numerico(origem)) {
        return rank_numerico(origem) <= rank_numerico(destino); // inteira -> flut pode; flut -> inteira nao
    }
    return 0;                                                   // qualquer outra mistura: erro
}

// Tamanho em bytes de uma variavel no alvo (para a verificacao de memoria)
static long tamanho_tipo(Semantico *sem, Tipo t, const char *nome_reg) {
    int uno = (sem->alvo == ALVO_UNO);
    switch (t) {
        case TIPO_INT: case TIPO_FLOAT: return 4;
        case TIPO_DOUBLE: return uno ? 4 : 8;      // no Uno duplocarpado e rebaixado para 32 bits
        case TIPO_BOOL: return 1;
        case TIPO_STRING: return uno ? 2 : 8;      // ponteiro: 16 bits no AVR
        case TIPO_REGISTRO: {
            DefRegistro *r = buscar_registro(sem->registros, nome_reg);
            long total = 0;
            if (r) for (int i = 0; i < r->n_campos; i++) total += tamanho_tipo(sem, r->campo_tipo[i], "");
            return total;
        }
        default: return 0;
    }
}

// Soma uma variavel na conta de memoria (global ou local da funcao atual)
static void contar_memoria(Semantico *sem, Tipo t, int is_vetor, long tam, const char *nome_reg) {
    long b = tamanho_tipo(sem, t, nome_reg) * (is_vetor ? tam : 1);
    if (sem->em_funcao) sem->bytes_locais += b; else sem->bytes_globais += b;
}

// Le uma constante inteira (5 ou -5) direto da arvore. Devolve 1 se e constante.
static int constante_inteira(Node *no, long *v) {
    if (!no) return 0;
    if (no->kind == ND_NUM && no->tipo_resultado == TIPO_INT) { *v = (long)no->num_valor; return 1; }
    if (no->kind == ND_BINOP && strcmp(no->nome, "-unario") == 0 &&
        no->filho2 && no->filho2->kind == ND_NUM && no->filho2->tipo_resultado == TIPO_INT) {
        *v = -(long)no->filho2->num_valor; return 1;
    }
    return 0;
}

// Se o indice e constante, a verificacao de limite e feita AQUI (em compilacao)
static void checar_indice_constante(Semantico *sem, Node *idx, Simbolo *s, int linha) {
    long v;
    if (s && s->is_vetor && constante_inteira(idx, &v) && (v < 0 || v >= s->tam_vetor)) {
        char msg[160];
        snprintf(msg, sizeof(msg), "indice constante %ld fora dos limites (0 a %d) do vetor",
                 v, s->tam_vetor - 1);
        erro_sem(sem, linha, msg, s->nome);
    }
}

static Tipo tipo_expr(Node *no, Semantico *sem);

// Confere quantidade e tipo dos argumentos de uma chamada (usada em expressao E em comando)
static void verificar_argumentos(Node *no, Simbolo *s, Semantico *sem) {
    int n_args = 0;
    Tipo tipos_args[MAX_PARAMS];
    for (Node *arg = no->lista; arg; arg = arg->prox) {
        Tipo t = tipo_expr(arg, sem);                   // calcula o tipo de cada argumento
        if (n_args < MAX_PARAMS) tipos_args[n_args] = t;
        n_args++;
    }
    if (n_args != s->n_params) {                        // 1) quantidade confere?
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "chamada com numero incorreto de argumentos (esperado %d, encontrado %d) para",
                 s->n_params, n_args);
        erro_sem(sem, no->linha, msg, no->nome);
        return;
    }
    for (int i = 0; i < n_args && i < MAX_PARAMS; i++) {   // 2) confere tipo por tipo
        if (tipos_args[i] != TIPO_INDEFINIDO && !atribuicao_compativel(s->param_tipos[i], tipos_args[i])) {
            char msg[160];
            snprintf(msg, sizeof(msg),
                     "tipo do argumento %d incompativel (esperado %s, encontrado %s) na chamada de",
                     i + 1, tipo_nome(s->param_tipos[i]), tipo_nome(tipos_args[i]));
            erro_sem(sem, no->linha, msg, no->nome);
        }
    }
}

// Se o no e o NOME de um vetor numerico declarado, devolve 1 e o tipo/tamanho dele
static int operando_vetor(Node *no, Semantico *sem, Tipo *elem, int *tam) {
    if (!no || no->kind != ND_VAR) return 0;
    Simbolo *s = buscar_simbolo(sem, no->nome);
    if (!s || !s->is_vetor) return 0;
    if (!eh_numerico(s->tipo)) {
        erro_sem(sem, no->linha, "operacao vetorial exige vetor numerico (inteira, flut ou duplocarpado)", no->nome);
    }
    *elem = s->tipo;
    *tam = s->tam_vetor;
    no->tipo_resultado = s->tipo;               // anota para o gerador
    return 1;
}

// mostrar(...) / escreve(...): aceita qualquer valor imprimivel
static void verificar_impressao(Node *no, Semantico *sem) {
    for (Node *a = no->lista; a; a = a->prox) {
        Tipo t = tipo_expr(a, sem);
        if (t != TIPO_INDEFINIDO && !eh_numerico(t) && t != TIPO_BOOL && t != TIPO_STRING) {
            erro_sem(sem, no->linha, "valor nao imprimivel (so inteira, flut, duplocarpado, bool ou palavra) em", no->nome);
        }
    }
}

// somavet(v), maxvet(v), prodesc(a, b): reducoes da DSL vetorial
static Tipo verificar_reducao(Node *no, Semantico *sem, Embutida bi) {
    int esperado = (bi == BI_PRODESC) ? 2 : 1;
    int n = 0;
    for (Node *a = no->lista; a; a = a->prox) n++;
    if (n != esperado) {
        char msg[128];
        snprintf(msg, sizeof(msg), "chamada com numero incorreto de argumentos (esperado %d, encontrado %d) para", esperado, n);
        erro_sem(sem, no->linha, msg, no->nome);
        return TIPO_INDEFINIDO;
    }
    Tipo t1, t2; int n1, n2;
    if (!operando_vetor(no->lista, sem, &t1, &n1)) {
        erro_sem(sem, no->linha, "o argumento deve ser o nome de um vetor numerico em", no->nome);
        return TIPO_INDEFINIDO;
    }
    if (!eh_numerico(t1)) return TIPO_INDEFINIDO;           // ja reportado: evita erro em cascata
    if (bi != BI_PRODESC) return t1;                        // soma/maximo: mesmo tipo do elemento
    if (!operando_vetor(no->lista->prox, sem, &t2, &n2)) {
        erro_sem(sem, no->linha, "o argumento 2 deve ser o nome de um vetor numerico em", no->nome);
        return TIPO_INDEFINIDO;
    }
    if (!eh_numerico(t2)) return TIPO_INDEFINIDO;
    if (n1 != n2) {
        char msg[160];
        snprintf(msg, sizeof(msg), "vetores de tamanhos diferentes (%d e %d) em", n1, n2);
        erro_sem(sem, no->linha, msg, no->nome);
    }
    return tipo_mais_largo(t1, t2);                         // promocao: inteira . flut -> flut
}

// Calcula o TIPO de uma expressao (funcao recursiva, desce ate as folhas da arvore).
// O resultado tambem fica ANOTADO em no->tipo_resultado (o gerador de codigo usa).
static Tipo tipo_expr_calc(Node *no, Semantico *sem) {
    switch (no->kind) {
        case ND_NUM: return no->tipo_resultado;         // casos-base: o tipo ja e conhecido
        case ND_STRING_LIT: return TIPO_STRING;
        case ND_BOOL_LIT: return TIPO_BOOL;

        case ND_VAR: {                                  // uso de variavel
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (!s) {                                   // AQUI e detectado "uso de nao declarado"
                erro_sem(sem, no->linha, "uso de identificador nao declarado", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (s->is_funcao) {                         // "x receba soma;" sem parenteses
                erro_sem(sem, no->linha, "uso de funcao como variavel", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (s->is_vetor) {                          // vetor inteiro so vale numa operacao vetorial
                erro_sem(sem, no->linha,
                         "vetor usado sem indice (use nome{i}); operacoes vetoriais so aceitam a forma 'c receba a + b;'",
                         no->nome);
                return TIPO_INDEFINIDO;
            }
            return s->tipo;                             // (TIPO_REGISTRO e rejeitado por quem usa)
        }

        case ND_ACESSO_VETOR: {                         // leitura v{i}
            Simbolo *s = buscar_simbolo(sem, no->nome);
            Tipo tipo_indice = tipo_expr(no->filho1, sem);      // RECURSAO no indice
            if (!s) {
                erro_sem(sem, no->linha, "uso de vetor nao declarado", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (!s->is_vetor) {
                erro_sem(sem, no->linha, "identificador nao e um vetor", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (tipo_indice != TIPO_INT && tipo_indice != TIPO_INDEFINIDO) {   // indice tem que ser inteira
                erro_sem(sem, no->linha, "indice de vetor deve ser do tipo inteira", no->nome);
            }
            checar_indice_constante(sem, no->filho1, s, no->linha);   // constante: verifica ja
            return s->tipo;                             // tipo do elemento = tipo do vetor
        }

        case ND_CAMPO_EXPR: {                           // leitura a.campo
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (!s) { erro_sem(sem, no->linha, "uso de registro nao declarado", no->nome); return TIPO_INDEFINIDO; }
            if (s->tipo != TIPO_REGISTRO || s->is_vetor) {
                erro_sem(sem, no->linha, "identificador nao e uma variavel de registro", no->nome);
                return TIPO_INDEFINIDO;
            }
            DefRegistro *r = buscar_registro(sem->registros, s->nome_reg);
            for (int i = 0; r && i < r->n_campos; i++) {
                if (strcmp(r->campo_nome[i], no->campo) == 0) {
                    no->aux = i;                        // anota o indice do campo (getelementptr)
                    strcpy(no->nome_reg, r->nome);
                    return r->campo_tipo[i];
                }
            }
            erro_sem(sem, no->linha, "campo inexistente no registro", no->campo);
            return TIPO_INDEFINIDO;
        }

        case ND_RAIZ_EXPR: {
            Tipo t = tipo_expr(no->filho1, sem);
            if (!eh_numerico(t) && t != TIPO_INDEFINIDO) {
                erro_sem(sem, no->linha, "raiz() exige argumento numerico", "");
            }
            sem->usa_duplo = 1;
            return TIPO_DOUBLE;                         // raiz SEMPRE devolve duplocarpado
        }

        case ND_BINOP: {                                // + - * / // ^
            Tipo te = tipo_expr(no->filho1, sem);       // RECURSAO lado esquerdo
            Tipo td = no->filho2 ? tipo_expr(no->filho2, sem) : te;   // RECURSAO lado direito
            if ((!eh_numerico(te) && te != TIPO_INDEFINIDO) ||
                (!eh_numerico(td) && td != TIPO_INDEFINIDO)) {
                erro_sem(sem, no->linha, "operador aritmetico exige operandos numericos", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (strcmp(no->nome, "/") == 0 || strcmp(no->nome, "//") == 0) {   // divisor constante zero
                if (no->filho2 && no->filho2->kind == ND_NUM && no->filho2->num_valor == 0.0)
                    erro_sem(sem, no->linha, "divisao por zero (divisor constante)", no->nome);
            }
            if (te == TIPO_INDEFINIDO || td == TIPO_INDEFINIDO) {   // um lado ja deu erro antes:
                if (strcmp(no->nome, "//") == 0) return TIPO_INT;   // nao gera erro repetido (evita cascata)
                return (te == TIPO_INDEFINIDO) ? td : te;
            }
            Tipo t = tipo_mais_largo(te, td);           // resultado = o tipo "maior"
            // PjBL 3 (decisoes_backend.md 1.2): "/" e SEMPRE divisao real (inteira / inteira -> flut)
            // e "//" e SEMPRE divisao inteira (resultado inteira, truncando para zero)
            if (strcmp(no->nome, "/") == 0) return tipo_mais_largo(t, TIPO_FLOAT);
            if (strcmp(no->nome, "//") == 0) return TIPO_INT;
            return t;
        }

        case ND_CHAMADA_EXPR: {                         // chamada dentro de expressao: x receba f(1, 2)
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (!s || !s->is_funcao) {
                erro_sem(sem, no->linha, "chamada de funcao nao declarada", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (s->embutida == BI_SOMAVET || s->embutida == BI_MAXVET || s->embutida == BI_PRODESC)
                return verificar_reducao(no, sem, s->embutida);
            if (s->embutida == BI_MOSTRAR || s->embutida == BI_ESCREVE) verificar_impressao(no, sem);
            else verificar_argumentos(no, s, sem);
            if (s->tipo_retorno == TIPO_VOID) {         // funcao vazio nao tem valor para usar
                erro_sem(sem, no->linha, "funcao do tipo vazio usada como valor", no->nome);
                return TIPO_INDEFINIDO;
            }
            if (s->embutida == BI_LEDUPLO) sem->usa_duplo = 1;
            return s->tipo_retorno;                     // o tipo da chamada e o tipo de retorno da funcao
        }

        default:
            return TIPO_INDEFINIDO;
    }
}

static Tipo tipo_expr(Node *no, Semantico *sem) {
    if (!no) return TIPO_INDEFINIDO;
    Tipo t = tipo_expr_calc(no, sem);
    no->tipo_resultado = t;                             // ANOTA a arvore para o gerador de codigo
    return t;
}

// Verifica uma condicao (de cond/repete/durante). Nao devolve tipo: so valida.
static void verificar_condicao(Node *no, Semantico *sem) {
    if (!no) return;
    switch (no->kind) {
        case ND_COMPARACAO: {                           // a < b, x == VER, ...
            Tipo te = tipo_expr(no->filho1, sem);
            Tipo td = tipo_expr(no->filho2, sem);
            int algum_indef = (te == TIPO_INDEFINIDO || td == TIPO_INDEFINIDO);
            if (!algum_indef) {                         // se um lado ja deu erro, nao reclama de novo
                int ok = (eh_numerico(te) && eh_numerico(td)) ||              // numero com numero: qualquer comparador
                         (te == TIPO_BOOL && td == TIPO_BOOL &&               // bool com bool: so == e !
                          (strcmp(no->nome, "==") == 0 || strcmp(no->nome, "!") == 0));
                if (!ok) {
                    erro_sem(sem, no->linha,
                             "operandos de comparacao com tipos incompativeis entre si", "");
                }
            }
            break;
        }
        case ND_LOGICO_BIN:                             // OU / E / XOR: verifica os dois lados
            verificar_condicao(no->filho1, sem);
            verificar_condicao(no->filho2, sem);
            break;
        case ND_LOGICO_NOT:                             // NAO: verifica o que vem depois
            verificar_condicao(no->filho1, sem);
            break;
        default:
            break;
    }
}

// c receba a + b;  /  c receba 3 * a;  (DSL vetorial, PjBL 4 Parte A)
static void verificar_atrib_vetorial(Node *no, Simbolo *s, Semantico *sem) {
    Node *v = no->filho1;
    int eh_op = v && v->kind == ND_BINOP &&
                (strcmp(v->nome, "+") == 0 || strcmp(v->nome, "-") == 0 || strcmp(v->nome, "*") == 0);
    Tipo te = TIPO_INDEFINIDO, td = TIPO_INDEFINIDO;
    int ne = 0, nd = 0;
    int ve = eh_op && operando_vetor(v->filho1, sem, &te, &ne);
    int vd = eh_op && operando_vetor(v->filho2, sem, &td, &nd);
    if (!ve && !vd) {
        erro_sem(sem, no->linha,
                 "atribuicao direta a um vetor (use nome{i} receba ..., ou uma operacao vetorial: a + b, a - b, a * b, k * a)",
                 no->nome);
        if (v) tipo_expr(v, sem);
        return;
    }
    if (!ve) te = tipo_expr(v->filho1, sem);    // lado escalar (k * a)
    if (!vd) td = tipo_expr(v->filho2, sem);
    if ((te != TIPO_INDEFINIDO && !eh_numerico(te)) || (td != TIPO_INDEFINIDO && !eh_numerico(td))) {
        erro_sem(sem, no->linha, "operacao vetorial exige operandos numericos", no->nome);
        return;
    }
    int n = ve ? ne : nd;
    char msg[200];
    if (ve && vd && ne != nd) {
        snprintf(msg, sizeof(msg), "operandos de tamanhos diferentes (%d e %d) na operacao vetorial atribuida a", ne, nd);
        erro_sem(sem, no->linha, msg, no->nome);
    } else if (n != s->tam_vetor) {
        snprintf(msg, sizeof(msg), "vetores de tamanhos diferentes (destino %d, operando %d) na operacao vetorial atribuida a",
                 s->tam_vetor, n);
        erro_sem(sem, no->linha, msg, no->nome);
    }
    Tipo res = tipo_mais_largo(te, td);         // promocao elemento a elemento (mesma regra do escalar)
    v->tipo_resultado = res;
    if (res != TIPO_INDEFINIDO && !atribuicao_compativel(s->tipo, res)) {
        char msg[160];
        snprintf(msg, sizeof(msg), "incompatibilidade de tipos na operacao vetorial (resultado %s, destino %s) em",
                 tipo_nome(res), tipo_nome(s->tipo));
        erro_sem(sem, no->linha, msg, no->nome);
    }
    no->eh_vetorial = 1;                        // anota: o gerador emite um nucleo MLIR aqui
}

/* ---------- Comandos ---------- */
static void verificar_comandos(Node *lista, Semantico *sem, int *tem_respost);

// Declaracoes: as mesmas regras valem para local (em funcao) e global (topo do arquivo)
static void verificar_declaracao(Node *no, Semantico *sem) {
    switch (no->kind) {
        case ND_DECL_VAR:                               // declaracao: so cadastra na tabela
            if (inserir_simbolo(sem, no->linha, no->nome, no->tipo_declarado, 0))
                contar_memoria(sem, no->tipo_declarado, 0, 1, "");
            break;
        case ND_DECL_VETOR: {
            long tam = (long)no->num_valor;
            if (no->num_valor != (double)tam || tam < 1) {
                erro_sem(sem, no->linha, "tamanho de vetor deve ser um numero inteiro maior que zero", no->nome);
                tam = 1;
            }
            Simbolo *s = inserir_simbolo(sem, no->linha, no->nome, no->tipo_declarado, 1);
            if (s) { s->tam_vetor = (int)tam; contar_memoria(sem, no->tipo_declarado, 1, tam, ""); }
            break;
        }
        case ND_DECL_REG: {                             // Aluno a;
            DefRegistro *r = buscar_registro(sem->registros, no->nome_reg);
            if (!r) { erro_sem(sem, no->linha, "tipo registro nao declarado", no->nome_reg); break; }
            Simbolo *s = inserir_simbolo(sem, no->linha, no->nome, TIPO_REGISTRO, 0);
            if (s) { strcpy(s->nome_reg, r->nome); contar_memoria(sem, TIPO_REGISTRO, 0, 1, r->nome); }
            break;
        }
        default: break;
    }
}

// Verifica UM comando. tem_respost e ponteiro para que qualquer nivel de aninhamento
// consiga marcar "achei um respost" na mesma variavel.
static void verificar_comando(Node *no, Semantico *sem, int *tem_respost) {
    switch (no->kind) {
        case ND_DECL_VAR: case ND_DECL_VETOR: case ND_DECL_REG:
            verificar_declaracao(no, sem);
            break;

        case ND_ATRIB: {                                // x receba valor;
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (s && s->is_vetor && !s->is_funcao) {    // destino vetor -> operacao vetorial da DSL
                verificar_atrib_vetorial(no, s, sem);
                break;
            }
            Tipo t_val = tipo_expr(no->filho1, sem);    // tipo do lado direito
            if (!s) {                                   // checagens em cascata:
                erro_sem(sem, no->linha, "atribuicao a identificador nao declarado", no->nome);   // 1) existe?
            } else if (s->is_funcao) {
                erro_sem(sem, no->linha, "atribuicao a um nome de funcao", no->nome);             // 2) e funcao?
            } else if (s->tipo == TIPO_REGISTRO) {
                erro_sem(sem, no->linha, "atribuicao direta a um registro (atribua campo a campo: nome.campo receba ...)", no->nome);
            } else if (t_val != TIPO_INDEFINIDO && !atribuicao_compativel(s->tipo, t_val)) {
                erro_sem(sem, no->linha, "incompatibilidade de tipos na atribuicao a", no->nome); // 3) tipo combina?
            }
            break;
        }

        case ND_ATRIB_VETOR: {                          // v{i} receba valor;
            Simbolo *s = buscar_simbolo(sem, no->nome);
            Tipo t_idx = tipo_expr(no->filho2, sem);    // tipo do indice
            Tipo t_val = tipo_expr(no->filho1, sem);    // tipo do valor
            if (!s) {
                erro_sem(sem, no->linha, "atribuicao a vetor nao declarado", no->nome);
            } else if (!s->is_vetor) {
                erro_sem(sem, no->linha, "identificador nao e um vetor", no->nome);
            } else {
                if (t_idx != TIPO_INT && t_idx != TIPO_INDEFINIDO) {
                    erro_sem(sem, no->linha, "indice de vetor deve ser do tipo inteira", no->nome);
                }
                checar_indice_constante(sem, no->filho2, s, no->linha);
                if (t_val != TIPO_INDEFINIDO && !atribuicao_compativel(s->tipo, t_val)) {
                    erro_sem(sem, no->linha, "incompatibilidade de tipos na atribuicao ao vetor", no->nome);
                }
            }
            break;
        }

        case ND_ATRIB_CAMPO: {                          // a.campo receba valor;
            Node leitura = *no;                         // reaproveita a regra da leitura de campo
            leitura.kind = ND_CAMPO_EXPR;
            Tipo t_campo = tipo_expr(&leitura, sem);
            no->aux = leitura.aux;
            strcpy(no->nome_reg, leitura.nome_reg);
            Tipo t_val = tipo_expr(no->filho1, sem);
            if (t_campo != TIPO_INDEFINIDO && t_val != TIPO_INDEFINIDO && !atribuicao_compativel(t_campo, t_val)) {
                erro_sem(sem, no->linha, "incompatibilidade de tipos na atribuicao ao campo", no->campo);
            }
            break;
        }

        case ND_COND:                                   // if
            if (no->filho1) verificar_condicao(no->filho1, sem);   // filho1 NULL = else final (sem condicao)
            entrar_escopo(sem);                         // cada bloco tem seu proprio escopo
            verificar_comandos(no->lista, sem, tem_respost);
            sair_escopo(sem);
            if (no->filho2) verificar_comando(no->filho2, sem, tem_respost);   // RECURSAO no proximo ramo
            break;

        case ND_REPETE:                                 // for
            entrar_escopo(sem);
            verificar_comando(no->filho1, sem, tem_respost);   /* inicializacao (e um ND_ATRIB comum) */
            verificar_condicao(no->filho2, sem);
            verificar_comandos(no->lista, sem, tem_respost);
            verificar_comando(no->filho3, sem, tem_respost);   /* incremento (tambem ND_ATRIB) */
            sair_escopo(sem);
            break;

        case ND_DURANTE:                                // while
            verificar_condicao(no->filho1, sem);
            entrar_escopo(sem);
            verificar_comandos(no->lista, sem, tem_respost);
            sair_escopo(sem);
            break;

        case ND_RESPOST: {                              // return
            *tem_respost = 1;                           // avisa verificar_corpo_funcao que achou um respost
            Tipo esperado = sem->tipo_retorno_funcao_atual;   // tipo da funcao em que estamos
            if (no->filho1 == NULL) {                   // "respost;" sem valor
                if (esperado != TIPO_VOID) {            // so vale em funcao vazio
                    erro_sem(sem, no->linha,
                             "'respost' sem valor em funcao que nao e do tipo vazio", "");
                }
            } else {                                    // "respost valor;"
                Tipo t_val = tipo_expr(no->filho1, sem);
                if (esperado == TIPO_VOID) {            // funcao vazio nao pode devolver valor
                    erro_sem(sem, no->linha,
                             "funcao do tipo vazio nao deve retornar um valor ('respost' com valor)", "");
                } else if (t_val != TIPO_INDEFINIDO && !atribuicao_compativel(esperado, t_val)) {
                    erro_sem(sem, no->linha,
                             "tipo de retorno incompativel com o tipo declarado da funcao", "");
                }
            }
            break;
        }

        case ND_CHAMADA_STMT: {                         // chamada como comando: f(x);
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (!s || !s->is_funcao) {
                erro_sem(sem, no->linha, "chamada de funcao nao declarada", no->nome);
                break;
            }
            if (s->embutida == BI_MOSTRAR || s->embutida == BI_ESCREVE) verificar_impressao(no, sem);
            else if (s->embutida == BI_SOMAVET || s->embutida == BI_MAXVET || s->embutida == BI_PRODESC)
                verificar_reducao(no, sem, s->embutida);
            else verificar_argumentos(no, s, sem);      // agora confere QUANTIDADE e TIPO (corrigido no PjBL 3)
            break;
        }

        default:
            break;
    }
}

// Percorre uma lista de comandos (ligados por ->prox), um por um
static void verificar_comandos(Node *lista, Semantico *sem, int *tem_respost) {
    for (Node *no = lista; no; no = no->prox) {
        verificar_comando(no, sem, tem_respost);
    }
}

// Cadastra um tipo registro (nome e campos), conferindo nomes repetidos
static void registrar_registro(Node *reg, Semantico *sem) {
    if (buscar_registro(sem->registros, reg->nome)) {
        erro_sem(sem, reg->linha, "registro ja declarado", reg->nome);
        return;
    }
    DefRegistro *r = calloc(1, sizeof(DefRegistro));
    strncpy(r->nome, reg->nome, sizeof(r->nome) - 1);
    for (Node *c = reg->lista; c; c = c->prox) {
        int repetido = 0;
        for (int i = 0; i < r->n_campos; i++) if (strcmp(r->campo_nome[i], c->nome) == 0) repetido = 1;
        if (repetido) { erro_sem(sem, c->linha, "campo repetido no registro", c->nome); continue; }
        if (r->n_campos >= MAX_CAMPOS) { erro_sem(sem, c->linha, "registro com campos demais", reg->nome); break; }
        strncpy(r->campo_nome[r->n_campos], c->nome, 63);
        r->campo_tipo[r->n_campos] = c->tipo_declarado;
        if (c->tipo_declarado == TIPO_VOID) erro_sem(sem, c->linha, "campo de registro nao pode ser vazio", c->nome);
        if (c->tipo_declarado == TIPO_DOUBLE) sem->usa_duplo = 1;
        r->n_campos++;
    }
    if (r->n_campos == 0) erro_sem(sem, reg->linha, "registro sem nenhum campo", reg->nome);
    DefRegistro **fim = &sem->registros;        // insere no FIM (mantem a ordem do arquivo)
    while (*fim) fim = &(*fim)->proximo;
    *fim = r;
}

// PASSO 1: cadastra so o CABECALHO da funcao (nome, retorno, tipos dos parametros)
static void registrar_assinatura_funcao(Node *func, Semantico *sem) {
    if (embutida_por_nome(func->nome) != BI_NENHUM) {
        erro_sem(sem, func->linha, "nome reservado para funcao embutida", func->nome);
        return;
    }
    Simbolo *s = inserir_simbolo(sem, func->linha, func->nome, func->tipo_declarado, 0);
    if (!s) return;                                 // nome repetido: inserir_simbolo ja reportou
    s->is_funcao = 1;
    s->tipo_retorno = func->tipo_declarado;
    s->n_params = 0;
    for (Node *param = func->filho1; param; param = param->prox) {
        if (s->n_params >= MAX_PARAMS) { erro_sem(sem, func->linha, "funcao com parametros demais", func->nome); break; }
        s->param_tipos[s->n_params++] = param->tipo_declarado;   // guarda o tipo de cada parametro, na ordem
        if (param->tipo_declarado == TIPO_VOID) erro_sem(sem, param->linha, "parametro nao pode ser do tipo vazio", param->nome);
        // parametros da principal sao lidos da entrada (decisoes_backend.md 2.3): so numericos
        if (func->num_valor != 0 && !eh_numerico(param->tipo_declarado))
            erro_sem(sem, param->linha, "parametro da principal deve ser numerico (e lido da entrada padrao)", param->nome);
    }
}

// PASSO 2: verifica o CORPO de uma funcao
static void verificar_corpo_funcao(Node *func, Semantico *sem) {
    entrar_escopo(sem);                             // escopo da funcao
    sem->em_funcao = 1;
    sem->bytes_locais = 0;
    for (Node *param = func->filho1; param; param = param->prox) {
        if (inserir_simbolo(sem, param->linha, param->nome, param->tipo_declarado, 0))   // parametros viram variaveis locais
            contar_memoria(sem, param->tipo_declarado, 0, 1, "");
    }
    sem->tipo_retorno_funcao_atual = func->tipo_declarado;   // "contexto" para o ND_RESPOST

    int tem_respost = 0;
    verificar_comandos(func->lista, sem, &tem_respost);      // passa o ENDERECO: qualquer nivel pode marcar

    if (func->tipo_declarado != TIPO_VOID && !tem_respost) { // funcao nao-vazio sem nenhum respost
        erro_sem(sem, func->linha,
                 "funcao com tipo de retorno diferente de vazio nao possui nenhum comando 'respost'",
                 func->nome);
    }

    // Verificacao de memoria por alvo (PjBL 4, Parte D): locais + globais cabem na RAM?
    long ram = ram_do_alvo(sem->alvo);
    if (ram > 0 && sem->bytes_globais <= ram && sem->bytes_locais + sem->bytes_globais > ram) {
        char msg[220];
        snprintf(msg, sizeof(msg),
                 "variaveis locais (%ld bytes) + globais (%ld bytes) excedem a RAM disponivel no alvo (%ld de 2048 bytes) na funcao",
                 sem->bytes_locais, sem->bytes_globais, ram);
        erro_sem(sem, func->linha, msg, func->nome);
    }
    sem->em_funcao = 0;
    sair_escopo(sem);                               // descarta parametros e variaveis locais
}

// Cadastra as funcoes embutidas no escopo global (antes de tudo)
static void registrar_embutidas(Semantico *sem) {
    for (int i = 0; EMBUTIDAS[i].nome; i++) {
        Simbolo *s = inserir_simbolo(sem, 0, EMBUTIDAS[i].nome, EMBUTIDAS[i].ret, 0);
        s->is_funcao = 1;
        s->embutida = EMBUTIDAS[i].id;
        s->tipo_retorno = EMBUTIDAS[i].ret;
        s->n_params = EMBUTIDAS[i].n < 0 ? 0 : EMBUTIDAS[i].n;
        for (int k = 0; k < s->n_params; k++) s->param_tipos[k] = EMBUTIDAS[i].p[k];
    }
    sem->usa_duplo = 0;                             // (leduplocarpado nao conta ate ser usada)
}

// Unica funcao chamada pelo main. Faz a verificacao em DOIS passos.
void semantico_verificar(Semantico *sem, Node *raiz) {
    entrar_escopo(sem); /* escopo global */
    registrar_embutidas(sem);

    // PASSO 1: registros, variaveis globais e assinaturas de TODAS as funcoes, antes de
    // olhar qualquer corpo. Assim uma funcao pode chamar outra declarada mais abaixo
    // (referencia para frente) e usar uma global declarada em qualquer ponto do arquivo.
    for (Node *no = raiz->lista; no; no = no->prox)
        if (no->kind == ND_REGISTRO) registrar_registro(no, sem);
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind == ND_DECL_VAR || no->kind == ND_DECL_VETOR || no->kind == ND_DECL_REG)
            verificar_declaracao(no, sem);          // global
    }
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind == ND_FUNCAO) {
            registrar_assinatura_funcao(no, sem);
            if (no->num_valor != 0) sem->viu_principal++;   // conta quantas principal existem
        }
    }

    if (sem->viu_principal == 0) {                  // tem que existir exatamente UMA principal
        erro_sem(sem, 0, "nenhuma funcao 'principal' foi encontrada no programa", "");
    } else if (sem->viu_principal > 1) {
        erro_sem(sem, 0, "mais de uma funcao 'principal' foi declarada", "");
    }
    long ram = ram_do_alvo(sem->alvo);
    if (ram > 0 && sem->bytes_globais > ram) {
        char msg[160];
        snprintf(msg, sizeof(msg), "variaveis globais (%ld bytes) excedem a RAM disponivel no alvo (%ld de 2048 bytes)",
                 sem->bytes_globais, ram);
        erro_sem(sem, 0, msg, "");
    }

    // PASSO 2: so agora verifica o corpo de cada funcao
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind == ND_FUNCAO) {
            verificar_corpo_funcao(no, sem);
        }
    }

    sair_escopo(sem);                               // libera o escopo global
}
