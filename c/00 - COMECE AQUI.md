# C do zero - antes das threads

Você lê C. Nunca escreveu. Isto aqui é a ponte, e ela é curta de propósito.

Cada `passo-NN-*.c` é um **programa completo, pequeno, com uma ideia só**.
Metade deles está **errado de propósito** - você roda, vê quebrar, e o passo
seguinte conserta. Aprender qual mensagem de erro corresponde a qual bug vale
mais que ler a explicação certa três vezes.

## Como rodar

No VS Code (`code ~/c-playground`): abra o arquivo, **`Ctrl+Shift+B`**.

Ou pelo terminal:

```bash
cd ~/c-playground/c-do-zero && make 01
```

`make 01` até `make 40`. `make limpar` apaga os binários.

Os passos `09`, `12`, `14` e `36` **terminam com erro de propósito**: o
sanitizer mata o programa e o `make` reporta falha. É o comportamento
esperado, não um build quebrado.

Os passos 17 e 18 recebem argumentos, então precisam do terminal:

```bash
make 17 ARGS="1e9 4"
```

O passo 19 tem três arquivos (`contas.h`, `contas.c` e o `passo-19`), então o
`Ctrl+Shift+B` **falha nele de propósito** - o erro que aparece é a aula.
Use `make 19`.

## Como usar de verdade

1. Leia o cabeçalho do arquivo (3 linhas).
2. **Rode antes de ler o resto.**
3. Leia o código com os comentários.
4. Leia o rodapé: o diagrama de memória e o "EXPERIMENTE".
5. **Faça pelo menos um dos experimentos.** Esta é a parte que ensina. Quebre
   de propósito, veja a mensagem, conserte.

Não pule os arquivos "errados". Eles são o conteúdo, não o aquecimento.

## Ver acontecendo

`inspetor.html` nesta pasta - abra direto no navegador, sem servidor nenhum.

Cinco painéis interativos para as coisas que o texto explica pior que um
desenho: a seta do ponteiro (`passo-05`), o quadro da pilha sendo reciclado
(`passo-14`), o `&i` do laço contra `&ids[i]`, o `contador++` passo a passo, e
chunk vs esparsa com o resto que fica órfão. Não substitui rodar o código -
serve para olhar antes e depois.

## A ordem

| Passo | Ideia | |
|---|---|---|
| `01` | compilar, `main`, `printf`, código de saída | |
| `02` | todo tipo tem um tamanho fixo em bytes | |
| `03` | divisão inteira e molde errado no printf | ⚠ errado |
| `04` | cast e o especificador certo | ✅ conserta 03 |
| `05` | **`&` e `*`** - endereço e conteúdo, com diagrama | 🔑 |
| `06` | a função que troca dois valores e não troca | ⚠ errado |
| `07` | passar o endereço - por que `scanf` pede `&` | ✅ conserta 06 |
| `08` | vetor não é lista: sem tamanho, sem checagem | |
| `09` | escrever fora do vetor | ⚠ errado |
| `10` | vetor vira ponteiro ao ser passado; `v[i]` é `*(v+i)` | 🔑 |
| `11` | string é vetor de `char` terminado em `\0` | |
| `12` | `strcpy` num destino pequeno | ⚠ errado |
| `13` | `snprintf`: quem escreve precisa do tamanho | ✅ conserta 12 |
| `14` | **tempo de vida**: ponteiro pra variável que morreu | ⚠ errado 🔑 |
| `15` | `malloc` e `free`: memória que é sua até você soltar | ✅ conserta 14 |
| `16` | `struct`, ponto e seta, struct no heap | 🔑 |
| `17` | `argc`/`argv`, e o `atoi` que mente | ⚠ errado |
| `18` | `strtol`/`strtod`: validar a entrada de verdade | ✅ conserta 17 |
| `19` | header `.h` + `.c`, compilar e **linkar** | |

## Bloco 2 - o C que o PPD pede

Daqui em diante os comentários estão **em inglês** (os passos 01–19 estão em
português; peça se quiser que eu traduza para o conjunto ficar uniforme).

