# Decisões de Projeto — Front-end Macaronica

Este documento resolve, e justifica, cada lacuna da especificação original
listada na Seção 4.3 do enunciado, além de outras ambiguidades identificadas
proativamente pelo grupo durante a implementação.

## 1. Ambiguidades listadas no enunciado (Seção 4.3)

### 1.1 Acesso a vetor: `{}` ou `[]`?

A especificação usa `{ }` para o **tamanho** na declaração
(`tipo vet{tamanho} nome;`) e sugere, sem mostrar exemplo, que `{ }`
também serviria para o **acesso** a um elemento.

**Decisão:** seguimos a sugestão da especificação e usamos `{ }` nos
dois casos: no **tamanho** do vetor, na declaração
(`inteira vet{10} notas;`), e no **índice**, no acesso a um elemento
(`notas{i}`), tanto para leitura em expressões (`x receba notas{i};`)
quanto para escrita (`notas{i} receba valor;`).

**Justificativa:** é a leitura mais fiel da especificação, que usa `{ }`
para vetores e já presume esse mesmo símbolo para o acesso. Apesar de o
símbolo ter dois papéis, isso **não** gera ambiguidade de gramática,
porque o parser decide pelo token que vem **antes** da chave, que é
sempre diferente nos dois casos:

- chave logo depois da palavra reservada `vet` → é o **tamanho**
  (regra de declaração);
- chave logo depois de um **identificador** → é o **índice** (regra de
  acesso/atribuição).

Os dois contextos nunca se sobrepõem, então basta olhar o token atual
para escolher a regra (continua LL(1) nesse ponto). Os colchetes `[ ]`
ficam reservados exclusivamente para os blocos de comando (corpo de
função, de registro, de `cond`, `repete` e `durante`), e assim cada
símbolo tem um único papel por contexto.

### 1.2 Sintaxe de chamada de função

Não definida explicitamente pela especificação original.

**Decisão:** adotamos `nome(argumentos)`, exatamente como sugerido no
próprio enunciado, tanto em contexto de expressão
(`resultado receba quadrado(numero);`, como no exemplo oficial) quanto
como comando isolado (`mostrar(x);`, chamada por efeito colateral, sem
uso do valor de retorno).

### 1.3 `respost` em função `vazio`

Não estava claro se o comando deveria ser omitido ou usado sem valor.

**Decisão:** para uma função `vazio`, `respost;` (sem valor) é
permitido, e a ausência total de `respost` também é permitida. Usar
`respost <valor>;` em uma função `vazio` é **erro semântico**. Para
qualquer outro tipo de retorno, `respost` deve trazer exatamente uma
expressão, e a função deve conter ao menos um `respost` com valor em
algum ponto do seu corpo (verificado pelo analisador semântico).

**Justificativa:** trata `vazio` de forma simétrica ao `void` de C,
que é a convenção mais previsível para quem lê o código, sem inventar
uma sintaxe nova não sugerida pelo material original.

## 2. Ambiguidades adicionais identificadas pelo grupo

### 2.1 Alfabeto incompleto

O alfabeto formal informado (`[ ] { } ( ) a..z A..Z 0..9 + - * / ; ^ < > = !`)
**não inclui** a vírgula (`,`) nem o `@`, mesmo que ambos apareçam
explicitamente nas estruturas da linguagem (parâmetros de função,
cabeçalho de função/registro). Também não inclui nenhum delimitador
para literais de texto, apesar de a linguagem definir o tipo `palavra`
(string).

**Decisão:** tratamos `,` e `@` como parte do alfabeto prático da
linguagem (são necessários para qualquer programa com mais de um
parâmetro ou qualquer função/registro). Para literais de texto,
adotamos aspas duplas (`"texto"`), por analogia direta com a convenção
já usada nos exemplos de Zolang (a linguagem irmã definida no mesmo
enunciado), garantindo consistência entre as duas linguagens do
trabalho.

### 2.2 Onde entram os operadores lógicos `OU`, `E`, `NAO`, `XOR`?

A regra formal de `CONDIÇÃO` (`VAR COMPARADOR VAR | VAR COMPARADOR NUM
| NUM COMPARADOR NUM`) só define uma comparação atômica única. Os
operadores lógicos aparecem na tabela de tokens, mas a gramática nunca
mostra como combiná-los com uma `CONDIÇÃO`.

**Decisão:** estendemos a regra para permitir combinar comparações
atômicas com `OU`/`E`/`XOR`, e negar qualquer condição com `NAO`,
sempre dentro do escopo de `cond(...)`, `durante(...)` ou da cláusula
de condição do `repete(...)`:

```
condicao      -> cond_simples ( (OU|E|XOR) cond_simples )*
cond_simples  -> NAO cond_simples | ( condicao ) | operando comparador operando
```

Cada comparação individual continua exigindo exatamente a forma
`VAR/NUM COMPARADOR VAR/NUM` prevista na especificação — não
inventamos comparações compostas, apenas demos um lugar de uso aos
operadores lógicos já definidos na tabela de tokens. Fora desse
contexto (por exemplo, do lado direito de um `receba`), os operadores
lógicos **não** são aceitos; isso é uma limitação deliberada e não um
esquecimento, para não extrapolar além do que a gramática formal
define.

