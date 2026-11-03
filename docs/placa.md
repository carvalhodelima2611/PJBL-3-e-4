# Execução em Placa — PjBL 4, Parte D

> **Antes da entrega:** confirmar a escolha da placa com o professor (Seção 5.4 do enunciado) e gravar o vídeo da execução (seção 8 deste arquivo).

## 1. Placa escolhida

**Principal: Arduino Uno (ATmega328P).** AVR de 8 bits, 16 MHz, **32 KB de flash, 2 KB de RAM**, sem sistema operacional. Escolhemos a placa mais restrita da lista de propósito: ela exercita todas as decisões que o enunciado levanta (inteiros de 32 bits num processador de 8, ausência de `double` de 64 bits, RAM minúscula, ausência de SIMD).

**Segunda arquitetura (opcional do nível Excelente): Raspberry Pi 3/4/5 com SO de 64 bits.** AArch64 com Linux. É o **mesmo `.ll`** do Uno e do PC, compilado com outro `--target`.

| | Arduino Uno | Raspberry Pi (64 bits) |
|---|---|---|
| tripla | `avr` (`llc -mtriple=avr`) | `aarch64-linux-gnu` |
| `-mcpu` | `atmega328p` | padrão (`generic`); em um Pi 4 dá para usar `-mcpu=cortex-a72` |
| executável | `placa/<programa>.hex` (e `.elf`) | `placa/<programa>_rpi` (ELF estático) |
| runtime | `runtime/zl_runtime_avr.c` (escrito à mão, só com `avr-libc`) | `runtime/zl_runtime_posix.c` (o mesmo do PC) |
| emulador usado nos testes | `simavr` | `qemu-aarch64` |

## 2. Ferramental e versões

| ferramenta | versão (Ubuntu 24.04) | uso |
|---|---|---|
| LLVM (`llc`, `llvm-link`, `clang`) | 18.1.3 | IR → objeto AVR; IR → ELF ARM64 |
| `avr-gcc` / `avr-libc` / `avr-binutils` | 7.3.0 / 2.0 / 2.26 | compila o runtime da placa e faz a ligação; `.elf` → `.hex` |
| `avrdude` | 7.x (não incluído nos testes) | gravação na placa |
| `simavr` | 1.6 | simulação do ATmega328P nos testes |
| `qemu-aarch64` (qemu-user) | 8.2.2 | execução do binário ARM64 nos testes |
| `gcc-aarch64-linux-gnu` | 13.3.0 | bibliotecas C de AArch64 para a ligação estática |

Instalação: `apt install llvm-18 clang-18 mlir-18-tools gcc-avr avr-libc avrdude simavr qemu-user gcc-aarch64-linux-gnu`, ou usar o `Dockerfile`.

## 3. Construção e gravação

```bash
make placa PROG=tests/placa/04_pisca_led.mac              # -> placa/04_pisca_led.hex
make placa PROG=tests/placa/02_recursao.mac PLACA=rpi     # -> placa/02_recursao_rpi
make placa-todos                                          # todos os programas de tests/placa, nas duas placas
make test-placa                                           # roda todos no simavr e no qemu e compara com o PC
```

O que o `scripts/placa.sh` faz para o Uno:
```bash
./macaronica prog.mac -o prog.ll --alvo=uno                           # verifica a memoria do alvo
scripts/pipeline.sh prog.mlir nucleos.ll base && llvm-link ...         # (so se houver operacoes vetoriais)
llc -mtriple=avr -mcpu=atmega328p -O2 -filetype=obj prog.ll -o prog.o
avr-gcc -mmcu=atmega328p -Os -DF_CPU=16000000UL runtime/zl_runtime_avr.c prog.o -lm -o prog.elf
avr-objcopy -O ihex -R .eeprom prog.elf placa/prog.hex
```

**Gravar** (Uno ligado na USB; no Windows a porta é `COM3`, `COM4`, ...):
```bash
avrdude -p m328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:placa/04_pisca_led.hex
```
Sem `avrdude`, também dá para gravar o `.hex` pela IDE do Arduino com a ferramenta "Upload Hex" ou pelo `arduino-cli upload --input-file`. **Ver a saída:** Monitor Serial da IDE do Arduino a **9600 baud**, ou `screen /dev/ttyACM0 9600`. Como o programa roda assim que a placa liga, abra o monitor e aperte o botão RESET da placa para ver a saída desde o começo.

