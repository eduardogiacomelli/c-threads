/* ============================================================================
 * PASSO 10 — o vetor "decai" pra ponteiro quando você o passa adiante.
 *
 * Isto explica de uma vez: por que uma função que recebe vetor precisa
 * receber o tamanho junto, por que sizeof mente lá dentro, e por que
 * v[i] e *(v+i) são a mesma coisa escrita de dois jeitos.
 *
 *     Ctrl+Shift+B      (ou: make 10)
 * ========================================================================= */

#include <stdio.h>

/* Escrevi `int v[]` aqui de propósito, porque é como quase todo mundo
 * escreve. Mas isto é uma MENTIRA SIMPÁTICA do C: o parâmetro não é um
 * vetor. O compilador reescreve silenciosamente para `int *v`.
 *
 * O vetor inteiro NÃO é copiado na chamada (seria caro). Só o endereço do
 * primeiro elemento é copiado — regra do passo-06, sem exceção.
 *
 * Consequência: `n` não é frescura. Sem ele, a função não tem como saber
 * onde o vetor acaba. */
int somar(int v[], size_t n)
{
    /* AQUI ESTÁ A ARMADILHA. sizeof(v) não é o tamanho do vetor: v é um
     * ponteiro, então sizeof(v) é 8. O truque do passo-08 só funciona no
     * escopo onde o vetor foi DECLARADO. */
    printf("   [dentro de somar] sizeof(v) = %zu  <- 8, é um ponteiro!\n",
           sizeof(v));

    int soma = 0;
    for (size_t i = 0; i < n; i++)
        soma += v[i];
    return soma;
}

int main(void)
{
    int notas[5] = {7, 8, 10, 6, 9};

    printf("[em main] sizeof(notas) = %zu  <- 20, o bloco inteiro\n",
           sizeof(notas));

    /* O nome do vetor, usado sozinho, VALE o endereço do primeiro elemento.
     * Estas duas linhas imprimem o mesmo número: */
    printf("\nnotas     = %p\n", (void *) notas);
    printf("&notas[0] = %p   <- o mesmo endereço\n", (void *) &notas[0]);

    /* Então isto é legal, e é o que a chamada de função faz por baixo: */
    int *p = notas;
    printf("\n*p        = %d   (o primeiro elemento)\n", *p);
    printf("p[2]      = %d   (indexar um PONTEIRO, sem vetor nenhum)\n", p[2]);

    /* ARITMÉTICA DE PONTEIRO: somar 1 a um ponteiro anda UM ELEMENTO, não um
     * byte. O compilador multiplica pelo sizeof do tipo apontado.
     *
     *     p + 1  ->  endereço + 4  (porque é int *)
     *
     * E aí a identidade que define a indexação em C:
     *
     *     v[i]  É POR DEFINIÇÃO  *(v + i)
     *
     * Colchete é açúcar sintático para "ande i elementos e desreferencie". */
    printf("\np     = %p  -> *p     = %d\n", (void *) p,     *p);
    printf("p + 1 = %p  -> *(p+1) = %d   (andou 4 bytes)\n",
           (void *) (p + 1), *(p + 1));
    printf("notas[1] = %d  == *(notas + 1) = %d\n", notas[1], *(notas + 1));

    /* Consequência divertida e inútil: como v[i] é *(v+i) e a soma é
     * comutativa, 1[notas] compila e funciona. Nunca escreva isso. Serve só
     * pra provar que colchete é mesmo só notação. */
    printf("1[notas] = %d   (funciona, e é horrível)\n", 1[notas]);

    /* A chamada: passamos `notas` (que decai pra ponteiro) e o tamanho. */
    size_t n = sizeof(notas) / sizeof(notas[0]);   /* calcule AQUI, em main */
    printf("\nsoma = %d\n", somar(notas, n));

    /* E como a função só tem um ponteiro, dá pra somar um PEDAÇO do vetor:
     * `notas + 2` é o endereço do terceiro elemento, e daí em diante são 3.
     *
     * É exatamente assim que se divide trabalho entre threads: cada uma
     * recebe um ponteiro pro seu pedaço e o tamanho dele. Guarde isto. */
    printf("soma dos 3 últimos = %d\n", somar(notas + 2, 3));

    return 0;
}

/* ============================================================================
 * O DIAGRAMA
 *
 *     notas ---> 0x7ffd1000 [  7 ]   notas[0]  *(notas+0)
 *                0x7ffd1004 [  8 ]   notas[1]  *(notas+1)
 *                0x7ffd1008 [ 10 ]   notas[2]  *(notas+2)
 *                0x7ffd100c [  6 ]   notas[3]
 *                0x7ffd1010 [  9 ]   notas[4]
 *
 *     somar(notas, 5)  copia só a seta, não as caixas.
 *
 *     somar(notas + 2, 3)
 *                       \---> começa em 0x7ffd1008, vê 3 caixas
 *
 * AS TRÊS FRASES DESTE PASSO
 *
 *   1. Passar um vetor para uma função passa um PONTEIRO. Nada é copiado.
 *   2. Por isso a função sempre precisa receber o tamanho como argumento.
 *   3. v[i] é *(v + i). Sempre foi.
 *
 * E o corolário que responde ao experimento 4 do passo-08: você não pode
 * escrever `a = b` entre vetores porque o nome do vetor não é uma caixa
 * reatribuível — é o endereço do bloco. Copiar é laço ou memcpy.
 *
 * EXPERIMENTE:
 *
 *  1. Dentro de somar, troque o laço por `for (size_t i = 0; i <= n; i++)`.
 *     O ASan pega o mesmo stack-buffer-overflow do passo-09, agora
 *     atravessando uma fronteira de função. Repare que o relatório mostra a
 *     pilha inteira: onde estourou (somar) e quem chamou (main).
 *
 *  2. Tente calcular o tamanho DENTRO de somar com sizeof(v)/sizeof(v[0]).
 *     Dá 2 (8 bytes / 4 bytes). Errado e silencioso. O gcc até avisa
 *     ("sizeof on array function parameter"). Este é um clássico de prova.
 *
 *  3. Chame `somar(notas + 3, 5)`. Você pediu 5 elementos começando no
 *     quarto — só existem 2. ASan pega. Ponteiro deslocado + tamanho errado
 *     é a fonte nº 1 de bugs em divisão de trabalho entre threads.
 *
 *  4. Escreva `void dobrar(int *v, size_t n)` que multiplica cada elemento
 *     por 2, chame em main e imprima o vetor depois. Funciona — a função
 *     alcança as caixas de main, pelo mesmo motivo do passo-07.
 *
 * -> passo-11: strings, que são só vetores de char com uma regra a mais
 * ========================================================================= */
