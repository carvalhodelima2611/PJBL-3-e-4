# Extensão Vetorial (DSL) — PjBL 4, Parte A

A extensão dá às operações sobre vetores uma sintaxe de primeira classe, no estilo da própria Macaronica: a atribuição continua sendo `receba` e as reduções usam a mesma forma de chamada `nome(args)` das outras funções (decisão 1.2 do PjBL 1). Ela opera sobre os vetores que já existem na linguagem (`inteira`, `flut` e `duplocarpado`).

## 1. Operações

| operação | sintaxe | semântica |
|---|---|---|
| elemento a elemento | `c receba a + b;` &nbsp; `c receba a - b;` &nbsp; `c receba a * b;` | `c{i} = a{i} op b{i}` para todo `i` |
| escalar × vetor | `c receba k * a;` &nbsp; `c receba a * k;` (também `+` e `-`) | `c{i} = k op a{i}` (ou `a{i} op k`); `k` é qualquer expressão escalar, avaliada **uma vez** |
| redução: soma | `somavet(a)` | soma de todos os elementos |
| redução: máximo | `maxvet(a)` | maior elemento |
| produto escalar | `prodesc(a, b)` | soma de `a{i} * b{i}` |

Exemplos (todos em `tests/dsl/`):
```
inteira vet{8} a, b, c;
flut vet{8} f;
c receba a + b;               /* elemento a elemento */
c receba (k + 1) * a;         /* escalar x vetor     */
f receba a * 0.5;             /* inteira * flut -> flut, guardado num vetor flut */
s receba somavet(a);          /* reducao             */
x receba prodesc(a, f);       /* produto escalar misto: resultado flut */
```

## 2. Especificação formal

**Tokens novos:** nenhum. A extensão só reaproveita tokens que já existiam (`receba`, `+`, `-`, `*`, identificadores, `(`, `)`, `,`). `somavet`, `maxvet` e `prodesc` são **funções embutidas** cadastradas na tabela de símbolos (como `mostrar`), e não palavras reservadas. Ainda assim o usuário não pode criar uma função com esses nomes ("nome reservado para função embutida").

**Produções.** A gramática do PjBL 1 já aceita essas formas: `atrib → ID receba expr ;` e `fator → ID ( args )`. O que muda é a **regra semântica**, aplicada quando o destino de um `receba` é um vetor:

```
atrib_vetorial  → IDvet receba operando_v op_v operando_v ;     (ao menos um operando é IDvet)
operando_v      → IDvet | expr_escalar
op_v            → + | - | *
reducao         → somavet ( IDvet ) | maxvet ( IDvet ) | prodesc ( IDvet , IDvet )
```

`IDvet` é um identificador declarado como vetor numérico. A decisão entre atribuição escalar e vetorial é **semântica**: o parser monta a mesma árvore (`ND_ATRIB` com um `ND_BINOP`), e o semântico marca o nó com `eh_vetorial = 1` quando o destino é vetor. Assim, a extensão não muda nenhuma regra sintática e não pode quebrar um programa válido do PjBL 1 (todos os testes antigos continuam passando no `make test`).

## 3. Regras semânticas

1. **Tamanhos:** todos os vetores de uma operação têm de ter o mesmo tamanho (destino incluído). Como todo tamanho de vetor em Macaronica é uma constante, isso é **sempre** verificado em compilação. Erro: `operandos de tamanhos diferentes (4 e 5) na operacao vetorial atribuida a 'c'` (`tests/dsl/invalidos/01_tamanhos_diferentes.mac`).
2. **Tipos:** só vetores numéricos (`bool vet{...}` ou `palavra vet{...}` dão erro). Valem as **mesmas regras de promoção do PjBL 3**, elemento a elemento: o resultado de `a op b` é o tipo mais largo dos elementos (e do escalar); o destino tem de aceitar esse tipo sem estreitamento. `c receba a + f;` com `c` inteira e `f` flut é erro (`tests/dsl/invalidos/02_tipos_incompativeis.mac`).
3. **Reduções:** `somavet`/`maxvet` devolvem o tipo do elemento; `prodesc(a, b)` devolve o tipo mais largo dos dois.
4. **Uma operação por atribuição:** `c receba a + b * d;` não é aceito. Cada operação vetorial vira um núcleo, e expressões compostas de vetores ficam para trabalho futuro (seriam uma fusão de núcleos). Fora de `receba` e das reduções, um vetor sem índice continua sendo erro.