**Raspberry Pi:** copiar `placa/<programa>_rpi` para o Pi (`scp`), dar `chmod +x` e executar. É estático, não depende de nada instalado no Pi.

## 4. Biblioteca de execução da placa (`runtime/zl_runtime_avr.c`)

- **Saída e entrada pela serial** (USART0, 9600 8N1), escrita direto nos registradores, sem o núcleo do Arduino. `leinteira()` e `leflut()` leem números digitados no Monitor Serial.
- **Erro de execução visível:** a mesma mensagem do PC na serial (`[ERRO DE EXECUCAO] indice de vetor fora dos limites (linha 10)`), e depois o **LED do pino 13 pisca rápido para sempre**, o sinal físico para quando não há computador ligado.
- **Fim do programa:** imprime `[codigo de saida: N]` (no PC esse é o código de saída do processo), e a placa fica parada.
- **Formatação idêntica ao PC:** os números são formatados por `runtime/zl_comum.h`, o mesmo código do runtime POSIX (o `printf` do `avr-libc` nem imprime `float`).
- `powf` é definido no runtime: o `llc` chama esse símbolo, que no `avr-libc` só existe como macro.
- Compilado com `-DZL_SIMULADOR` (só nos testes), o runtime desliga a CPU no fim em vez de ficar em laço, o que faz o `simavr` encerrar.

### Funções embutidas de E/S física

Registradas na tabela de símbolos como `mostrar` (em `semantic.c`). Usar com nome errado ou com tipo errado é erro semântico, como qualquer chamada.

| Macaronica | efeito no Uno | no PC / Raspberry Pi |
|---|---|---|
| `pino_modo(p, m)` | `m = 1` saída, `0` entrada, `2` entrada com pull-up interno | simulado (registra no `stderr`) |
| `pino_escreve(p, v)` | `v = 0` BAIXO, outro valor ALTO | simulado (guarda o valor) |
| `pino_le(p)` | lê o pino (0 ou 1) | devolve o último valor escrito |
| `espera(ms)` | espera ativa de `ms` milissegundos | `nanosleep` |

**Numeração dos pinos:** a **mesma impressa na placa** (0 a 13 digitais, 14 a 19 = A0 a A5), para que um programa da apostila do Arduino funcione igual. O pino 13 é o LED da placa.

## 5. Decisões dependentes do alvo

| decisão | Uno | Raspberry Pi |
|---|---|---|
| `inteira` | `i32` (o `llc` gera as operações de 32 bits com rotinas da `libgcc` do AVR, como `__mulsi3`) | `i32` |
| `duplocarpado` | **rebaixado para `float`** com `--alvo=uno`, com aviso (`decisoes_backend.md` 1.7) | `double` |
| ponteiros / `palavra` | 16 bits | 64 bits |
| textos (`"..."`) | na RAM (o `avr-gcc` copia da flash na partida) | no segmento de dados |
| núcleos vetoriais (DSL) | os mesmos núcleos MLIR, rebaixados no modo `base`: **laço escalar** (o AVR não tem SIMD) | idem; o mesmo pipeline com `vetor` geraria NEON |
| verificação de memória | **sim** (ver abaixo) | não (Linux com memória virtual: documentado, como permite o enunciado) |

### Verificação de memória por alvo

Com `--alvo=uno`, o semântico soma o tamanho de todas as variáveis no alvo (`inteira`/`flut` = 4 bytes, `duplocarpado` = 4 rebaixado, `bool` = 1, `palavra` = 2, registros = soma dos campos, vetores = N × elemento). Ele reporta **erro semântico** quando as globais, ou as globais mais as locais de alguma função, passam de **1600 bytes**:

```
[Linha 4] ERRO SEMANTICO: variaveis locais (2400 bytes) + globais (0 bytes) excedem a RAM disponivel no alvo (1600 de 2048 bytes) na funcao 'principal'
```
(`tests/placa/invalidos/01_excede_ram.mac`, que no PC é um programa válido.)

