# Decisões de Projeto do Back-end (PjBL 3) — Macaronica → LLVM IR

Este documento resolve as lacunas da Seção 4.3 do enunciado do PjBL 3, registra as lacunas adicionais que o grupo encontrou e explica a análise do `mem2reg`/`phi` pedida na Seção 4.4.

## 0. Visão geral

```
programa.mac ──► lexer ──► parser ──► semântico ──► codegen_llvm.c ──► programa.ll ──┐
                                      (AST anotada)   └► codegen_mlir.c ──► programa.mlir ─► pipeline.sh ─► nucleos.ll
                                                                                         llvm-link ◄──┘
                                                       programa.ll (+ nucleos.ll) + runtime/zl_runtime_<plataforma>.c
                                                          ├─ clang ............................ executável do PC
                                                          ├─ lli .............................. execução JIT
                                                          ├─ llc -mtriple=avr + avr-gcc ....... Arduino Uno (.hex)
                                                          └─ clang --target=aarch64-linux-gnu . Raspberry Pi (ELF)
```

- **Como a IR é emitida:** texto, com `fprintf` para um buffer em memória (`Buf`, em `codegen_mlir.c`). Escolhemos texto em vez da API C do LLVM (`llvm-c/Core.h`) porque o compilador continua dependendo só da biblioteca padrão do C, e porque a IR textual é o que a arguição pede para ler. O arquivo só é escrito no final, então um programa inválido nunca deixa `.ll` para trás (o `main.c` até apaga uma saída antiga com o mesmo nome).
- **Separação das fases:** o gerador só recebe a AST **já anotada** pelo semântico (tipo de cada expressão em `tipo_resultado`, índice do campo em `aux`, operação vetorial em `eh_vetorial`). Ele não refaz nenhuma verificação de tipo, por isso nunca falha.
- **Interface:** `./macaronica entrada.mac -o saida.ll [--alvo=pc|uno|rpi]`. Sem `-o`, só valida (como no PjBL 1).

## 1. Lacunas da Seção 4.3

### 1.1 Largura dos tipos (independente da plataforma)

| Macaronica | LLVM IR | Observação |
|---|---|---|
| `inteira` | `i32` | **sempre 32 bits**, até no AVR de 8 bits (o `llc` gera as rotinas de 32 bits; ver `tests/placa/02_recursao.mac`, que calcula `2 ^ 30`) |
| `flut` | `float` | IEEE 754, 32 bits |
| `duplocarpado` | `double` | IEEE 754, 64 bits. **Exceção:** no Arduino Uno é gerado como `float` (ver 1.7) |
| `bool` | `i1` | na memória ocupa 1 byte; é estendido para `i32` (`zext`) só para imprimir |
| `palavra` | `ptr` | aponta para uma constante global `[N x i8]` terminada em zero |
| `vazio` | `void` | só como tipo de retorno |
| vetor `T vet{N} x` | `[N x T]` | acesso com `getelementptr inbounds [N x T], ptr %x, i32 0, i32 %i` |
| registro `R` | `%struct.R = type { ... }` | campo com `getelementptr inbounds %struct.R, ptr %r, i32 0, i32 <índice>` |

Seguimos as 6 regras da Seção 4.5: sem `target triple`/`datalayout`, tipos de largura fixa, nenhuma função variádica, endereços sempre por `getelementptr` com índices `i32`, tudo `nounwind`, e a principal vira `@zl_principal`. O tamanho em bytes de vetores e registros (usado no `memset` que zera as variáveis) também é calculado sem depender do alvo, com o idioma `ptrtoint (ptr getelementptr (T, ptr null, i32 1) to i32)`.

### 1.2 Promoção entre `inteira`, `flut` e `duplocarpado`; semântica de `/` e `//`

**Promoção.** Continua a regra do PjBL 1 (`decisoes.md` 2.5): a conversão implícita só "sobe" (`inteira → flut → duplocarpado`). Numa operação mista, os dois lados são convertidos para o tipo mais largo antes de operar. O gerador emite a conversão explicitamente:

| de → para | instrução |
|---|---|
| `inteira` → `flut`/`duplocarpado` | `sitofp i32 ... to float/double` |
| `flut` → `duplocarpado` | `fpext float ... to double` |
| estreitamento (`flut` → `inteira`, ...) | **erro semântico**; nunca é gerado (`fptrunc`/`fptosi` só aparecem em `//` e no código de saída da principal) |

Isso vale também para atribuições, argumentos de chamada, `respost` e comparações (`7 < 7.5` compara como `flut`).