| Passo | Ideia | |
|---|---|---|
| `20` | **ponteiro para função** - é o que `pthread_create` recebe | 🔑 |
| `21` | swap "genérico" que só sabe `int` - corrompe `double` em silêncio | ⚠ errado |
| `22` | o genérico de verdade: `void *` + `size_t`, o modelo do `qsort` | ✅ conserta 21 |
| `23` | `signed` vs `unsigned` - o bug que **nenhum sanitizer pega** | 🔑 |
| `24` | bits: flags, máscaras, `<<`, e a precedência de `&` | |
| `25` | `union` e type punning - os mesmos bytes, duas leituras | |
| `26` | `int m[3][4]` **não** é `int *rows[3]` - layout e `argv` | 🔑 |
| `27` | recursão: os quadros empilhando, medidos, até estourar | |

## Bloco 3 - build, linker e o sistema operacional

| Passo | Ideia | |
|---|---|---|
| `28` | as **quatro etapas** do build, rodadas uma por uma | 🔑 |
| `29` | macros são texto: `SQUARE(1+2)` dá 5 | ⚠ errado |
| `30` | o pré-processador no que só ele faz: `__LINE__`, `#`, `##`, `do{}while(0)` | ✅ conserta 29 |
| `31` | tabela de símbolos: o que `nm` mostra e o que o linker procura | 🔑 |
| `32` | bibliotecas `.a` e `.so`, `-L`, `-l`, `ldd`, ordem no link | |
| `33` | `/proc/self/maps`: o diagrama do [[memoria]] conferido no kernel | 🔑 |
| `34` | `printf` x `write(2)`: buffer, `strace`, e por que a saída some no crash | 🔑 |

| `35` | `errno`: as três convenções, e por que pthreads não usa nenhuma delas | 🔑 |
| `36` | `gdb` num programa que quebra de propósito, sessão real capturada | 🔑 |
| `37` | `volatile`, e tudo o que ele **não** faz (spoiler: não serve pra thread) | 🔑 |
| `38` | `_Atomic`: o que o `volatile` nunca deu, medido e com preço | 🔑 |
| `39` | medindo: por que a mesma conta pode ser 23x mais lenta, e por que 8 threads não dá 8x | 🔑 |
| `40` | uma thread vista pelo SO: `clone3`, a página de guarda, `strace -f` | 🔑 |

Complemento: [[tools]] - todo o ferramental (warnings, sanitizers, `nm`,
`gdb`, `strace`, `perf`), com o que está instalado nesta máquina e o que não
está.

Os 🔑 são os que o resto depende. Se um deles não entrou, volte nele
antes de seguir.

Complemento: [[memoria]] - pilha, heap e área estática num diagrama só.

## Depois daqui

Vá para `~/vaultin/Vaulters/01_Courses/PPD/01_PPD_Go/07_Pthreads_Do_Zero/`,
começando por `void-pointer.md` e depois `passo-01`. O passo-16 daqui é
exatamente o pré-requisito do passo-06 de lá.

A conexão é direta, e vale saber desde já qual é:

- **passo-05 + passo-07** → uma thread recebe um `void *`, que é um endereço.
- **passo-14** → passar `&i` do laço pra uma thread é *aquele* bug.
- **passo-15 + passo-16** → a solução: um `malloc` de struct por thread.
- **passo-18** → o Exercício 7 e os dois Desafios pedem `argc`/`argv` com
  validação; o Desafio 1 passa `1e9`, que `atoi` lê como 1.
- **passo-19** → `undefined reference to 'pthread_create'` significa que
  faltou `-pthread`, não que faltou `#include`.

## Quando travar

```bash
man 3 printf      # função de biblioteca
man 2 write       # chamada de sistema
man -k string     # procurar por assunto
```

E leia a mensagem do compilador inteira, de cima pra baixo. Em C, warning é
quase sempre um bug real - foi o gcc que achou os passos 03, 10, 12, 13 e 14
antes da gente rodar.

## Coisas que confundem no começo

| Sintoma | Causa |
|---|---|
| `Segmentation fault` sem mais nada | compilou sem sanitizer |
| seus `printf` somem quando o programa quebra | o buffer não foi esvaziado no crash; a linha rodou sim |
| `undefined reference to 'sqrt'` | faltou `-lm` |
| `implicit declaration of function` | faltou o `#include`; `man 3 <função>` diz qual |
| o número impresso é sempre o mesmo lixo | bug determinístico, não aleatório - passo-03 |

→ [[memoria]] · [[void-pointer]]