**Por que 1600 e não 2048:** o próprio runtime e os textos ocupam ~240 bytes de RAM (`avr-size`: `Data` = 236 bytes num programa que só faz `respost 0;`), e a pilha precisa de folga para as chamadas. Medimos isso no `simavr` com um vetor global de N `inteira` e uma recursão de 10 níveis:

| N | RAM estática (`avr-size`) | resultado |
|---|---|---|
| 380 | 1760 B (86%) | correto |
| 440 | 2000 B (98%) | correto (só 48 B de pilha: qualquer recursão mais funda quebraria) |
| 460 | > 2048 B | **o ligador recusa** (`.bss is not within region data`) |

Antes dessa calibração, o compilador aceitava N = 460 e só o ligador falhava. Com 1600, o maior vetor de `inteira` que um programa pode declarar no Uno é de **~395 elementos** (`1600 / 4`, menos as outras variáveis). Limitação conhecida: a verificação é estática e não conta a profundidade da recursão (uma recursão muito funda ainda pode estourar a pilha em execução).

## 6. Programas executados na placa

Em `tests/placa/`, todos com saída esperada **idêntica** à do PC (o `make test-placa` compara a saída do `simavr` e do `qemu` com o mesmo `.esperado`):

| programa | o que mostra | flash / RAM no Uno |
|---|---|---|
| `01_laco_vetor.mac` | laço + vetor (bubble sort) | 7,5 KB (23%) / 252 B (12%) |
| `02_recursao.mac` | funções recursivas (Hanoi, Fibonacci) e `2 ^ 30` em 32 bits num processador de 8 | 7,0 KB (21%) / 336 B (16%) |
| `03_erro_execucao.mac` | erro da linguagem: índice fora do vetor → mensagem + LED piscando | 6,6 KB (20%) / 252 B (12%) |
| `04_pisca_led.mac` | **E/S física**: pisca o LED do pino 13 e lê o próprio pino | 6,6 KB (20%) / 324 B (16%) |
| `05_dsl_na_placa.mac` | núcleos da extensão vetorial (gerados via MLIR) rodando no AVR | 8,0 KB (25%) / 608 B (30%) |
| `invalidos/01_excede_ram.mac` | **rejeitado** pela verificação de memória do alvo | — |

A maior parte dos ~6,5 KB de flash é fixa: rotinas de `float` e de 64 bits usadas pela formatação de números, e as de 32 bits do `libgcc`.

## 7. Lacunas da Seção 5.5 ligadas à placa

- **Maior vetor:** ver a seção 5 (≈ 395 `inteira` ou `flut` no Uno; no Raspberry Pi, o limite é a memória do sistema, mas vetores locais vão para a pilha de 8 MB do Linux, então vetores grandes devem ser globais).
- **Números de pino:** os da placa (seção 4).
- **Núcleos vetoriais sem SIMD:** no Uno, os núcleos são rebaixados no modo `base` (laço escalar). Usar o modo `vetor` não adiantaria: o `llc` do AVR "desmontaria" o `<8 x i32>` em 8 operações escalares de 32 bits, sem ganho e com código maior. `tests/placa/05_dsl_na_placa.mac` prova que o mesmo núcleo roda, com o mesmo resultado.

## 8. Vídeo da execução

> **A fazer pelo grupo:** gravar até 1 minuto com o `04_pisca_led.hex` rodando (LED piscando e Monitor Serial mostrando as linhas) e o `03_erro_execucao.hex` (mensagem de erro e LED piscando rápido). Colocar o link aqui: `<link do vídeo>`.

## 9. Programa inédito na apresentação

Roteiro para o programa que o professor vai fornecer na hora:
```bash
make                                              # se ainda nao compilou
make run   PROG=inedito.mac                       # 1) roda no PC
make placa PROG=inedito.mac                       # 2) gera placa/inedito.hex
avrdude -p m328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:placa/inedito.hex
screen /dev/ttyACM0 9600                          # 3) ve a saida (ou o Monitor Serial da IDE)
```
Se o programa usar `leinteira()`, digite os números no Monitor Serial.
