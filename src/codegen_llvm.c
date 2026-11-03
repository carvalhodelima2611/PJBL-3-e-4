/* codegen_llvm.c - Gerador de LLVM IR textual da linguagem Macaronica (PjBL 3)
 *
 * Forma "em memoria": toda variavel local vira um alloca no bloco de entrada
 * da funcao, e cada uso vira load/store. O passe mem2reg depois promove
 * essas variaveis a registradores SSA (e e ai que aparecem as instrucoes phi).
 *
 * Regras de portabilidade (enunciado, secao 4.5), todas seguidas aqui:
 *   1. sem "target triple" e sem "target datalayout";
 *   2. tipos de largura fixa (inteira = i32, flut = float, duplocarpado = double, bool = i1);
 *   3. nenhuma funcao variadica: E/S so por funcoes zl_* de assinatura fixa;
 *   4. enderecos sempre por getelementptr com indices i32 (nunca contas a mao);
 *   5. toda funcao gerada ou declarada e nounwind (#0);
 *   6. a principal do programa vira @zl_principal; main/setup ficam no runtime.
 *
 * Nomes na IR (para ficar legivel): funcoes do usuario @mac_<nome>, globais
 * @g_<nome>, variaveis locais %<nome>.addr, parametros %<nome>, temporarios
 * %t.<n>, blocos <estrutura>.<parte>.<n>, nucleos vetoriais @dsl.<op>.<n>. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "codegen.h"

// Codigos de erro de execucao (iguais aos de runtime/zl_runtime.h)
#define ZL_ERRO_INDICE       2
#define ZL_ERRO_DIVISAO_ZERO 3
#define ZL_ERRO_SEM_RETORNO  4

/* ---------- Tabela de simbolos do gerador (onde cada variavel mora na memoria) ---------- */
typedef struct VarCG {
    char nome[64];          // nome no fonte
    char ptr[96];           // ponteiro na IR: %x.addr ou @g_x
    Tipo tipo;              // tipo (do elemento, se vetor)
    int is_vetor, tam;
    char nome_reg[64];      // se registro
    struct VarCG *prox;
} VarCG;

typedef struct EscCG { VarCG *vars; struct EscCG *pai; } EscCG;

typedef struct NomeUsado { char base[80]; int vezes; struct NomeUsado *prox; } NomeUsado;

typedef struct {
    Buf *topo;              // tipos registro, globais, textos, declaracoes
    Buf *mlir;              // nucleos vetoriais
    Buf entrada;            // allocas da funcao atual (vao no bloco "entry")
    Buf corpo;              // instrucoes da funcao atual
    Buf textos;             // constantes de texto @.str.N
    Buf decl_nucleos;       // declare dos nucleos
    int tmp, rotulo, n_textos, n_nucleos;
    int terminado;          // o bloco atual ja terminou (br/ret/unreachable)?
    char bloco[64];         // rotulo do bloco atual (usado nos phi)
    EscCG *esc;
    NomeUsado *usados;
    Node *raiz;
    const Semantico *sem;
    int rd;                 // 1 = rebaixar duplocarpado para float (alvo Uno)
    Tipo ret_atual;
} CG;

/* ---------- Utilitarios ---------- */

static const char *lt(CG *cg, Tipo t) {                 // tipo LLVM de um tipo Macaronica
    switch (t) {
        case TIPO_INT: return "i32";
        case TIPO_FLOAT: return "float";
        case TIPO_DOUBLE: return cg->rd ? "float" : "double";
        case TIPO_BOOL: return "i1";
        case TIPO_STRING: return "ptr";
        case TIPO_VOID: return "void";
        default: return "i32";
    }
}

static int eh_real(Tipo t) { return t == TIPO_FLOAT || t == TIPO_DOUBLE; }

// Identificadores podem ter bytes nao-ASCII (decisoes.md 2.3): troca por _XX para a IR
static void sanitizar(const char *in, char *out, size_t max) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j + 4 < max; i++) {
        unsigned char c = (unsigned char)in[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out[j++] = (char)c;
        else j += (size_t)snprintf(out + j, max - j, "_%02X", c);
    }
    out[j] = '\0';
}

static void novo_tmp(CG *cg, char *out) { sprintf(out, "%%t.%d", ++cg->tmp); }

