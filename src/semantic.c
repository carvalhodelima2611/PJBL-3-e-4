/* semantic.c - Analisador Semantico da linguagem Macaronica
 * Tabela de simbolos com escopos, declaracao antes do uso, tipos,
 * assinatura de funcao, exatamente uma principal, regra do respost. */
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

void semantico_iniciar(Semantico *sem) {
    sem->escopo_atual = NULL;
    sem->escopo_global = NULL;
    sem->erros_semanticos = 0;
    sem->tipo_retorno_funcao_atual = TIPO_INDEFINIDO;
    sem->viu_principal = 0;
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
    strcpy(s->nome, nome);
    s->tipo = tipo;
    s->is_vetor = is_vetor;
    s->proximo = sem->escopo_atual->simbolos;           // insere no INICIO da lista (mais rapido)
    sem->escopo_atual->simbolos = s;
    return s;
}

// "Regua" de amplitude numerica: inteira(1) < flut(2) < duplocarpado(3)
// Decisao do grupo (decisoes.md 2.5): so pode converter "para cima"
static int rank_numerico(Tipo t) {
    switch (t) {
        case TIPO_INT: return 1;
        case TIPO_FLOAT: return 2;
        case TIPO_DOUBLE: return 3;
        default: return -1;         // bool, palavra, vazio: nao sao numericos
    }
}
static int eh_numerico(Tipo t) { return rank_numerico(t) > 0; }

// Pode guardar um valor do tipo "origem" numa variavel do tipo "destino"?
static int atribuicao_compativel(Tipo destino, Tipo origem) {
    if (destino == origem) return 1;                            // mesmo tipo: sempre pode
    if (eh_numerico(destino) && eh_numerico(origem)) {
        return rank_numerico(origem) <= rank_numerico(destino); // inteira -> flut pode; flut -> inteira nao
    }
    return 0;                                                   // qualquer outra mistura: erro
}

// Calcula o TIPO de uma expressao (funcao recursiva, desce ate as folhas da arvore)
static Tipo tipo_expr(Node *no, Semantico *sem) {
    if (!no) return TIPO_INDEFINIDO;
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
            return s->tipo;
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
            }
            if (tipo_indice != TIPO_INT && tipo_indice != TIPO_INDEFINIDO) {   // indice tem que ser inteira
                erro_sem(sem, no->linha, "indice de vetor deve ser do tipo inteira", no->nome);
            }
            return s->tipo;                             // tipo do elemento = tipo do vetor
        }                                               // (nao checa se o indice passa do tamanho)

        case ND_RAIZ_EXPR: {
            Tipo t = tipo_expr(no->filho1, sem);
            if (!eh_numerico(t) && t != TIPO_INDEFINIDO) {
                erro_sem(sem, no->linha, "raiz() exige argumento numerico", "");
            }
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
            if (te == TIPO_INDEFINIDO) return td;       // um lado ja deu erro antes:
            if (td == TIPO_INDEFINIDO) return te;       // nao gera erro repetido (evita cascata)
            return (rank_numerico(te) >= rank_numerico(td)) ? te : td;   // resultado = o tipo "maior"
            // ⚠️ consequencia: inteira / inteira da inteira (7 / 2 cabe numa variavel inteira)
        }

        case ND_CHAMADA_EXPR: {                         // chamada dentro de expressao: x receba f(1, 2)
            Simbolo *s = buscar_simbolo(sem, no->nome);
            if (!s || !s->is_funcao) {
                erro_sem(sem, no->linha, "chamada de funcao nao declarada", no->nome);
                return TIPO_INDEFINIDO;
            }
            int n_args = 0;
            Node *arg = no->lista;
            Tipo tipos_args[MAX_PARAMS];
            while (arg && n_args < MAX_PARAMS) {        // calcula o tipo de cada argumento
                tipos_args[n_args] = tipo_expr(arg, sem);
                n_args++;
                arg = arg->prox;
            }
            if (n_args != s->n_params) {                // 1) quantidade confere?
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "chamada com numero incorreto de argumentos (esperado %d, encontrado %d) para",
                         s->n_params, n_args);
                erro_sem(sem, no->linha, msg, no->nome);
            } else {                                    // 2) se a quantidade bateu, confere tipo por tipo
                for (int i = 0; i < n_args; i++) {
                    if (!atribuicao_compativel(s->param_tipos[i], tipos_args[i]) &&
                        tipos_args[i] != TIPO_INDEFINIDO) {
                        char msg[160];
                        snprintf(msg, sizeof(msg),
                                 "tipo do argumento %d incompativel (esperado %s, encontrado %s) na chamada de",
                                 i + 1, tipo_nome(s->param_tipos[i]), tipo_nome(tipos_args[i]));
                        erro_sem(sem, no->linha, msg, no->nome);
                    }
                }
            }
            return s->tipo_retorno;                     // o tipo da chamada e o tipo de retorno da funcao
        }

        default:
            return TIPO_INDEFINIDO;
    }
}