## 4. Integração com o compilador: estratégia híbrida

Escolhemos a estratégia **híbrida** da Seção 5.2. O programa inteiro continua em LLVM IR (PjBL 3), e **cada operação vetorial vira um núcleo MLIR separado** (`src/codegen_mlir.c`). Na IR principal, ela aparece só como uma chamada:

```llvm
call void @dsl.soma.1(ptr @g_a, ptr @g_b, ptr @g_c)   ; operacao vetorial (nucleo MLIR)
```

e no `.mlir`, gerado ao lado do `.ll`:

```mlir
func.func @dsl.soma.1(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>) attributes {passthrough = ["nounwind"]} {
  affine.for %i = 0 to 8 {
    %x = affine.load %a[%i] : memref<8xi32>
    %y = affine.load %b[%i] : memref<8xi32>
    %r = arith.addi %x, %y : i32
    affine.store %r, %c[%i] : memref<8xi32>
  }
  return
}
```

`scripts/pipeline.sh` rebaixa o `.mlir` até LLVM IR, e o `llvm-link` junta os dois módulos. A ligação entre eles usa a convenção **bare pointer** (`convert-func-to-llvm=use-bare-ptr-memref-call-conv=1`): cada `memref` de tamanho fixo vira **um único `ptr`** no parâmetro, que é exatamente o endereço do vetor `[N x T]` na IR do PjBL 3. O escalar `k` é calculado na IR principal (já convertido para o tipo do resultado) e passado por valor.

**Por que híbrida:** (1) o PjBL 3 continua intacto e já testado; (2) cada núcleo é uma função pequena, que dá para mostrar inteira em cada nível do pipeline (relatório, Parte B); (3) o estudo de otimização (Parte C) pode trocar só o pipeline dos núcleos, sem mexer no resto do programa. O custo é uma chamada de função por operação, desprezível para vetores grandes.

## 5. Lacunas da Seção 5.5

| pergunta | decisão |
|---|---|
| Operações entre vetores cujo tamanho só é conhecido em execução | **Não existem em Macaronica**: o tamanho de todo vetor é uma constante na declaração (`vet{10}`). A verificação de tamanhos é sempre estática, e o gerador nunca precisa de verificação em execução para isso. |
| O destino pode ser um dos operandos (`a receba a + b;`)? | **Pode** (`tests/dsl/01_elemento_a_elemento.mac` testa `a receba a + a;`). É seguro porque `c{i}` só depende de `a{i}` e `b{i}`, do mesmo índice: não existe dependência entre iterações, mesmo com destino e operando iguais. Por isso também não atrapalha a vetorização nem a paralelização: cada iteração lê antes de escrever a mesma posição. (Mudaria se houvesse deslocamento, como `c{i} = a{i+1}`, que a DSL não oferece.) No MLIR, os dois `memref` apontam para a mesma memória; os passes affine que usamos não supõem que `memref`s diferentes sejam disjuntos, então continuam corretos. |
| Redução sobre vetor vazio ou de tamanho 1 | **Vetor vazio não existe**: `vet{0}` é erro semântico ("tamanho de vetor deve ser um número inteiro maior que zero"). Tamanho 1: `somavet` e `maxvet` devolvem o próprio elemento, e `prodesc` o produto dos dois elementos (testado em `tests/dsl/03_reducoes.mac`). `maxvet` começa do primeiro elemento (não de um "menos infinito"), e por isso não precisa de valor neutro. |
| Nas placas: o que muda nos núcleos num alvo sem SIMD? | Ver `docs/placa.md`: o mesmo núcleo, rebaixado no modo `base`, roda no AVR como laço escalar (`tests/placa/05_dsl_na_placa.mac`). |

## 6. Testes

`tests/dsl/` tem um programa por operação (elemento a elemento, escalar × vetor, reduções e produto escalar). Cada um compara o resultado do núcleo com a versão escalar escrita à mão na própria linguagem, e a diferença tem que ser zero. `tests/dsl/invalidos/` tem três programas que devem ser rejeitados: tamanhos diferentes, tipos incompatíveis, e vetor não numérico com produto escalar de tamanhos diferentes. O `make test` roda os testes válidos **três vezes**: núcleos sem otimização, vetorizados no MLIR e paralelizados com OpenMP. O resultado tem que ser o mesmo nos três.
