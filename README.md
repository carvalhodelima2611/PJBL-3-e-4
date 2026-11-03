# Compilador Macaronica — PjBL 1, 3 e 4

Compilador, escrito em C99, da linguagem **Macaronica**. Ele faz as análises léxica, sintática e semântica (PjBL 1) e gera **LLVM IR** portável (PjBL 3). As operações vetoriais da extensão DSL viram núcleos **MLIR**, que são otimizados, medidos (PjBL 4) e executados no PC, num **Arduino Uno** e num **Raspberry Pi**.

```
programa.mac ──► macaronica ──► programa.ll  (+ programa.mlir, se houver operações vetoriais)
                                    │
           ┌────────────────────────┼─────────────────────────────┐
     clang + runtime POSIX     llc -mtriple=avr + runtime AVR     clang --target=aarch64
        (PC / lli)               (Arduino Uno: .hex)            (Raspberry Pi: ELF)
```

## Versões fixadas

| ferramenta | versão |
|---|---|
| gcc (compila o compilador) | qualquer com C99 (testado com 13.3) |
| LLVM: `clang`, `opt`, `llvm-as`, `lli`, `llc`, `llvm-link` | **18.1.3** |
| MLIR: `mlir-opt`, `mlir-translate` | **18.1.3** (pacote `mlir-18-tools`) |
| OpenMP (C3 do estudo) | `libomp-18-dev` |
| Arduino Uno: `avr-gcc` / `avr-libc` / `simavr` / `avrdude` | 7.3.0 / 2.0.0 / 1.6 / 7.x |
| Raspberry Pi: `gcc-aarch64-linux-gnu` / `qemu-user` | 13.3 / 8.2.2 |
| Python 3 + matplotlib (gráficos do estudo) | 3.12 / 3.x |

O caminho mais simples é o contêiner, que já tem tudo instalado:
```bash
docker build -t macaronica .
docker run --rm -it -v "$PWD":/trabalho macaronica make test
```
Sem Docker (Ubuntu 24.04): `sudo apt install build-essential llvm-18 clang-18 mlir-18-tools libomp-18-dev gcc-avr avr-libc simavr avrdude qemu-user gcc-aarch64-linux-gnu python3-matplotlib bc`.

## Uso

```bash
make                                         # compila o compilador: ./macaronica
./macaronica prog.mac                        # só valida (como no PjBL 1)
./macaronica prog.mac -o prog.ll             # valida e gera LLVM IR (e prog.mlir se houver DSL)
./macaronica prog.mac -o prog.ll --alvo=uno  # alvo da placa: verificação de memória + duplocarpado 32 bits

make run PROG=tests/execucao/04_recursao.mac                # compila e executa no PC
make placa PROG=tests/placa/04_pisca_led.mac                # placa/04_pisca_led.hex (Arduino Uno)
make placa PROG=tests/placa/04_pisca_led.mac PLACA=rpi      # placa/04_pisca_led_rpi (Raspberry Pi)
make test                                    # TODOS os testes (ver abaixo)
make bench                                   # estudo de otimização: tabela e gráficos em docs/resultados/
```

Para programas inválidos, o compilador mostra os erros (fase, linha, coluna no léxico, token inesperado no sintático) e **não gera arquivo de saída**.

Caminho manual, passo a passo (o que o `make run` faz):
```bash
./macaronica prog.mac -o prog.ll
scripts/pipeline.sh prog.mlir nucleos.ll && llvm-link -S prog.ll nucleos.ll -o prog.ll   # só se houver prog.mlir
llvm-as prog.ll -o /dev/null && opt -passes=verify prog.ll -o /dev/null                  # a IR é válida
clang prog.ll runtime/zl_runtime_posix.c -lm -o prog && ./prog
```

## Testes (`make test`)

| alvo | o que faz |
|---|---|
| `test-pjbl1` | `tests/validos` aceitos; `tests/invalidos`, `tests/dsl/invalidos` e `tests/placa/invalidos` rejeitados **sem** gerar `.ll` |
| `test-execucao` | 14 programas de `tests/execucao`: IR passa no `llvm-as` e no `opt -passes=verify`, e roda com **`lli`** e como **executável nativo**; a saída tem de ser igual ao `.esperado` (com o código de saída na última linha; `.entrada` vai para o stdin) |
| `test-dsl` | os programas da DSL, **três vezes**: núcleos sem otimização, vetorizados no MLIR e paralelos com OpenMP |
| `test-placa` | `tests/placa` no **simavr** (Arduino Uno) e no **qemu-aarch64** (Raspberry Pi), com a mesma saída esperada do PC; e a verificação de memória do Uno |

Os testes de execução cobrem o mínimo do enunciado (precedência e associatividade, condicionais aninhadas, as duas repetições, recursão, vetor, registro, E/S, tipos mistos, `^` e `raiz()`, `/` contra `//`) e casos de borda: erros de execução com a linha do fonte, escopos e sombreamento, globais, curto-circuito evitando um acesso fora do vetor, vetor de tamanho 1.

## A linguagem em uma página