**`/` e `//`.** Essa era a pergunta mais importante, porque o PjBL 1 deixava `inteira / inteira` com resultado `inteira`, o que tornava `/` e `//` iguais para inteiros. Decidimos que **cada operador tem um significado só, independente dos tipos**:

| expressão | tipo do resultado | IR gerada |
|---|---|---|
| `a / b` (quaisquer numéricos) | o mais largo entre os operandos e `flut` | `fdiv` (com `sitofp` antes, se precisar) |
| `a // b` (dois `inteira`) | `inteira` | `sdiv` (trunca em direção a zero, como C) |
| `a // b` (algum real) | `inteira` | `fdiv` e depois `fptosi` (trunca em direção a zero) |

Exemplo (`tests/execucao/10_divisao_real_e_inteira.mac`): `7 / 2` é `3.5000`, `7 // 2` é `3`, `-7 // 2` é `-3`, `7.9 // 2` é `3`. Consequência para o semântico: `inteira q; q receba 7 / 2;` agora é **erro** (estreitamento de `flut` para `inteira`). Ninguém perde nada, porque existe `//` justamente para isso.

**Justificativa:** a linguagem já tem um operador só para a divisão inteira. Se `/` também fizesse divisão inteira entre inteiros, o programador nunca saberia o que `a / b` faz sem olhar os tipos (a armadilha clássica do C). Com a nossa regra, quem lê o operador já sabe o resultado.

### 1.3 `^` e `raiz()`

| expressão | tradução |
|---|---|
| `inteira ^ inteira` | `call i32 @zl_pot_inteira(i32, i32)` (runtime): potência exata por quadrados sucessivos. Expoente negativo dá `0` (o valor `1/b^n` truncado), exceto para base `1` e `-1` |
| com algum real | os dois lados são convertidos para o tipo mais largo; `llvm.pow.f32` ou `llvm.pow.f64` |
| `raiz(x)` | `llvm.sqrt.f64`; o resultado é **sempre `duplocarpado`** (regra do PjBL 1), mesmo para argumento `inteira` |

Não usamos `llvm.powi` para `inteira ^ inteira` porque ele calcula em ponto flutuante e erraria para resultados acima de 2²⁴ (`2 ^ 30` precisa ser exato).

### 1.4 `E`, `OU`, `XOR` e `NAO`

- **`E` e `OU` usam curto-circuito.** O lado direito vira um bloco separado e só é avaliado se precisar. O resultado se junta num `phi`, que existe **antes mesmo do mem2reg**:
  ```llvm
  %t.9 = icmp slt i32 %t.8, 1000
  br i1 %t.9, label %e.dir.3, label %e.fim.3      ; E: se a esquerda é falsa, pula a direita
  e.dir.3:
    ...
  e.fim.3:
    %t.12 = phi i1 [ false, %durante.cond.2 ], [ %t.11, %e.dir.3 ]
  ```
  Justificativa: é o comportamento esperado por quem vem de C/Java, e permite proteger um acesso, como em `cond ( i < 3 E v{i} > 0 )` (ver `tests/execucao/14_bool_e_texto.mac`, onde o curto-circuito evita um erro de índice).
- **`XOR`** avalia os dois lados (não existe curto-circuito possível) e gera `xor i1 %a, %b`.
- **`NAO`** gera `xor i1 %c, true`.
- **Comparações** produzem `i1`. Inteiros usam `icmp slt/sgt/eq/ne`; reais usam `fcmp olt/ogt/oeq` (ordenadas: falsas se algum lado é NaN) e `fcmp une` para `!`, de modo que `x ! x` é verdadeiro quando `x` é NaN, como no IEEE 754.

### 1.5 Significado de `respost` dentro de `principal`

O valor do `respost` da principal **vira o código de saída do processo**. A principal do usuário é gerada como `@mac_principal` (interna), e o compilador gera também o ponto de entrada `@zl_principal`, que o runtime chama:

```llvm
define i32 @zl_principal() #0 {
entry:
  %r = call i32 @mac_principal()
  ret i32 %r
}
```

- principal `flut`/`duplocarpado`: o valor é truncado (`fptosi`); `bool`: `zext` (VER = 1); `vazio`: código 0.
- No PC, o `main` do runtime devolve `r & 0xFF` (o sistema operacional só guarda 8 bits). Na placa, que não tem processo, o runtime **imprime** `[codigo de saida: N]` na serial. Por isso a saída esperada dos testes termina com essa linha nos dois casos.
- **Parâmetros da principal** (permitidos pela especificação, `tipo principal(tipo nome, ...)`): o `@zl_principal` lê cada um da entrada padrão (`zl_le_int`, `zl_le_flut`, `zl_le_duplo`) antes de chamar a principal. Isso conversa com a decisão 2.8 do PjBL 1 (parâmetros como mecanismo de entrada). Por isso só são aceitos parâmetros numéricos na principal (erro semântico caso contrário).

