# Memória em C — o mapa inteiro numa página

Leia depois do `passo-05`, e de novo depois do `passo-15`. É o mesmo desenho
das duas vezes; na segunda ele faz mais sentido.

---

## 1. Uma variável é uma caixa num endereço

```c
int idade = 25;
```

```
    endereço          conteúdo
    0x7ffd1234    ->  [ 25 ]      <- a variável `idade`
```

Duas coisas separadas, e quase toda confusão com ponteiro vem de misturá-las:

- `&idade` → **onde** a caixa está → `0x7ffd1234`
- `idade` → **o que tem** dentro → `25`

## 2. Um ponteiro é uma caixa que guarda um endereço

```c
int *p = &idade;
```

```
    0x7ffd1234    ->  [ 25 ]              <- idade
    0x7ffd9999    ->  [ 0x7ffd1234 ]      <- p
                          |
                          +--> aponta pra caixa de cima
```

`*p` = "vá até esse endereço e leia" → `25`.
Escrever `*p = 30` altera a **caixa original**. Não existe cópia envolvida.

O tipo (`int *` vs `char *`) não muda o tamanho do ponteiro — todo endereço
tem 8 bytes numa máquina 64 bits. O tipo diz **quantos bytes ler** e **como
interpretá-los** quando você faz `*p`. É instrução pro compilador, não algo
guardado na memória.

---

## 3. As três regiões

Este é o desenho que resolve o `passo-14`.

```
  endereços altos
  +--------------------------------------+
  |  PILHA (stack)                       |   cresce pra baixo
  |    variáveis locais, parâmetros,     |
  |    endereços de retorno              |
  |    - o compilador aloca e libera     |
  |    - morre no fim do bloco { }       |
  |    - ~8 MB no total                  |
  |  ..................................  |
  |             (espaço livre)           |
  |  ..................................  |
  |  HEAP                                |   cresce pra cima
  |    malloc / calloc / realloc         |
  |    - VOCÊ aloca e VOCÊ libera        |
  |    - vive até o free                 |
  |    - do tamanho da RAM               |
  +--------------------------------------+
  |  ESTÁTICA                            |
  |    globais, `static`, literais       |
  |    - existe o programa inteiro       |
  |    - zerada no início                |
  +--------------------------------------+
  |  CÓDIGO (só leitura)                 |
  |    as instruções, e as strings       |
  |    literais: char *s = "oi"          |
  +--------------------------------------+
  endereços baixos
```

Na prática você reconhece a região pelo endereço impresso:

```
0x7ffd...  ou  0x7f9c...    pilha       (número enorme)
0x502000...                 heap        (com sanitizer, bem menor)
0x5e5ef3...                 código/estática
```

Rode qualquer `passo-` e compare os `%p` da saída.

---

## 4. Tempo de vida: a pergunta que decide tudo

> **Esta caixa precisa continuar existindo depois que esta função retornar?**

| Resposta | Onde pôr | Como |
|---|---|---|
| não | pilha | `int x;` |
| sim, e é uma só no programa inteiro | estática | `static int x;` |
| sim, e é uma por chamada / por thread | heap | `malloc` |

Errar isso é o `passo-14`:

```
  1) chamou a função:      [ main ][ fabricar: numero=42 ]  <- 0x...bf4
  2) a função retornou:    [ main ][ ...disponível, 42... ]  <- 0x...bf4
  3) chamou outra função:  [ main ][ outra: outro=777     ]  <- 0x...bf4
  4) leu pelo ponteiro:    777
```

O ponteiro não ficou "inválido" — ele é só um número, e continua sendo o
mesmo número. Quem tem que saber que a caixa foi embora é você.

---

## 5. E aí chegam as threads

Uma thread é mais uma linha de execução no **mesmo processo**. Ou seja:

- cada thread ganha a **sua própria pilha**;
- **heap e estática são compartilhados** por todas.

```
  +-----------+  +-----------+  +-----------+
  | pilha T0  |  | pilha T1  |  | pilha T2  |    uma por thread
  +-----------+  +-----------+  +-----------+
        \             |              /
         \            |             /
          +-----------------------+
          |  HEAP  +  ESTÁTICA    |            compartilhados por todas
          +-----------------------+
```

Isso explica de uma vez as duas metades do trabalho de PPD:

**Por que passar `&i` do laço pra uma thread é bug** — `i` está na pilha de
`main`. A thread vai ler aquele endereço mais tarde, quando `main` já mudou o
valor (ou já saiu do bloco onde ele existia). É o `passo-14`, com a
"função que retornou" trocada por "a thread ainda não rodou".

**Por que precisa de mutex** — se duas threads têm um ponteiro pra mesma
caixa no heap, elas escrevem na mesma caixa. É o experimento 3 do `passo-05`
com duas linhas de execução em vez de uma.

**E qual é o padrão certo**: um `malloc` de struct **por thread** (heap, vive
o quanto for preciso, e cada uma tem a sua), com um `free` combinado.

```c
typedef struct { int id; int *dados; size_t inicio, fim; } Tarefa;

for (int i = 0; i < N; i++) {
    Tarefa *t = malloc(sizeof(Tarefa));   /* um por thread */
    t->id = i;                            /* cada um com o SEU id */
    pthread_create(&threads[i], NULL, funcao, t);
}
```

Compare com o errado, que é uma caixa só pra todo mundo:

```c
Tarefa t;                                  /* UMA struct, na pilha de main */
for (int i = 0; i < N; i++) {
    t.id = i;                              /* sobrescreve a cada volta */
    pthread_create(&threads[i], NULL, funcao, &t);   /* todos o mesmo endereço */
}
```

---

→ [[00 - COMECE AQUI]] · [[void-pointer]]
