# Front-end Macaronica

Front-end de compilador (análise léxica, sintática e semântica) para a
linguagem **Macaronica**, definida no Trabalho Prático de Compiladores
(disciplina Linguagens Formais e Compiladores).

Não há geração de código: o programa lê um arquivo-fonte `.mac` e
informa se ele é léxica, sintática e semanticamente válido, apontando
linha (e, na fase léxica, também coluna) e a causa de cada erro
encontrado.

## Compilação

Requer `gcc` com suporte a C99.

```bash
make          # compila o binario ./macaronica
make test     # roda todos os casos de teste em tests/ e mostra um resumo
make clean    # remove os artefatos de build
```

A compilação usa `-std=c99 -Wall -Wextra -pedantic` e não produz
nenhum warning.

## Uso

```bash
./macaronica caminho/para/programa.mac
# ou, lendo da entrada padrao:
./macaronica < caminho/para/programa.mac
```

Saída (`stdout`) resume o resultado de cada fase; erros específicos
(com linha/coluna) vão para `stderr`. Código de saída `0` significa
programa válido; `1` significa que alguma fase encontrou erro.

Exemplo:

```
=== Front-end Macaronica ===
> Analise Lexica: OK (58 tokens reconhecidos).
> Analise Sintatica: OK (arvore sintatica construida sem erros).
> Analise Semantica: OK (tipos, escopo e assinaturas de funcao verificados).
>> Programa Macaronica VALIDO (lexica, sintatica e semanticamente).
```

## Estrutura do projeto

```
projeto/
├── src/
│   ├── lexer.h/.c      -> analisador lexico (tokens, linha/coluna)
│   ├── parser.h/.c     -> AST + analisador sintatico (recursive descent)
│   ├── semantic.h/.c   -> tabela de simbolos, escopo, tipos, assinaturas
│   └── main.c          -> orquestracao das 3 fases
├── tests/
│   ├── validos/        -> 5 programas validos
│   └── invalidos/      -> 6 programas invalidos (1 por tipo de falha + 1 extra)
├── docs/
│   └── decisoes.md     -> gramatica assumida e justificativa de cada decisao
├── Makefile
└── README.md
```

## Arquitetura

Cada fase é um módulo independente:

- **lexer**: tokeniza toda a entrada de uma vez (`lexer_tokenizar`),
  reportando *todos* os erros léxicos encontrados (caractere fora do
  alfabeto, string não terminada) com linha e coluna, sem abortar no
  primeiro erro.
- **parser**: constrói uma AST via descida recursiva. Ao encontrar um
  erro sintático, reporta e sincroniza (avança até `;`, `]` ou fim de
  arquivo) para continuar procurando outros erros na mesma execução,
  em vez de abortar na primeira falha.
- **semantic**: percorre a AST com uma tabela de símbolos em pilha de
  escopos (global, por função e por bloco de `cond`/`repete`/`durante`),
  verificando declaração antes do uso, compatibilidade de tipos e
  assinatura de chamadas de função (número e tipo de argumentos, tipo
  de retorno).

A compilação nunca trava, nem em entradas vazias, malformadas ou muito
grandes (o buffer de entrada é dinâmico, sem limite fixo) — validado
com AddressSanitizer/UBSan sobre todos os casos de teste.

## Ambiguidades da especificação original

Ver [`docs/decisoes.md`](docs/decisoes.md) para a resolução detalhada
de cada lacuna da Seção 4.3 do enunciado, além de ambiguidades
adicionais identificadas pelo grupo (alfabeto incompleto, uso dos
operadores lógicos, coerção de tipos, comentários, entre outras).

## Uso de IA generativa

Este front-end foi desenvolvido com apoio de IA generativa (Claude,
Anthropic) para: (i) revisão de uma implementação inicial do grupo,
identificando bugs de robustez (loop infinito em `funcao()` ao
encontrar um comando inválido, buffer de entrada fixo de 1024 bytes,
falha em reconhecer `OU`/`E`/`NAO`/`XOR`, ausência de suporte a
literais de texto) e lacunas de cobertura de gramática (`cond`,
`repete`, `durante`, `registro`, chamadas de função e expressões
aritméticas não estavam implementados); (ii) reescrita completa do
front-end nos três módulos exigidos, cobrindo a gramática completa da
linguagem; (iii) elaboração da documentação de decisões de projeto.
Todo o código foi revisado e testado (compilação sem warnings sob
`-Wall -Wextra -pedantic`, execução sob AddressSanitizer/UBSan, e
validação contra o exemplo oficial do enunciado e os 11 casos de
teste) antes da entrega. Cada integrante do grupo deve estar apto a
explicar qualquer trecho do código, conforme exigido pela Seção 8 do
enunciado.