### 1.6 Divisão por zero, estouro inteiro e acesso fora dos limites

Política: **verificar em tempo de execução só o que não dá para provar em compilação**, e abortar com mensagem e linha do fonte (`zl_erro_execucao(codigo, linha)`, que não retorna).

| situação | em compilação (semântico) | em execução (IR) |
|---|---|---|
| índice de vetor **constante** fora do vetor (`v{10}` em `vet{5}`) | **erro semântico** | nada gerado |
| índice constante dentro do vetor | provado: **nenhuma verificação** | só o `getelementptr` |
| índice calculado (`v{i}`) | — | `icmp ult i32 %i, N` (sem sinal: pega negativo e ≥ N de uma vez) + desvio para `zl_erro_execucao(2, linha)` |
| `//` por constante zero | **erro semântico** | — |
| `//` por constante ≠ 0 | provado: **nenhuma verificação** | só o `sdiv` |
| `//` por valor calculado | — | `icmp ne i32 %b, 0` + `zl_erro_execucao(3, linha)` |
| `INT_MIN // -1` | — | tratado à parte com `select` (dá `INT_MIN`): o `sdiv` do x86 **trava o processador** nesse caso |
| `/` (sempre real) por zero | — | sem verificação: segue o IEEE 754 (`inf` ou `nan`) |
| estouro em `+ - * ^` de `inteira` | — | **dá a volta** (complemento de dois): geramos `add`/`sub`/`mul` **sem** `nsw`, então o comportamento é definido |
| função não-`vazio` que chega ao fim sem `respost` | o semântico exige ao menos um `respost` | se ainda assim cair no fim (ex.: só um `cond` tem `respost`): `zl_erro_execucao(4, linha)` + `unreachable` |

Exemplo do que é gerado para `v{i} receba ...` (linha 8):
```llvm
  %t.3 = icmp ult i32 %t.2, 5
  br i1 %t.3, label %indice.ok.2, label %indice.erro.2
indice.erro.2:
  call void @zl_erro_execucao(i32 2, i32 8)
  unreachable
indice.ok.2:
  %t.4 = getelementptr inbounds [5 x i32], ptr %v.addr, i32 0, i32 %t.2
```

Justificativa: o comportamento indefinido do C seria a opção mais rápida, mas um erro silencioso numa placa sem tela é muito difícil de achar. Como a verificação é eliminada sempre que o índice ou divisor é constante, o custo só aparece onde ela é necessária. Saída no PC: `[ERRO DE EXECUCAO] indice de vetor fora dos limites (linha 8)` e código de saída `100 + código` (102, 103...). Na placa, a mesma mensagem na serial, e o LED do pino 13 pisca rápido.

### 1.7 `duplocarpado` em placas sem `double` de 64 bits

O `avr-libc` do Arduino Uno não tem as rotinas de `double` de 64 bits (no `avr-gcc`, `double` tem 32 bits). Das três opções do enunciado (rejeitar, rebaixar para `float`, fornecer em software), escolhemos **rebaixar para `float`**, só com `--alvo=uno`, com um aviso na compilação:

```
AVISO: o alvo 'uno' nao tem double de 64 bits; duplocarpado sera gerado como float (32 bits)
```

Rejeitar impediria até o `raiz()` de funcionar na placa (ele sempre devolve `duplocarpado`). Fornecer em software custaria vários KB dos 32 KB de flash. O preço é que, nesse alvo, programas com `duplocarpado` podem imprimir valores ligeiramente diferentes do PC (só programas que dependem de mais de ~7 dígitos significativos). Esse é o **único** ponto em que a IR depende do alvo, e o cabeçalho do `.ll` avisa quando acontece.

### 1.8 Formato de impressão

| tipo | formato | exemplo |
|---|---|---|
| `inteira` | decimal com sinal | `-42` |
| `flut`, `duplocarpado` | **4 casas decimais**, arredondando a metade para longe do zero; notação científica acima de 10¹⁵; `nan`, `inf` | `3.5000`, `-0.1250` |
| `bool` | `VER` / `FAL` | |
| `palavra` | o texto; texto vazio (variável não atribuída) imprime nada | |

A formatação **não usa `printf`**: fica em `runtime/zl_comum.h`, compartilhado por todas as implementações do runtime. O motivo é garantir saída **idêntica** no PC e na placa (o `printf` do `avr-libc` nem imprime `float` por padrão).