**Atualização (PjBL 3):** os operandos de cada comparação passaram a aceitar **expressões aritméticas**, e não só `VAR`/`NUM`: `cond ( (n // 2) * 2 == n )`, `durante ( i < n - 1 )`. A forma original da especificação continua valendo (é um caso particular). Motivo: no PjBL 3 os programas passaram a ser executados, e a restrição obrigava a criar variáveis temporárias só para comparar uma conta. Um `(` no início de uma condição é decidido olhando à frente (sem consumir tokens): se dentro desse parêntese, no primeiro nível, aparece um comparador ou operador lógico, ele agrupa **condições** (`(a < 1 OU b > 2)`); senão, agrupa uma **expressão** (`(a + 1) < b`).

### 2.3 Palavra reservada "NÃO" com acento

"NÃO" contém o caractere acentuado `Ã` (dois bytes em UTF-8), o que é
frágil de tratar com as funções padrão de `<ctype.h>` (`isalpha`,
`isalnum`), cujo comportamento para bytes fora do intervalo ASCII é
indefinido em C.

**Decisão:** o lexer aceita qualquer byte `>= 0x80` como continuação
válida de identificador, o que permite capturar corretamente a
sequência multibyte de "NÃO", comparando-a literalmente (em UTF-8)
com a palavra reservada. Como efeito colateral documentado, isso
significa que outros caracteres não-ASCII (acentos em identificadores,
por exemplo) também são aceitos como parte de um identificador — uma
simplificação aceitável para o escopo deste trabalho, já que o
objetivo é reconhecer corretamente o token exigido pela especificação
sem implementar uma tabela completa de classificação Unicode.

### 2.4 Comparador "diferente": `!` ou `!=`?

A tabela de comparadores lista `!` isoladamente como "diferente", não
`!=` (diferente da maioria das linguagens no estilo C).

**Decisão:** seguimos a especificação literalmente: um único `!` é o
token de comparação "diferente". A negação lógica é a palavra `NAO`
("NÃO"), que é um token totalmente distinto e não conflita com `!`.

### 2.5 Coerção de tipos em atribuições e expressões

A especificação não define nenhuma regra de compatibilidade entre os
tipos numéricos (`inteira`, `flut`, `duplocarpado`).

**Decisão:** permitimos apenas ampliação implícita, na ordem
`inteira -> flut -> duplocarpado`; qualquer atribuição ou passagem de
argumento no sentido contrário (estreitamento) é erro semântico. Em
expressões aritméticas, o tipo do resultado é o do operando de maior
amplitude.

**Atualização (PjBL 3):** a exceção é a divisão. `/` sempre dá um resultado real (no mínimo `flut`, mesmo entre dois `inteira`) e `//` sempre dá `inteira` (ver `docs/decisoes_backend.md` 1.2).

### 2.6 Comentários

A especificação não define comentários. Adotamos comentários de bloco
no estilo C (abre com barra-asterisco, fecha com asterisco-barra).
Não adotamos comentário de linha com barra dupla, pois esse token já é
o operador de divisão inteira da linguagem (Seção 4.2); reutilizá-lo
como comentário criaria uma ambiguidade lexica real e desnecessária.

### 2.7 Acesso a campo de registro (`struct`)

A especificação define como declarar um `registro`, mas não define
nenhuma sintaxe para acessar um campo de uma variável desse tipo
(equivalente a `ponto.x` em C). Por não haver nenhuma pista na
especificação sobre a forma pretendida, o grupo optou por **não
inventar** essa sintaxe: o analisador léxico e sintático reconhecem e
validam a declaração de registros, mas o uso de valores desse tipo
(além da própria declaração) está fora do escopo desta entrega e não
é uma limitação silenciosa — está documentada aqui.

**Atualização (PjBL 3):** como a geração de código exige registros usáveis (`%struct` + `getelementptr`), a decisão foi revista: variável de registro com `NomeDoRegistro nome;` e acesso a campo com `nome.campo` (leitura e escrita), o que acrescenta o `.` ao alfabeto. Detalhes e limites em `docs/decisoes_backend.md` 2.3.

### 2.8 Entrada e saída de dados

Nem a tabela de tokens nem a lista de estruturas de Macaronica definem
comandos de leitura/impressão (diferente de Zolang, que define
`mostrar{...}` e `leitura{...}`). Como o enunciado (Seção 7) exige que
os casos de teste válidos cubram "ao menos um uso de entrada/saída", e
a especificação de Macaronica não define nenhum comando desse tipo,
tratamos parâmetros de função e o valor de `respost` como o mecanismo
de entrada/saída de dados de um programa Macaronica (uma função recebe
dados através dos parâmetros e devolve resultados através de
`respost`), e os casos de teste válidos exercitam esse mecanismo.