// Emite uma instrucao. Se o bloco ja terminou (ex: depois de um respost), abre um
// bloco "morto" sem predecessores so para a IR continuar valida.
static void emitir(CG *cg, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void emitir(CG *cg, const char *fmt, ...) {
    if (cg->terminado) {
        sprintf(cg->bloco, "inalcancavel.%d", ++cg->rotulo);
        buf_printf(&cg->corpo, "%s:\n", cg->bloco);
        cg->terminado = 0;
    }
    char linha[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(linha, sizeof(linha), fmt, ap);
    va_end(ap);
    buf_printf(&cg->corpo, "  %s\n", linha);
}

// Emite um terminador (br, ret, unreachable): o bloco acaba aqui
static void terminar(CG *cg, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void terminar(CG *cg, const char *fmt, ...) {
    if (cg->terminado) return;                  // codigo morto: nada a fazer
    char linha[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(linha, sizeof(linha), fmt, ap);
    va_end(ap);
    buf_printf(&cg->corpo, "  %s\n", linha);
    cg->terminado = 1;
}

// Abre um novo bloco basico. Se o anterior nao terminou, liga com um br (fall-through).
static void rotulo(CG *cg, const char *nome) {
    if (!cg->terminado) buf_printf(&cg->corpo, "  br label %%%s\n", nome);
    buf_printf(&cg->corpo, "%s:\n", nome);
    strncpy(cg->bloco, nome, sizeof(cg->bloco) - 1);
    cg->terminado = 0;
}

static void comentario_linha(CG *cg, int linha) {
    if (!cg->terminado) buf_printf(&cg->corpo, "  ; linha %d\n", linha);
}

/* ---------- Escopos (espelham exatamente os escopos do semantico) ---------- */
static void esc_entrar(CG *cg) { EscCG *e = calloc(1, sizeof(EscCG)); e->pai = cg->esc; cg->esc = e; }
static void esc_sair(CG *cg) {
    EscCG *e = cg->esc; cg->esc = e->pai;
    VarCG *v = e->vars; while (v) { VarCG *t = v; v = v->prox; free(t); }
    free(e);
}
static VarCG *esc_buscar(CG *cg, const char *nome) {
    for (EscCG *e = cg->esc; e; e = e->pai)
        for (VarCG *v = e->vars; v; v = v->prox)
            if (strcmp(v->nome, nome) == 0) return v;
    return NULL;
}
static VarCG *esc_inserir(CG *cg, const char *nome, const char *ptr, Tipo t, int is_vetor, int tam, const char *reg) {
    VarCG *v = calloc(1, sizeof(VarCG));
    strncpy(v->nome, nome, sizeof(v->nome) - 1);
    strncpy(v->ptr, ptr, sizeof(v->ptr) - 1);
    v->tipo = t; v->is_vetor = is_vetor; v->tam = tam;
    if (reg) strncpy(v->nome_reg, reg, sizeof(v->nome_reg) - 1);
    v->prox = cg->esc->vars; cg->esc->vars = v;
    return v;
}

// Nome unico por funcao para o alloca: x.addr, x.addr.1, x.addr.2 (sombreamento)
static void nome_alloca(CG *cg, const char *nome, char *out) {
    char base[80];
    sanitizar(nome, base, sizeof(base));
    for (NomeUsado *u = cg->usados; u; u = u->prox) {
        if (strcmp(u->base, base) == 0) { sprintf(out, "%%%s.addr.%d", base, u->vezes++); return; }
    }
    NomeUsado *u = calloc(1, sizeof(NomeUsado));
    strcpy(u->base, base); u->vezes = 1; u->prox = cg->usados; cg->usados = u;
    sprintf(out, "%%%s.addr", base);
}

// Tipo LLVM da VARIAVEL inteira (vetor -> [N x T], registro -> %struct.R)
static void tipo_variavel(CG *cg, Tipo t, int is_vetor, int tam, const char *reg, char *out) {
    const char *base = (t == TIPO_REGISTRO) ? NULL : lt(cg, t);
    char elem[128];
    if (base) strcpy(elem, base); else { char s[96]; sanitizar(reg, s, sizeof(s)); sprintf(elem, "%%struct.%s", s); }
    if (is_vetor) sprintf(out, "[%d x %s]", tam, elem); else strcpy(out, elem);
}

/* ---------- Constantes e conversoes ---------- */

// Constante real na IR: sempre em hexadecimal (formato exato; "0.1" decimal nao e aceito para float)
static void const_real(double v, int como_float, char *out) {
    if (como_float) v = (double)(float)v;               // float: o valor tem que ser representavel em 32 bits
    uint64_t bits; memcpy(&bits, &v, sizeof(bits));
    sprintf(out, "0x%016llX", (unsigned long long)bits);
}

typedef struct { char r[128]; Tipo t; } Val;           // um valor ja calculado: nome na IR + tipo

// Converte implicitamente (so para cima: inteira -> flut -> duplocarpado), gerando sitofp/fpext
static Val converter(CG *cg, Val v, Tipo para) {
    if (v.t == para || !eh_numerico(v.t) || !eh_numerico(para)) return v;
    const char *de = lt(cg, v.t), *pt = lt(cg, para);
    Val r; r.t = para;
    if (strcmp(de, pt) == 0) { strcpy(r.r, v.r); return r; }   // duplocarpado rebaixado no Uno: mesmo tipo
    novo_tmp(cg, r.r);
    if (v.t == TIPO_INT) emitir(cg, "%s = sitofp i32 %s to %s", r.r, v.r, pt);
    else if (rank_numerico(para) > rank_numerico(v.t)) emitir(cg, "%s = fpext %s %s to %s", r.r, de, v.r, pt);
    else emitir(cg, "%s = fptrunc %s %s to %s", r.r, de, v.r, pt);
    return r;
}

// Verificacao em tempo de execucao: se 'ok' (i1) for falso, chama zl_erro_execucao e para
static void checar(CG *cg, const char *ok, int codigo, int linha, const char *nome) {
    int n = ++cg->rotulo;
    char bom[64], ruim[64];
    sprintf(bom, "%s.ok.%d", nome, n);
    sprintf(ruim, "%s.erro.%d", nome, n);
    terminar(cg, "br i1 %s, label %%%s, label %%%s", ok, bom, ruim);
    rotulo(cg, ruim);
    emitir(cg, "call void @zl_erro_execucao(i32 %d, i32 %d)", codigo, linha);
    terminar(cg, "unreachable");
    rotulo(cg, bom);
}

static int constante_int(Node *no, long *v) {
    if (no && no->kind == ND_NUM && no->tipo_resultado == TIPO_INT) { *v = (long)no->num_valor; return 1; }
    return 0;
}

/* ---------- Textos ---------- */
static void texto_global(CG *cg, const char *s, char *out) {
    int n = cg->n_textos++;
    size_t len = strlen(s);
    buf_printf(&cg->textos, "@.str.%d = private unnamed_addr constant [%zu x i8] c\"", n, len + 1);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 32 && c < 127 && c != '"' && c != '\\') buf_printf(&cg->textos, "%c", c);
        else buf_printf(&cg->textos, "\\%02X", c);       // bytes especiais em hexadecimal
    }
    buf_printf(&cg->textos, "\\00\"\n");
    sprintf(out, "@.str.%d", n);
}

/* ---------- Funcoes do usuario ---------- */
static Node *achar_funcao(CG *cg, const char *nome) {
    for (Node *f = cg->raiz->lista; f; f = f->prox)
        if (f->kind == ND_FUNCAO && strcmp(f->nome, nome) == 0) return f;
    return NULL;
}
static void nome_funcao(const char *nome, char *out) { char s[96]; sanitizar(nome, s, sizeof(s)); sprintf(out, "@mac_%s", s); }

static Val gerar_expr(CG *cg, Node *no);
static void gerar_comandos(CG *cg, Node *lista);

// Endereco de um elemento de vetor (com verificacao de limites quando nao e constante)
static void endereco_elemento(CG *cg, VarCG *v, Node *idx, int linha, char *out) {
    Val i = gerar_expr(cg, idx);
    long c;
    if (!constante_int(idx, &c)) {
        // icmp ult (sem sinal) pega de uma vez indice >= N e indice negativo
        char ok[32]; novo_tmp(cg, ok);
        emitir(cg, "%s = icmp ult i32 %s, %d", ok, i.r, v->tam);
        checar(cg, ok, ZL_ERRO_INDICE, linha, "indice");
    }   // constante: o semantico ja provou que esta dentro dos limites -> sem verificacao
    char tv[160]; tipo_variavel(cg, v->tipo, 1, v->tam, v->nome_reg, tv);
    novo_tmp(cg, out);
    emitir(cg, "%s = getelementptr inbounds %s, ptr %s, i32 0, i32 %s", out, tv, v->ptr, i.r);
}

static void endereco_campo(CG *cg, VarCG *v, int indice, char *out) {
    char tv[160]; tipo_variavel(cg, TIPO_REGISTRO, 0, 0, v->nome_reg, tv);
    novo_tmp(cg, out);
    emitir(cg, "%s = getelementptr inbounds %s, ptr %s, i32 0, i32 %d", out, tv, v->ptr, indice);
}

/* ---------- Nucleos vetoriais (DSL) ---------- */
static VarCG *vetor_do_no(CG *cg, Node *no) {
    if (no && no->kind == ND_VAR) { VarCG *v = esc_buscar(cg, no->nome); if (v && v->is_vetor) return v; }
    return NULL;
}

static void registrar_nucleo(CG *cg, Nucleo *k, const char *op_nome, const char *ret, const char *params) {
    k->id = ++cg->n_nucleos;
    snprintf(k->nome, sizeof(k->nome), "dsl.%s.%d", op_nome, k->id);
    mlir_nucleo(cg->mlir, k, cg->rd);
    buf_printf(&cg->decl_nucleos, "declare %s @%s(%s) #0\n", ret, k->nome, params);
}

// c receba a op b;  -> uma chamada ao nucleo MLIR que faz o laco inteiro
static void gerar_atrib_vetorial(CG *cg, Node *no) {
    VarCG *dest = esc_buscar(cg, no->nome);
    Node *v = no->filho1;
    VarCG *va = vetor_do_no(cg, v->filho1), *vb = vetor_do_no(cg, v->filho2);
    Nucleo k; memset(&k, 0, sizeof(k));
    k.tipo = NUC_ELEMENTO;
    k.op = v->nome[0];
    k.n = dest->tam;
    k.esq_vetor = va != NULL; k.dir_vetor = vb != NULL;
    k.tres = v->tipo_resultado;
    k.tdest = dest->tipo;
    char arg_a[160], arg_b[160], params[160];
    if (va) { k.te = va->tipo; sprintf(arg_a, "ptr %s", va->ptr); }
    else { Val e = converter(cg, gerar_expr(cg, v->filho1), k.tres); k.te = k.tres; sprintf(arg_a, "%s %s", lt(cg, k.tres), e.r); }
    if (vb) { k.td = vb->tipo; sprintf(arg_b, "ptr %s", vb->ptr); }
    else { Val e = converter(cg, gerar_expr(cg, v->filho2), k.tres); k.td = k.tres; sprintf(arg_b, "%s %s", lt(cg, k.tres), e.r); }
    sprintf(params, "%s, %s, ptr", va ? "ptr" : lt(cg, k.tres), vb ? "ptr" : lt(cg, k.tres));
    const char *nomeop = k.op == '+' ? "soma" : k.op == '-' ? "sub" : "mul";
    registrar_nucleo(cg, &k, nomeop, "void", params);
    no->id_nucleo = k.id;
    emitir(cg, "call void @%s(%s, %s, ptr %s)   ; operacao vetorial (nucleo MLIR)", k.nome, arg_a, arg_b, dest->ptr);
}

// somavet(v), maxvet(v), prodesc(a, b)
static Val gerar_reducao(CG *cg, Node *no, Embutida bi) {
    Nucleo k; memset(&k, 0, sizeof(k));
    VarCG *a = vetor_do_no(cg, no->lista);
    k.n = a->tam; k.te = a->tipo; k.tres = no->tipo_resultado;
    Val r; r.t = k.tres;
    novo_tmp(cg, r.r);
    if (bi == BI_PRODESC) {
        VarCG *b = vetor_do_no(cg, no->lista->prox);
        k.tipo = NUC_PRODESC; k.td = b->tipo;
        registrar_nucleo(cg, &k, "prodesc", lt(cg, k.tres), "ptr, ptr");
        emitir(cg, "%s = call %s @%s(ptr %s, ptr %s)   ; produto escalar (nucleo MLIR)", r.r, lt(cg, k.tres), k.nome, a->ptr, b->ptr);
    } else {
        k.tipo = (bi == BI_SOMAVET) ? NUC_SOMA : NUC_MAXIMO;
        k.tres = k.te;
        registrar_nucleo(cg, &k, bi == BI_SOMAVET ? "somavet" : "maxvet", lt(cg, k.te), "ptr");
        emitir(cg, "%s = call %s @%s(ptr %s)   ; reducao (nucleo MLIR)", r.r, lt(cg, k.te), k.nome, a->ptr);
    }
    return r;
}

/* ---------- Expressoes ---------- */

static Val gerar_binop(CG *cg, Node *no) {
    Val a = gerar_expr(cg, no->filho1);
    Val b = gerar_expr(cg, no->filho2);
    Tipo t = no->tipo_resultado;
    Val r; r.t = t;
    const char *op = no->nome;

    if (strcmp(op, "//") == 0) {                        // divisao INTEIRA (resultado sempre inteira)
        if (a.t == TIPO_INT && b.t == TIPO_INT) {
            long c;
            int constante = constante_int(no->filho2, &c);
            if (!constante) {                            // divisor desconhecido: verifica zero em execucao
                char ok[32]; novo_tmp(cg, ok);
                emitir(cg, "%s = icmp ne i32 %s, 0", ok, b.r);
                checar(cg, ok, ZL_ERRO_DIVISAO_ZERO, no->linha, "divisao");
                // INT_MIN // -1 estoura (e trava o processador x86): trata -1 a parte, como 0 - a
                char m1[32], seguro[32], q[32], neg[32];
                novo_tmp(cg, m1); novo_tmp(cg, seguro); novo_tmp(cg, q); novo_tmp(cg, neg);
                emitir(cg, "%s = icmp eq i32 %s, -1", m1, b.r);
                emitir(cg, "%s = select i1 %s, i32 1, i32 %s", seguro, m1, b.r);
                emitir(cg, "%s = sdiv i32 %s, %s", q, a.r, seguro);
                emitir(cg, "%s = sub i32 0, %s", neg, a.r);
                novo_tmp(cg, r.r);
                emitir(cg, "%s = select i1 %s, i32 %s, i32 %s", r.r, m1, neg, q);
            } else if (c == -1) {
                novo_tmp(cg, r.r);
                emitir(cg, "%s = sub i32 0, %s", r.r, a.r);
            } else {                                     // constante != 0 e != -1: sdiv direto
                novo_tmp(cg, r.r);
                emitir(cg, "%s = sdiv i32 %s, %s", r.r, a.r, b.r);
            }
            return r;
        }
        // com real: divide em ponto flutuante e trunca para inteira (fptosi)
        Tipo w = tipo_mais_largo(a.t, b.t);
        a = converter(cg, a, w); b = converter(cg, b, w);
        if (!(no->filho2->kind == ND_NUM)) {
            char ok[32]; novo_tmp(cg, ok);
            emitir(cg, "%s = fcmp une %s %s, 0.0", ok, lt(cg, w), b.r);
            checar(cg, ok, ZL_ERRO_DIVISAO_ZERO, no->linha, "divisao");
        }
        char q[32]; novo_tmp(cg, q);
        emitir(cg, "%s = fdiv %s %s, %s", q, lt(cg, w), a.r, b.r);
        novo_tmp(cg, r.r);
        emitir(cg, "%s = fptosi %s %s to i32", r.r, lt(cg, w), q);
        return r;
    }

    if (strcmp(op, "^") == 0) {                         // potencia
        if (t == TIPO_INT) {                            // inteira ^ inteira: funcao do runtime (exata)
            novo_tmp(cg, r.r);
            emitir(cg, "%s = call i32 @zl_pot_inteira(i32 %s, i32 %s)", r.r, a.r, b.r);
            return r;
        }
        a = converter(cg, a, t); b = converter(cg, b, t);
        novo_tmp(cg, r.r);
        const char *sufixo = strcmp(lt(cg, t), "float") == 0 ? "f32" : "f64";
        emitir(cg, "%s = call %s @llvm.pow.%s(%s %s, %s %s)", r.r, lt(cg, t), sufixo, lt(cg, t), a.r, lt(cg, t), b.r);
        return r;
    }

    // + - * /  : converte os dois lados para o tipo do resultado e opera
    a = converter(cg, a, t); b = converter(cg, b, t);
    const char *ins;
    if (eh_real(t)) ins = (op[0] == '+') ? "fadd" : (op[0] == '-') ? "fsub" : (op[0] == '*') ? "fmul" : "fdiv";
    else ins = (op[0] == '+') ? "add" : (op[0] == '-') ? "sub" : "mul";   // inteira: complemento de dois (estouro da a volta)
    novo_tmp(cg, r.r);
    emitir(cg, "%s = %s %s %s, %s", r.r, ins, lt(cg, t), a.r, b.r);
    return r;
}

static Val gerar_chamada(CG *cg, Node *no) {
    Val r; r.t = no->tipo_resultado; r.r[0] = '\0';
    Embutida bi = embutida_por_nome(no->nome);
    switch (bi) {
        case BI_LEINTEIRA: novo_tmp(cg, r.r); emitir(cg, "%s = call i32 @zl_le_int()", r.r); return r;
        case BI_LEFLUT:    novo_tmp(cg, r.r); emitir(cg, "%s = call float @zl_le_flut()", r.r); return r;
        case BI_LEDUPLO:   novo_tmp(cg, r.r); emitir(cg, "%s = call %s @zl_le_duplo()", r.r, lt(cg, TIPO_DOUBLE)); return r;
        case BI_PINO_LE: {
            Val p = gerar_expr(cg, no->lista);
            novo_tmp(cg, r.r); emitir(cg, "%s = call i32 @zl_pino_le(i32 %s)", r.r, p.r); return r;
        }
        case BI_SOMAVET: case BI_MAXVET: case BI_PRODESC:
            return gerar_reducao(cg, no, bi);
        default: break;
    }
    // funcao do usuario: avalia argumentos da esquerda para a direita, convertendo para o tipo do parametro
    Node *f = achar_funcao(cg, no->nome);
    char args[2048] = "";
    Node *param = f ? f->filho1 : NULL;
    for (Node *a = no->lista; a; a = a->prox) {
        Val v = gerar_expr(cg, a);
        if (param) { v = converter(cg, v, param->tipo_declarado); param = param->prox; }
        size_t u = strlen(args);
        snprintf(args + u, sizeof(args) - u, "%s%s %s", u ? ", " : "", lt(cg, v.t), v.r);
    }
    char nf[128]; nome_funcao(no->nome, nf);
    Tipo ret = f ? f->tipo_declarado : TIPO_VOID;
    r.t = ret;
    if (ret == TIPO_VOID) emitir(cg, "call void %s(%s)", nf, args);
    else { novo_tmp(cg, r.r); emitir(cg, "%s = call %s %s(%s)", r.r, lt(cg, ret), nf, args); }
    return r;
}

static Val gerar_expr(CG *cg, Node *no) {
    Val r; r.t = no->tipo_resultado; r.r[0] = '\0';
    switch (no->kind) {
        case ND_NUM:
            if (r.t == TIPO_INT) sprintf(r.r, "%ld", (long)no->num_valor);
            else const_real(no->num_valor, strcmp(lt(cg, r.t), "float") == 0, r.r);
            return r;
        case ND_BOOL_LIT: strcpy(r.r, no->num_valor != 0 ? "true" : "false"); return r;
        case ND_STRING_LIT: texto_global(cg, no->str_valor, r.r); return r;
        case ND_VAR: {
            VarCG *v = esc_buscar(cg, no->nome);
            novo_tmp(cg, r.r);
            emitir(cg, "%s = load %s, ptr %s", r.r, lt(cg, v->tipo), v->ptr);
            return r;
        }
        case ND_ACESSO_VETOR: {
            VarCG *v = esc_buscar(cg, no->nome);
            char p[32]; endereco_elemento(cg, v, no->filho1, no->linha, p);
            novo_tmp(cg, r.r);
            emitir(cg, "%s = load %s, ptr %s", r.r, lt(cg, v->tipo), p);
            return r;
        }
        case ND_CAMPO_EXPR: {
            VarCG *v = esc_buscar(cg, no->nome);
            char p[32]; endereco_campo(cg, v, no->aux, p);
            novo_tmp(cg, r.r);
            emitir(cg, "%s = load %s, ptr %s   ; %s.%s", r.r, lt(cg, r.t), p, no->nome, no->campo);
            return r;
        }
        case ND_RAIZ_EXPR: {
            Val a = converter(cg, gerar_expr(cg, no->filho1), TIPO_DOUBLE);
            const char *t = lt(cg, TIPO_DOUBLE);
            novo_tmp(cg, r.r);
            emitir(cg, "%s = call %s @llvm.sqrt.%s(%s %s)", r.r, t, strcmp(t, "float") == 0 ? "f32" : "f64", t, a.r);
            return r;
        }
        case ND_BINOP: return gerar_binop(cg, no);
        case ND_CHAMADA_EXPR: return gerar_chamada(cg, no);
        default: strcpy(r.r, "0"); return r;
    }
}

/* ---------- Condicoes (sempre produzem i1) ---------- */
static void gerar_cond(CG *cg, Node *no, char *out) {
    switch (no->kind) {
        case ND_COMPARACAO: {
            Val a = gerar_expr(cg, no->filho1), b = gerar_expr(cg, no->filho2);
            const char *c = no->nome;
            novo_tmp(cg, out);
            if (a.t == TIPO_BOOL) {                     // bool so com == e ! (semantico garante)
                emitir(cg, "%s = icmp %s i1 %s, %s", out, c[0] == '=' ? "eq" : "ne", a.r, b.r);
                return;
            }
            Tipo w = tipo_mais_largo(a.t, b.t);         // inteira < flut: compara como flut
            a = converter(cg, a, w); b = converter(cg, b, w);
            if (eh_real(w)) {                           // ordenado (o*): falso se algum lado for NaN; "!" e une
                const char *p = c[0] == '<' ? "olt" : c[0] == '>' ? "ogt" : c[0] == '=' ? "oeq" : "une";
                emitir(cg, "%s = fcmp %s %s %s, %s", out, p, lt(cg, w), a.r, b.r);
            } else {
                const char *p = c[0] == '<' ? "slt" : c[0] == '>' ? "sgt" : c[0] == '=' ? "eq" : "ne";
                emitir(cg, "%s = icmp %s i32 %s, %s", out, p, a.r, b.r);
            }
            return;
        }
        case ND_LOGICO_NOT: {
            char a[32]; gerar_cond(cg, no->filho1, a);
            novo_tmp(cg, out);
            emitir(cg, "%s = xor i1 %s, true", out, a);
            return;
        }
        case ND_LOGICO_BIN: {
            if (strcmp(no->nome, "XOR") == 0) {         // XOR: os dois lados sempre avaliados
                char a[32], b[32];
                gerar_cond(cg, no->filho1, a); gerar_cond(cg, no->filho2, b);
                novo_tmp(cg, out);
                emitir(cg, "%s = xor i1 %s, %s", out, a, b);
                return;
            }
            // E / OU com CURTO-CIRCUITO: o lado direito so e avaliado se precisar.
            // O resultado se junta num phi (que existe mesmo ANTES do mem2reg).
            int e = (strcmp(no->nome, "E") == 0);
            int n = ++cg->rotulo;
            char dir[64], fim[64], a[32], b[32], bloco_esq[64], bloco_dir[64];
            sprintf(dir, "%s.dir.%d", e ? "e" : "ou", n);
            sprintf(fim, "%s.fim.%d", e ? "e" : "ou", n);
            gerar_cond(cg, no->filho1, a);
            strcpy(bloco_esq, cg->bloco);
            if (e) terminar(cg, "br i1 %s, label %%%s, label %%%s", a, dir, fim);   // E: falso a esquerda -> pula
            else   terminar(cg, "br i1 %s, label %%%s, label %%%s", a, fim, dir);   // OU: verdadeiro -> pula
            rotulo(cg, dir);
            gerar_cond(cg, no->filho2, b);
            strcpy(bloco_dir, cg->bloco);
            rotulo(cg, fim);
            novo_tmp(cg, out);
            emitir(cg, "%s = phi i1 [ %s, %%%s ], [ %s, %%%s ]", out, e ? "false" : "true", bloco_esq, b, bloco_dir);
            return;
        }
        default: strcpy(out, "false"); return;
    }
}

/* ---------- Comandos ---------- */

static void declarar_local(CG *cg, Node *no) {
    char ptr[96], tv[160];
    nome_alloca(cg, no->nome, ptr);
    int is_vetor = (no->kind == ND_DECL_VETOR);
    int tam = is_vetor ? (int)no->num_valor : 0;
    Tipo t = no->tipo_declarado;
    const char *reg = (no->kind == ND_DECL_REG) ? no->nome_reg : "";
    tipo_variavel(cg, t, is_vetor, tam, reg, tv);
    buf_printf(&cg->entrada, "  %s = alloca %s   ; %s (linha %d)\n", ptr, tv, no->nome, no->linha);
    esc_inserir(cg, no->nome, ptr, t, is_vetor, tam, reg);
    // Toda variavel comeca zerada (0, 0.0, FAL, texto vazio) no ponto da declaracao.
    if (is_vetor || t == TIPO_REGISTRO) {
        // tamanho em bytes sem depender do alvo: "sizeof" via getelementptr sobre ptr null
        emitir(cg, "call void @llvm.memset.p0.i32(ptr %s, i8 0, i32 ptrtoint (ptr getelementptr (%s, ptr null, i32 1) to i32), i1 false)",
               ptr, tv);
    } else {
        const char *zero = (t == TIPO_BOOL) ? "false" : (t == TIPO_STRING) ? "null" : eh_real(t) ? "0.0" : "0";
        emitir(cg, "store %s %s, ptr %s", lt(cg, t), zero, ptr);
    }
}

static void imprimir(CG *cg, Val v) {
    switch (v.t) {
        case TIPO_INT: emitir(cg, "call void @zl_imprime_int(i32 %s)", v.r); break;
        case TIPO_FLOAT: emitir(cg, "call void @zl_imprime_flut(float %s)", v.r); break;
        case TIPO_DOUBLE: emitir(cg, "call void @zl_imprime_duplo(%s %s)", lt(cg, TIPO_DOUBLE), v.r); break;
        case TIPO_BOOL: {
            char z[32]; novo_tmp(cg, z);
            emitir(cg, "%s = zext i1 %s to i32", z, v.r);
            emitir(cg, "call void @zl_imprime_bool(i32 %s)", z);
            break;
        }
        case TIPO_STRING: emitir(cg, "call void @zl_imprime_texto(ptr %s)", v.r); break;
        default: break;
    }
}

static void gerar_comando(CG *cg, Node *no);

static void gerar_bloco(CG *cg, Node *lista) {
    esc_entrar(cg);
    gerar_comandos(cg, lista);
    esc_sair(cg);
}

// cond / casocontrario cond / casocontrario: blocos entao, senao e fim
static void gerar_cond_cmd(CG *cg, Node *no) {
    int n = ++cg->rotulo;
    char entao[64], senao[64], fim[64], c[32];
    sprintf(entao, "cond.entao.%d", n);
    sprintf(senao, "cond.senao.%d", n);
    sprintf(fim, "cond.fim.%d", n);
    gerar_cond(cg, no->filho1, c);
    terminar(cg, "br i1 %s, label %%%s, label %%%s", c, entao, no->filho2 ? senao : fim);
    rotulo(cg, entao);
    gerar_bloco(cg, no->lista);
    terminar(cg, "br label %%%s", fim);
    if (no->filho2) {
        rotulo(cg, senao);
        if (no->filho2->filho1) gerar_cond_cmd(cg, no->filho2);     // casocontrario cond (senao se)
        else gerar_bloco(cg, no->filho2->lista);                    // casocontrario edai (senao final)
        terminar(cg, "br label %%%s", fim);
    }
    rotulo(cg, fim);
}

static void gerar_comando(CG *cg, Node *no) {
    comentario_linha(cg, no->linha);
    switch (no->kind) {
        case ND_DECL_VAR: case ND_DECL_VETOR: case ND_DECL_REG:
            declarar_local(cg, no);
            break;

        case ND_ATRIB: {
            if (no->eh_vetorial) { gerar_atrib_vetorial(cg, no); break; }
            VarCG *v = esc_buscar(cg, no->nome);
            Val val = converter(cg, gerar_expr(cg, no->filho1), v->tipo);
            emitir(cg, "store %s %s, ptr %s", lt(cg, v->tipo), val.r, v->ptr);
            break;
        }
        case ND_ATRIB_VETOR: {
            VarCG *v = esc_buscar(cg, no->nome);
            char p[32]; endereco_elemento(cg, v, no->filho2, no->linha, p);   // indice primeiro, depois o valor
            Val val = converter(cg, gerar_expr(cg, no->filho1), v->tipo);
            emitir(cg, "store %s %s, ptr %s", lt(cg, v->tipo), val.r, p);
            break;
        }
        case ND_ATRIB_CAMPO: {
            VarCG *v = esc_buscar(cg, no->nome);
            DefRegistro *r = buscar_registro(cg->sem->registros, v->nome_reg);
            Tipo tc = r->campo_tipo[no->aux];
            char p[32]; endereco_campo(cg, v, no->aux, p);
            Val val = converter(cg, gerar_expr(cg, no->filho1), tc);
            emitir(cg, "store %s %s, ptr %s   ; %s.%s", lt(cg, tc), val.r, p, no->nome, no->campo);
            break;
        }

        case ND_COND: gerar_cond_cmd(cg, no); break;

        case ND_REPETE: {                               // for: o teste fica no cabecalho do laco
            int n = ++cg->rotulo;
            char tst[64], corpo[64], inc[64], fim[64], c[32];
            sprintf(tst, "repete.cond.%d", n); sprintf(corpo, "repete.corpo.%d", n);
            sprintf(inc, "repete.inc.%d", n); sprintf(fim, "repete.fim.%d", n);
            esc_entrar(cg);
            gerar_comando(cg, no->filho1);              // inicializacao
            rotulo(cg, tst);
            gerar_cond(cg, no->filho2, c);
            terminar(cg, "br i1 %s, label %%%s, label %%%s", c, corpo, fim);
            rotulo(cg, corpo);
            gerar_comandos(cg, no->lista);
            rotulo(cg, inc);
            gerar_comando(cg, no->filho3);              // incremento
            terminar(cg, "br label %%%s", tst);
            rotulo(cg, fim);
            esc_sair(cg);
            break;
        }
        case ND_DURANTE: {                              // while
            int n = ++cg->rotulo;
            char tst[64], corpo[64], fim[64], c[32];
            sprintf(tst, "durante.cond.%d", n); sprintf(corpo, "durante.corpo.%d", n); sprintf(fim, "durante.fim.%d", n);
            rotulo(cg, tst);
            gerar_cond(cg, no->filho1, c);
            terminar(cg, "br i1 %s, label %%%s, label %%%s", c, corpo, fim);
            rotulo(cg, corpo);
            gerar_bloco(cg, no->lista);
            terminar(cg, "br label %%%s", tst);
            rotulo(cg, fim);
            break;
        }

        case ND_RESPOST:
            if (cg->ret_atual == TIPO_VOID) terminar(cg, "ret void");
            else {
                Val v = converter(cg, gerar_expr(cg, no->filho1), cg->ret_atual);
                terminar(cg, "ret %s %s", lt(cg, cg->ret_atual), v.r);
            }
            break;

        case ND_CHAMADA_STMT: {
            Embutida bi = embutida_por_nome(no->nome);
            if (bi == BI_MOSTRAR || bi == BI_ESCREVE) {
                for (Node *a = no->lista; a; a = a->prox) imprimir(cg, gerar_expr(cg, a));
                if (bi == BI_MOSTRAR) emitir(cg, "call void @zl_imprime_nl()");
            } else if (bi == BI_NOVALINHA) {
                emitir(cg, "call void @zl_imprime_nl()");
            } else if (bi == BI_PINO_MODO || bi == BI_PINO_ESCREVE) {
                Val p = gerar_expr(cg, no->lista), v = gerar_expr(cg, no->lista->prox);
                emitir(cg, "call void @zl_%s(i32 %s, i32 %s)", bi == BI_PINO_MODO ? "pino_modo" : "pino_escreve", p.r, v.r);
            } else if (bi == BI_ESPERA) {
                Val ms = gerar_expr(cg, no->lista);
                emitir(cg, "call void @zl_espera(i32 %s)", ms.r);
            } else {                                    // usuario ou embutida com valor: o valor e descartado
                Node tmp = *no; tmp.kind = ND_CHAMADA_EXPR;
                tmp.tipo_resultado = (bi == BI_NENHUM) ? TIPO_INDEFINIDO : no->tipo_resultado;
                gerar_chamada(cg, &tmp);
            }
            break;
        }
        default: break;
    }
}

static void gerar_comandos(CG *cg, Node *lista) {
    for (Node *no = lista; no; no = no->prox) gerar_comando(cg, no);
}

/* ---------- Funcoes ---------- */

static void liberar_usados(CG *cg) {
    NomeUsado *u = cg->usados; while (u) { NomeUsado *t = u; u = u->prox; free(t); }
    cg->usados = NULL;
}

static void gerar_funcao(CG *cg, Node *f, Buf *saida) {
    buf_iniciar(&cg->entrada);
    buf_iniciar(&cg->corpo);
    cg->tmp = 0; cg->terminado = 0;
    strcpy(cg->bloco, "corpo");             // as instrucoes vao no bloco "corpo" (o "entry" so tem allocas)
    cg->ret_atual = f->tipo_declarado;
    esc_entrar(cg);

    char nf[128]; nome_funcao(f->nome, nf);
    buf_printf(saida, "; linha %d: %s %s\n", f->linha, f->num_valor ? "principal" : "outrafuncao", f->nome);
    buf_printf(saida, "define %s%s %s(", f->num_valor ? "internal " : "", lt(cg, f->tipo_declarado), nf);
    int primeiro = 1;
    for (Node *p = f->filho1; p; p = p->prox) {
        char s[96]; sanitizar(p->nome, s, sizeof(s));
        buf_printf(saida, "%s%s %%%s", primeiro ? "" : ", ", lt(cg, p->tipo_declarado), s);
        primeiro = 0;
        char ptr[96]; nome_alloca(cg, p->nome, ptr);    // parametro tambem vira variavel em memoria
        buf_printf(&cg->entrada, "  %s = alloca %s\n", ptr, lt(cg, p->tipo_declarado));
        buf_printf(&cg->entrada, "  store %s %%%s, ptr %s\n", lt(cg, p->tipo_declarado), s, ptr);
        esc_inserir(cg, p->nome, ptr, p->tipo_declarado, 0, 0, NULL);
    }
    buf_printf(saida, ") #0 {\n");

    gerar_comandos(cg, f->lista);

    if (!cg->terminado) {                           // chegou ao fim do corpo sem respost
        if (f->tipo_declarado == TIPO_VOID) terminar(cg, "ret void");
        else {
            emitir(cg, "call void @zl_erro_execucao(i32 %d, i32 %d)   ; fim da funcao sem respost", ZL_ERRO_SEM_RETORNO, f->linha);
            terminar(cg, "unreachable");
        }
    }
    buf_printf(saida, "entry:\n");
    buf_anexar(saida, &cg->entrada);
    buf_printf(saida, "  br label %%corpo\ncorpo:\n");
    buf_anexar(saida, &cg->corpo);
    buf_printf(saida, "}\n\n");

    esc_sair(cg);
    liberar_usados(cg);
    buf_liberar(&cg->entrada);
    buf_liberar(&cg->corpo);
}

// @zl_principal(): ponto de entrada chamado pelo runtime de cada plataforma.
// Le os parametros da principal da entrada padrao e devolve o codigo de saida (i32).
static void gerar_ponto_entrada(CG *cg, Node *princ, Buf *saida) {
    buf_printf(saida, "; ponto de entrada chamado pelo runtime (main/setup ficam em runtime/zl_runtime_*.c)\n");
    buf_printf(saida, "define i32 @zl_principal() #0 {\nentry:\n");
    char args[1024] = "";
    int i = 0;
    for (Node *p = princ->filho1; p; p = p->prox, i++) {
        const char *fn = p->tipo_declarado == TIPO_INT ? "zl_le_int" : p->tipo_declarado == TIPO_FLOAT ? "zl_le_flut" : "zl_le_duplo";
        buf_printf(saida, "  %%arg.%d = call %s @%s()   ; parametro '%s' lido da entrada\n", i, lt(cg, p->tipo_declarado), fn, p->nome);
        size_t u = strlen(args);
        snprintf(args + u, sizeof(args) - u, "%s%s %%arg.%d", u ? ", " : "", lt(cg, p->tipo_declarado), i);
    }
    Tipo t = princ->tipo_declarado;
    if (t == TIPO_VOID) {
        buf_printf(saida, "  call void @mac_principal(%s)\n  ret i32 0\n}\n\n", args);
        return;
    }
    buf_printf(saida, "  %%r = call %s @mac_principal(%s)\n", lt(cg, t), args);
    if (t == TIPO_INT) buf_printf(saida, "  ret i32 %%r\n}\n\n");
    else if (t == TIPO_BOOL) buf_printf(saida, "  %%c = zext i1 %%r to i32\n  ret i32 %%c\n}\n\n");
    else if (eh_numerico(t)) buf_printf(saida, "  %%c = fptosi %s %%r to i32\n  ret i32 %%c\n}\n\n", lt(cg, t));
    else buf_printf(saida, "  ret i32 0\n}\n\n");
}

int gerar_llvm(Node *raiz, const Semantico *sem, const char *fonte, Buf *ll, Buf *mlir) {
    CG cg; memset(&cg, 0, sizeof(cg));
    cg.topo = ll; cg.mlir = mlir; cg.raiz = raiz; cg.sem = sem;
    cg.rd = (sem->alvo == ALVO_UNO);
    buf_iniciar(&cg.textos);
    buf_iniciar(&cg.decl_nucleos);
    esc_entrar(&cg);                                // escopo global

    buf_printf(ll, "; Modulo gerado pelo compilador Macaronica a partir de '%s'\n", fonte);
    buf_printf(ll, "; IR portavel: sem target triple/datalayout (o alvo e escolhido no llc/clang)\n");
    if (cg.rd) buf_printf(ll, "; alvo uno: duplocarpado gerado como float (32 bits), ver docs/placa.md\n");
    buf_printf(ll, "\n");

    // Tipos registro: %struct.Nome = type { campos }
    for (const DefRegistro *r = sem->registros; r; r = r->proximo) {
        char s[96]; sanitizar(r->nome, s, sizeof(s));
        buf_printf(ll, "%%struct.%s = type { ", s);
        for (int i = 0; i < r->n_campos; i++) buf_printf(ll, "%s%s", i ? ", " : "", lt(&cg, r->campo_tipo[i]));
        buf_printf(ll, " }   ; registro %s\n", r->nome);
    }
    if (sem->registros) buf_printf(ll, "\n");

    // Variaveis globais (zeradas): @g_nome = global T zeroinitializer
    int tem_global = 0;
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind != ND_DECL_VAR && no->kind != ND_DECL_VETOR && no->kind != ND_DECL_REG) continue;
        int is_vetor = no->kind == ND_DECL_VETOR, tam = is_vetor ? (int)no->num_valor : 0;
        const char *reg = no->kind == ND_DECL_REG ? no->nome_reg : "";
        char tv[160], s[96], ptr[110];
        tipo_variavel(&cg, no->tipo_declarado, is_vetor, tam, reg, tv);
        sanitizar(no->nome, s, sizeof(s));
        sprintf(ptr, "@g_%s", s);
        buf_printf(ll, "%s = global %s zeroinitializer   ; linha %d: global %s\n", ptr, tv, no->linha, no->nome);
        esc_inserir(&cg, no->nome, ptr, no->tipo_declarado, is_vetor, tam, reg);
        tem_global = 1;
    }
    if (tem_global) buf_printf(ll, "\n");

    // Funcoes (principal vira @mac_principal interna; @zl_principal e o ponto de entrada)
    Buf funcoes; buf_iniciar(&funcoes);
    Node *princ = NULL;
    for (Node *no = raiz->lista; no; no = no->prox) {
        if (no->kind != ND_FUNCAO) continue;
        if (no->num_valor) princ = no;
        gerar_funcao(&cg, no, &funcoes);
    }
    if (princ) gerar_ponto_entrada(&cg, princ, &funcoes);
    buf_anexar(ll, &funcoes);
    buf_liberar(&funcoes);

    if (cg.n_textos) { buf_anexar(ll, &cg.textos); buf_printf(ll, "\n"); }

    // Biblioteca de execucao (runtime/zl_runtime.h) e intrinsecos usados
    const char *td = lt(&cg, TIPO_DOUBLE);
    buf_printf(ll, "; biblioteca de execucao (runtime/zl_runtime.h): so funcoes de assinatura fixa\n");
    buf_printf(ll, "declare void @zl_imprime_int(i32) #0\n");
    buf_printf(ll, "declare void @zl_imprime_flut(float) #0\n");
    buf_printf(ll, "declare void @zl_imprime_duplo(%s) #0\n", td);
    buf_printf(ll, "declare void @zl_imprime_bool(i32) #0\n");
    buf_printf(ll, "declare void @zl_imprime_texto(ptr) #0\n");
    buf_printf(ll, "declare void @zl_imprime_nl() #0\n");
    buf_printf(ll, "declare i32 @zl_le_int() #0\n");
    buf_printf(ll, "declare float @zl_le_flut() #0\n");
    buf_printf(ll, "declare %s @zl_le_duplo() #0\n", td);
    buf_printf(ll, "declare void @zl_erro_execucao(i32, i32) #1\n");
    buf_printf(ll, "declare i32 @zl_pot_inteira(i32, i32) #0\n");
    buf_printf(ll, "declare void @zl_pino_modo(i32, i32) #0\n");
    buf_printf(ll, "declare void @zl_pino_escreve(i32, i32) #0\n");
    buf_printf(ll, "declare i32 @zl_pino_le(i32) #0\n");
    buf_printf(ll, "declare void @zl_espera(i32) #0\n");
    buf_printf(ll, "declare double @llvm.sqrt.f64(double)\ndeclare float @llvm.sqrt.f32(float)\n");
    buf_printf(ll, "declare double @llvm.pow.f64(double, double)\ndeclare float @llvm.pow.f32(float, float)\n");
    buf_printf(ll, "declare void @llvm.memset.p0.i32(ptr, i8, i32, i1)\n");
    if (cg.n_nucleos) {
        buf_printf(ll, "\n; nucleos vetoriais da DSL: definidos no arquivo .mlir (ligados com llvm-link)\n");
        buf_anexar(ll, &cg.decl_nucleos);
    }
    buf_printf(ll, "\nattributes #0 = { nounwind }\nattributes #1 = { nounwind noreturn }\n");

    esc_sair(&cg);
    buf_liberar(&cg.textos);
    buf_liberar(&cg.decl_nucleos);
    return cg.n_nucleos;
}