E/S na linguagem (funções embutidas, já cadastradas na tabela de símbolos antes do programa):

| Macaronica | runtime |
|---|---|
| `mostrar(a, b, ...)` | imprime cada valor e pula linha (`zl_imprime_*` + `zl_imprime_nl`) |
| `escreve(a, b, ...)` | igual, sem pular linha |
| `novalinha()` | `zl_imprime_nl` |
| `leinteira()`, `leflut()`, `leduplocarpado()` | `zl_le_int`, `zl_le_flut`, `zl_le_duplo` (sem número válido: erro de execução 5) |
| `pino_modo`, `pino_escreve`, `pino_le`, `espera` | E/S física (`docs/placa.md`) |

## 2. Lacunas adicionais identificadas pelo grupo

### 2.1 Valor inicial das variáveis
A especificação não diz. **Decisão:** toda variável começa zerada (`0`, `0.0`, `FAL`, texto vazio; vetores e registros inteiros zerados) **no ponto da declaração**. Uma variável declarada dentro de um laço volta a zero a cada volta. Globais são `zeroinitializer`. Justificativa: elimina leitura de lixo (comportamento indefinido em C), e o custo some no `-O2` quando a variável é atribuída logo depois.

### 2.2 Variáveis globais
O PjBL 1 não tinha globais, mas a Seção 4.1 do PjBL 3 exige `@global`. **Decisão:** uma declaração (`tipo nome;`, `tipo vet{N} nome;` ou `Registro nome;`) fora de qualquer função é global, com a **mesma sintaxe** da declaração local. A gramática continua sem ambiguidade: no topo do arquivo, `tipo principal` é a função principal e `tipo identificador` é global. Uma global é visível em todas as funções, em qualquer posição do arquivo (o semântico registra as globais no passo 1, junto com as assinaturas). Na IR: `@g_nome = global T zeroinitializer`.

### 2.3 Registros usáveis
O PjBL 1 só validava a declaração de registros (decisão 2.7 do `decisoes.md`), mas o PjBL 3 exige `%struct` + `getelementptr`. **Decisão:** variável de registro com `NomeDoRegistro variavel;` e acesso a campo com `variavel.campo`, para leitura e escrita. Isso acrescenta o `.` ao alfabeto (token `TOK_PONTO`). Não usamos `@` porque ele já significa "aqui começa o corpo" na declaração de função e de registro. Limites assumidos: campos só de tipos primitivos, sem atribuição de registro inteiro (`a receba b;` é erro: atribua campo a campo), sem registro como parâmetro ou retorno. O semântico agora também rejeita registro repetido, campo repetido e registro sem campos.

### 2.4 Ordem de avaliação
Argumentos de chamada e operandos são avaliados da **esquerda para a direita**. Em `v{i} receba expr;`, o índice é avaliado antes do valor. Como existem funções com efeito colateral (`leinteira()`), isso precisa estar definido.

### 2.5 Nomes na IR e colisões
Funções do usuário viram `@mac_<nome>`, globais `@g_<nome>`, núcleos vetoriais `@dsl.<op>.<n>`. Sem o prefixo, uma função Macaronica chamada `abs`, `sqrt` ou `main` colidiria com a biblioteca C na ligação. Bytes não-ASCII de identificadores (decisão 2.3 do PjBL 1) viram `_XX` hexadecimal. Locais com o mesmo nome em escopos diferentes recebem sufixo (`%x.addr`, `%x.addr.1`).

### 2.6 Código depois de `respost`
`respost` termina o bloco básico. Se houver comandos depois dele no mesmo bloco, o gerador os coloca num bloco `inalcancavel.N` sem predecessores, que mantém a IR válida e é removido pelo `opt`.

## 3. Análise do mem2reg e das instruções `phi`

Arquivos: `docs/exemplos/somapares.mac` e as três versões da IR ao lado (`somapares.ll`, `.mem2reg.ll`, `.O2.ll`), geradas com:

```
./macaronica docs/exemplos/somapares.mac -o docs/exemplos/somapares.ll
opt -S -passes=mem2reg docs/exemplos/somapares.ll -o docs/exemplos/somapares.mem2reg.ll
opt -S -O2              docs/exemplos/somapares.ll -o docs/exemplos/somapares.O2.ll
```

```
outrafuncao inteira somapares(inteira n) @
[
    inteira i, s;
    s receba 0;
    repete ( i receba 0, i < n, i receba i + 1 ) edai
    [
        cond ( (i // 2) * 2 == i ) edai [ s receba s + i; ]
    ]
    respost s;
]
```