**Atualização (PjBL 3):** com a execução dos programas, entrada e saída passaram a ser **funções embutidas**, já cadastradas na tabela de símbolos antes do programa: `mostrar(...)`, `escreve(...)`, `novalinha()`, `leinteira()`, `leflut()`, `leduplocarpado()` (e, para a placa, `pino_modo`, `pino_escreve`, `pino_le`, `espera`). Elas usam a sintaxe de chamada já decidida no item 1.2, então nenhuma regra sintática nova foi necessária. Os parâmetros da principal continuam sendo um mecanismo de entrada: eles são lidos da entrada padrão (`docs/decisoes_backend.md` 1.5).

## 3. Regras gerais assumidas (fora da Seção 4.3)

- A função `func`, listada na tabela de palavras reservadas, nunca é
  usada em nenhuma estrutura da linguagem (as estruturas usam
  `outrafuncao` e `principal`). Mantivemos `func` como palavra
  reservada (não pode ser usada como identificador), mas ela não tem
  papel sintático nesta implementação.
- Deve existir exatamente uma função `principal` no programa; zero ou
  mais de uma é erro semântico.
- Índices de vetor devem ser do tipo `inteira`.
- O escopo mínimo exigido (global + função) foi implementado com
  aninhamento adicional: cada bloco de `cond`/`repete`/`durante` abre
  seu próprio escopo interno, permitindo variáveis locais ao bloco.

## 4. Gramática completa (versão do PjBL 3/4)

Notação: `X*` = zero ou mais, `X?` = opcional, `|` = alternativa, termos entre aspas são tokens literais. O parser é de descida recursiva, LL(1) em quase todo ponto. Os únicos lugares com olhar à frente maior estão anotados.

```
programa      -> ( registro | funcao | decl_global )*
registro      -> "registro" ID "@" "[" ( tipo ID ( "," ID )* ";" )* "]"
funcao        -> "outrafuncao" tipo ID "(" params? ")" "@" bloco
               | tipo "principal" "(" params? ")" "@" bloco          (LL(2): "tipo principal")
params        -> tipo ID ( "," tipo ID )*
decl_global   -> decl | decl_reg                                    (no topo do arquivo = global)
bloco         -> "[" comando* "]"

comando       -> decl | decl_reg | atrib ";" | cond | repete | durante | respost | chamada ";"
decl          -> tipo ( "vet" "{" NUM "}" )? ID ( "," ID )* ";"
decl_reg      -> ID ID ( "," ID )* ";"                               (LL(2): ID seguido de ID)
atrib         -> ID ( "{" expr "}" | "." ID )? "receba" expr
cond          -> "cond" "(" condicao ")" "edai" bloco
                 ( "casocontrario" ( cond | "edai" bloco ) )?
repete        -> "repete" "(" atrib "," condicao "," atrib ")" "edai" bloco
durante       -> "durante" "(" condicao ")" "edai" bloco
respost       -> "respost" expr? ";"
chamada       -> ID "(" args? ")"                                    (LL(2): ID seguido de "(")
args          -> expr ( "," expr )*

condicao      -> cond_simples ( ( "OU" | "E" | "XOR" ) cond_simples )*
cond_simples  -> ( "NAO" | "NÃO" ) cond_simples
               | "(" condicao ")"                                    (olha a frente: ver 2.2)
               | expr comparador expr
comparador    -> "<" | ">" | "==" | "!"

expr          -> termo ( ( "+" | "-" ) termo )*
termo         -> fator ( ( "*" | "/" | "//" | "^" ) fator )*
fator         -> NUM | TEXTO | "VER" | "FAL" | "raiz" "(" expr ")" | "(" expr ")" | "-" fator
               | ID | ID "{" expr "}" | ID "." ID | ID "(" args? ")"
tipo          -> "inteira" | "flut" | "bool" | "palavra" | "duplocarpado" | "vazio"
```

Precedência (da maior para a menor): `- unário`; `* / // ^` (mesmo nível, associativos à esquerda); `+ -`; comparadores; `NAO`; `OU E XOR` (mesmo nível, à esquerda).

Regras que ficam no **semântico**, e não na gramática: atribuição a um vetor sem índice só é aceita como operação vetorial da DSL (`docs/dsl.md`); `somavet`, `maxvet` e `prodesc` são funções embutidas; uma declaração no topo do arquivo é global.

## 5. Histórico de alterações após o PjBL 1

| item | o que mudou | onde |
|---|---|---|
| 1.1 | índice de vetor passou de `[ ]` para `{ }` | `parser.c` |
| 2.2 | comparações aceitam expressões | `parser.c` (`parse_cond_simples`) |
| 2.7 | registros usáveis: `Registro x;` e `x.campo` | `lexer.c`, `parser.c`, `semantic.c` |
| 2.8 | E/S por funções embutidas | `semantic.c` |
| novo | variáveis globais | `parser.c` (`parser_analisar`) |
| novo | `/` é sempre divisão real; `//` sempre inteira | `semantic.c`, `decisoes_backend.md` 1.2 |
| correção | chamada como comando agora confere o tipo dos argumentos | `semantic.c` |
| correção | vetor sem índice em expressão escalar agora é erro | `semantic.c` |
| correção | registro e campo repetidos agora são erro | `semantic.c` |
| correção | `sincronizar()` parava em `}` em vez de `]` (erros falsos em cascata) | `parser.c` |