// Verifica uma condicao (de cond/repete/durante). Nao devolve tipo: so valida.
static void verificar_condicao(Node *no, Semantico *sem) {
    if (!no) return;
    switch (no->kind) {
        case ND_COMPARACAO: {                           // a < b, x == VER, ...
            Tipo te = tipo_expr(no->filho1, sem);
            Tipo td = tipo_expr(no->filho2, sem);
            int ambos_indef = (te == TIPO_INDEFINIDO || td == TIPO_INDEFINIDO);   // (nome enganoso: e "ALGUM indefinido")
            if (!ambos_indef) {                         // se um lado ja deu erro, nao reclama de novo
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

/* ---------- Comandos ---------- */
static void verificar_comandos(Node *lista, Semantico *sem, int *tem_respost);

// Verifica UM comando. tem_respost e ponteiro para que qualquer nivel de aninhamento
// consiga marcar "achei um respost" na mesma variavel.
static void verificar_comando(Node *no, Semantico *sem, int *tem_respost) {
    switch (no->kind) {
        case ND_DECL_VAR:                               // declaracao: so cadastra na tabela
            inserir_simbolo(sem, no->linha, no->nome, no->tipo_declarado, 0);
            break;

        case ND_DECL_VETOR:
            inserir_simbolo(sem, no->linha, no->nome, no->tipo_declarado, 1);   // is_vetor = 1
            break;

        case ND_ATRIB: {                                // x receba valor;
            Simbolo *s = buscar_simbolo(sem, no->nome);
            Tipo t_val = tipo_expr(no->filho1, sem);    // tipo do lado direito
            if (!s) {                                   // 4 checagens em cascata:
                erro_sem(sem, no->linha, "atribuicao a identificador nao declarado", no->nome);   // 1) existe?
            } else if (s->is_funcao) {
                erro_sem(sem, no->linha, "atribuicao a um nome de funcao", no->nome);             // 2) e funcao?
            } else if (s->is_vetor) {
                erro_sem(sem, no->linha,
                         "atribuicao direta a um vetor (use acesso com indice: nome{i} receba ...)", no->nome); // 3) e vetor?
            } else if (t_val != TIPO_INDEFINIDO && !atribuicao_compativel(s->tipo, t_val)) {
                erro_sem(sem, no->linha, "incompatibilidade de tipos na atribuicao a", no->nome); // 4) tipo combina?
            }
            break;
        }

        case ND_ATRIB_VETOR: {                          // v{i} receba valor;
            Simbolo *s = buscar_simbolo(sem, no->nome);
            Tipo t_idx = tipo_expr(no->filho2, sem);    // tipo do indice
            Tipo t_val = tipo_expr(no->filho1, sem);    // tipo do valor
            if (!s) {
                erro_sem(sem, no->linha, "atribuicao a vetor nao declarado", no->nome);
            } else {
                if (!s->is_vetor) erro_sem(sem, no->linha, "identificador nao e um vetor", no->nome);
                if (t_idx != TIPO_INT && t_idx != TIPO_INDEFINIDO) {
                    erro_sem(sem, no->linha, "indice de vetor deve ser do tipo inteira", no->nome);
                }
                if (t_val != TIPO_INDEFINIDO && !atribuicao_compativel(s->tipo, t_val)) {
                    erro_sem(sem, no->linha, "incompatibilidade de tipos na atribuicao ao vetor", no->nome);
                }
            }
            break;
        }

        case ND_COND:                                   // if
            if (no->filho1) verificar_condicao(no->filho1, sem);   // filho1 NULL = else final (sem condicao)
            entrar_escopo(sem);                         // cada bloco tem seu proprio escopo
            verificar_comandos(no->lista, sem, tem_respost);
            sair_escopo(sem);
            if (no->filho2) verificar_comando(no->filho2, sem, tem_respost);   // RECURSAO no proximo ramo (else if / else)
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
            int n_args = 0;
            for (Node *a = no->lista; a; a = a->prox) { tipo_expr(a, sem); n_args++; }
            // ⚠️ aqui so confere a QUANTIDADE; o tipo de cada argumento nao e comparado
            //    (diferente do ND_CHAMADA_EXPR). "f(VER);" com f(inteira a) passa.
            if (n_args != s->n_params) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "chamada com numero incorreto de argumentos (esperado %d, encontrado %d) para",
                         s->n_params, n_args);
                erro_sem(sem, no->linha, msg, no->nome);
            }
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

// PASSO 1: cadastra so o CABECALHO da funcao (nome, retorno, tipos dos parametros)
static void registrar_assinatura_funcao(Node *func, Semantico *sem) {
    Simbolo *s = inserir_simbolo(sem, func->linha, func->nome, func->tipo_declarado, 0);
    if (!s) return;                                 // nome repetido: inserir_simbolo ja reportou
    s->is_funcao = 1;
    s->tipo_retorno = func->tipo_declarado;
    s->n_params = 0;
    for (Node *param = func->filho1; param && s->n_params < MAX_PARAMS; param = param->prox) {
        s->param_tipos[s->n_params++] = param->tipo_declarado;   // guarda o tipo de cada parametro, na ordem
    }
}

// PASSO 2: verifica o CORPO de uma funcao
static void verificar_corpo_funcao(Node *func, Semantico *sem) {
    entrar_escopo(sem);                             // escopo da funcao
    for (Node *param = func->filho1; param; param = param->prox) {
        inserir_simbolo(sem, param->linha, param->nome, param->tipo_declarado, 0);   // parametros viram variaveis locais
    }
    sem->tipo_retorno_funcao_atual = func->tipo_declarado;   // "contexto" para o ND_RESPOST

    int tem_respost = 0;
    verificar_comandos(func->lista, sem, &tem_respost);      // passa o ENDERECO: qualquer nivel pode marcar

    if (func->tipo_declarado != TIPO_VOID && !tem_respost) { // funcao nao-vazio sem nenhum respost
        erro_sem(sem, func->linha,
                 "funcao com tipo de retorno diferente de vazio nao possui nenhum comando 'respost'",
                 func->nome);
    }
    sair_escopo(sem);                               // descarta parametros e variaveis locais
}

// Unica funcao chamada pelo main. Faz a verificacao em DOIS passos.
void semantico_verificar(Semantico *sem, Node *raiz) {
    entrar_escopo(sem); /* escopo global */

    // PASSO 1: registra a assinatura de TODAS as funcoes antes de olhar qualquer corpo.
    // Assim uma funcao pode chamar outra declarada mais abaixo no arquivo (referencia para frente).
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind == ND_FUNCAO) {
            registrar_assinatura_funcao(no, sem);
            if (no->num_valor != 0) sem->viu_principal++;   // conta quantas principal existem
        }
        // ⚠️ registros (ND_REGISTRO) sao ignorados aqui: nome ou campo repetido nao e detectado.
        //    O USO de registros esta fora do escopo (decisoes.md 2.7).
    }

    if (sem->viu_principal == 0) {                  // tem que existir exatamente UMA principal
        erro_sem(sem, 0, "nenhuma funcao 'principal' foi encontrada no programa", "");
    } else if (sem->viu_principal > 1) {
        erro_sem(sem, 0, "mais de uma funcao 'principal' foi declarada", "");
    }

    // PASSO 2: so agora verifica o corpo de cada funcao
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind == ND_FUNCAO) {
            verificar_corpo_funcao(no, sem);
        }
    }

    sair_escopo(sem);                               // libera o escopo global
}