**Antes (forma "em memória").** `i`, `s` e o parâmetro `n` são `alloca` no bloco `entry`. Todo uso é um `load` e toda atribuição um `store`. Não há nenhum `phi`, porque a memória "guarda" o valor entre um bloco e outro.

**Depois de `opt -passes=mem2reg`.** As três `alloca` somem, e com elas todos os `load`/`store`. Surgem **três** `phi`:

```llvm
repete.cond.1:                                    ; preds = %repete.inc.1, %corpo
  %i.addr.0 = phi i32 [ 0, %corpo ], [ %t.13, %repete.inc.1 ]
  %s.addr.0 = phi i32 [ 0, %corpo ], [ %s.addr.1, %repete.inc.1 ]
  ...
cond.fim.2:                                       ; preds = %cond.entao.2, %repete.corpo.1
  %s.addr.1 = phi i32 [ %t.11, %cond.entao.2 ], [ %s.addr.0, %repete.corpo.1 ]
```

Por que aparecem: em SSA cada valor tem **uma única definição**. Um `phi` é necessário sempre que um bloco tem mais de um predecessor e a variável chega com valores diferentes por eles (o bloco está na *fronteira de dominância* das atribuições).

- `%i.addr.0` e `%s.addr.0` ficam em `repete.cond.1`, o **cabeçalho do laço**, que tem dois predecessores: a entrada (`corpo`, onde `i` e `s` valem 0) e a volta do laço (`repete.inc.1`, onde valem o resultado da iteração anterior). O `phi` escolhe o valor conforme de onde o fluxo veio.
- `%s.addr.1` fica em `cond.fim.2`, a **junção do `cond`**: se o `entao` executou, `s` vale `s + i` (`%t.11`); se não, continua o valor que entrou na iteração (`%s.addr.0`).
- `i` não precisa de `phi` em `cond.fim.2` porque não é atribuído dentro do `cond`. `n` não precisa de nenhum, porque nunca é reatribuído: o mem2reg simplesmente troca todo `load %n.addr` pelo próprio argumento `%n`.

**Depois de `-O2`** (comparação pedida no nível Excelente da rubrica):
- `sdiv i32 %i, 2` seguido de `mul ... 2` e `icmp eq` vira `and i32 %i, 1` + `icmp eq ..., 0` (redução de força: "é par" virou teste do bit menos significativo).
- o `cond` some: vira um `select` (`%t.11 = select i1 %t.8, i32 %i, i32 0`), ou seja, o laço fica **sem desvio interno** (if-conversion).
- o laço é rotacionado: o teste `i < n` passa para o fim do corpo e ganha uma guarda `n > 0` antes, então o `phi` do cabeçalho agora tem os predecessores `entry` e o próprio corpo. Aparece um `phi` extra em `repete.fim.1` (`%s.addr.0.lcssa`, forma LCSSA: o valor que sai do laço).
- `i + 1` ganha `nuw nsw`: o otimizador provou que `i` vai de 0 a `n` e nunca estoura.
- em `@zl_principal`, a chamada `somapares(10)` é calculada em compilação: sobra `call void @zl_imprime_int(i32 20)`.

## 4. Biblioteca de execução (Seção 4.6)

`runtime/zl_runtime.h` é a interface (todas as funções de assinatura fixa). As implementações são `zl_runtime_posix.c` (PC e Raspberry Pi) e `zl_runtime_avr.c` (Arduino Uno). Ambas incluem `zl_comum.h`, que tem a formatação de números, as mensagens de erro e `zl_pot_inteira`. Códigos de erro: `1` paridade (Zolang, não usado), `2` índice, `3` divisão inteira por zero, `4` função sem `respost`, `5` leitura inválida.

## 5. Correções do front-end feitas no PjBL 3

O enunciado pede que erros do PjBL 1 sejam corrigidos antes da geração de código. Corrigimos os que nós mesmos encontramos testando (a correção oficial do PjBL 1 ainda não tinha chegado ao grupo):

1. chamada de função usada como **comando** (`f(VER);`) não conferia o **tipo** dos argumentos, só a quantidade; agora as duas formas usam a mesma verificação;
2. vetor usado sem índice numa expressão escalar (`x receba notas + 1;`) era aceito; agora é erro;
3. registro e campo repetidos eram aceitos;
4. `sincronizar()` parava em `}` em vez de `]` e gerava erros falsos em cascata dentro de blocos;
5. operandos de comparação passaram a aceitar expressões (`cond ( (n // 2) * 2 == n )`), ver `decisoes.md` 2.2;
6. a regra de `/` (item 1.2 acima).