```
registro Aluno @ [ palavra nome; inteira idade, nota; ]   /* tipo registro */
inteira vet{100} global;                                  /* variavel global (comeca zerada) */

outrafuncao inteira quadrado(inteira n) @
[
    respost n * n;
]

inteira principal() @
[
    inteira i, s;                         /* declaracao e atribuicao sao comandos separados */
    flut f;
    inteira vet{5} v;                     /* tamanho com { } */
    Aluno a;
    s receba 0;
    repete ( i receba 0, i < 5, i receba i + 1 ) edai
    [
        v{i} receba quadrado(i);           /* indice tambem com { } */
        s receba s + v{i};
    ]
    a.nome receba "Ana";                  /* campo com . */
    f receba s / 2;                       /* / e divisao REAL: 15.0000 */
    cond ( s // 2 > 10 E a.idade == 0 ) edai [ mostrar("s = ", s, ", f = ", f); ]
    casocontrario edai [ mostrar("outro caso"); ]
    durante ( s > 0 ) edai [ s receba s - 7; ]
    respost 0;                            /* vira o codigo de saida do programa */
]
```

- Tipos: `inteira` (32 bits), `flut`, `duplocarpado`, `bool` (`VER`/`FAL`), `palavra` (`"texto"`), `vazio` (só retorno).
- Operadores: `+ - * / // ^`, `raiz(x)`; comparadores `< > == !` (`!` = diferente); lógicos `E OU XOR NAO`. Comentários `/* ... */`.
- E/S: `mostrar(a, b, ...)` (pula linha), `escreve(...)` (não pula), `novalinha()`, `leinteira()`, `leflut()`, `leduplocarpado()`.
- Placa: `pino_modo(pino, 1=saída|0=entrada|2=pull-up)`, `pino_escreve(pino, valor)`, `pino_le(pino)`, `espera(ms)`.
- DSL vetorial: `c receba a + b;`, `c receba 3 * a;`, `somavet(a)`, `maxvet(a)`, `prodesc(a, b)`.

## Estrutura

```
src/        lexer, parser, semantic (PjBL 1) + codegen_llvm.c, codegen_mlir.c (PjBL 3/4), main.c
runtime/    zl_runtime.h (interface), zl_comum.h (formatação comum), zl_runtime_posix.c, zl_runtime_avr.c
scripts/    pipeline.sh (MLIR -> LLVM IR, comentado), macc.sh, placa.sh, testa_*.sh, bench/ (estudo)
tests/      validos/ invalidos/ (PjBL 1), execucao/ (PjBL 3), dsl/ (PjBL 4), placa/ (Parte D)
docs/       decisoes.md (PjBL 1 + gramática), decisoes_backend.md (PjBL 3), dsl.md,
            relatorio_otimizacao.md (Partes B e C), placa.md (Parte D), resultados/, exemplos/
placa/      executáveis de entrega: .hex/.elf (Arduino Uno) e _rpi (Raspberry Pi 64 bits)
```

## Documentação

- [`docs/decisoes.md`](docs/decisoes.md): lacunas da especificação (PjBL 1), atualizações e **gramática completa**.
- [`docs/decisoes_backend.md`](docs/decisoes_backend.md): mapeamento de tipos, todas as lacunas da Seção 4.3 do PjBL 3, verificações em execução, **análise do mem2reg e das instruções `phi`**.
- [`docs/dsl.md`](docs/dsl.md): especificação da extensão vetorial e lacunas da Seção 5.5.
- [`docs/relatorio_otimizacao.md`](docs/relatorio_otimizacao.md): núcleo em cada nível do pipeline, estudo C0 a C3 e a pergunta da redução de ponto flutuante.
- [`docs/placa.md`](docs/placa.md): Arduino Uno e Raspberry Pi, comandos, verificação de memória, tamanhos.

## Uso de IA generativa

O projeto foi desenvolvido com apoio de IA generativa (Claude, da Anthropic), em todas as etapas:

- **PjBL 1:** revisão de uma implementação inicial do grupo (que tinha bugs de robustez, como laço infinito ao encontrar um comando inválido, buffer de entrada fixo de 1024 bytes e operadores lógicos não reconhecidos, além de partes da gramática não implementadas), reescrita dos três módulos do front-end, comentários no código e documentação das decisões de projeto.
- **PjBL 3:** extensões do front-end (globais, registros usáveis, funções embutidas de E/S, regra de `/` e `//`), gerador de LLVM IR (`codegen_llvm.c`), biblioteca de execução, bateria de testes de execução e `decisoes_backend.md`.
- **PjBL 4:** extensão vetorial, emissor MLIR e `pipeline.sh`, scripts de benchmark e de gráficos, runtime do Arduino Uno, scripts de placa e de teste em emulador, e os documentos `dsl.md`, `relatorio_otimizacao.md` e `placa.md`.

Todo o código foi testado antes da entrega: compila sem warnings com `-Wall -Wextra -pedantic`; o `make test` passa inteiro; e o compilador foi submetido a 3.000 programas corrompidos aleatoriamente com AddressSanitizer, UBSan e LeakSanitizer, sem nenhum travamento, laço infinito ou vazamento, e com toda IR gerada aprovada pelo `opt -passes=verify`. **A validação na placa física e o vídeo ficam a cargo do grupo** (os testes usaram os emuladores `simavr` e `qemu`). Conforme exigido pelo enunciado, cada integrante deve ser capaz de explicar qualquer trecho do código e da IR gerada.
