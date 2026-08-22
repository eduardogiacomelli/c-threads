/* ============================================================================
 * PASSO 15 - a correção do passo-14: memória que você controla.
 *
 * malloc pede um pedaço de memória ao sistema. Esse pedaço não pertence a
 * nenhuma função: ele existe até você devolvê-lo com free. É a única
 * memória em C cuja duração é decisão sua.
 *
 *     Ctrl+Shift+B      (ou: make 15)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* malloc, calloc, free: man 3 malloc */

/* Agora a função devolve um endereço que continua válido depois do return,
 * porque a caixa não está na pilha dela. */
int *fabricar_numero(void)
{
    /* malloc recebe um número de BYTES e devolve o endereço do início do
     * bloco - ou NULL se não conseguiu.
     *
     * Escreva sizeof(int), não 4. O sizeof documenta a intenção e continua
     * certo se o tipo mudar. */
    int *p = malloc(sizeof(int));

    /* Sempre teste o retorno. Na prática malloc quase nunca falha no Linux
     * moderno, mas "quase nunca" com ponteiro é um crash silencioso: sem
     * este if, o *p abaixo escreveria no endereço 0. */
    if (p == NULL) {
        perror("malloc");
        return NULL;
    }

    *p = 42;                 /* escreva ATRAVÉS do ponteiro, como no passo-07 */
    printf("   [fabricar] aloquei em %p e guardei %d\n", (void *) p, *p);
    return p;                /* devolvemos o endereço, e ele continua válido */
}

/* A mesma função do passo-14: ela reaproveita o pedaço de pilha que
 * fabricar_numero deixou. Repare no endereço que ela imprime. */
void outra_funcao(void)
{
    int outro = 777;
    printf("   [outra]    outro = %d, mora em %p\n", outro, (void *) &outro);
}

int main(void)
{
    int *p = fabricar_numero();
    if (p == NULL)
        return 1;

    printf("logo depois:  *p = %d\n", *p);
    outra_funcao();
    printf("depois de outra função rodar:  *p = %d   <- intacto\n", *p);
    printf("^ o endereço do malloc não tem nada a ver com o da pilha.\n");

    /* free devolve o bloco. Depois disso o ponteiro está pendurado de novo:
     * o endereço continua sendo um número válido, e a memória não é mais sua.
     *
     * Hábito que evita o bug: zere o ponteiro logo após liberar. */
    free(p);
    p = NULL;
    printf("\nliberado. p agora é NULL, então um uso acidental estoura na\n"
           "hora em vez de ler lixo.\n");

    /* Um vetor de tamanho decidido em tempo de EXECUÇÃO - coisa que
     * `int v[n]` de tamanho fixo não dá. Aqui está a outra metade do valor
     * do malloc. */
    int quantos = 5;
    int *v = malloc(quantos * sizeof(int));     /* n elementos, não n bytes! */
    if (v == NULL)
        return 1;

    for (int i = 0; i < quantos; i++)
        v[i] = (i + 1) * 10;                    /* indexa igualzinho a vetor */

    printf("\nvetor no heap: ");
    for (int i = 0; i < quantos; i++)
        printf("%d ", v[i]);
    printf("\n");

    /* calloc faz o mesmo e ainda zera tudo. Assinatura diferente:
     * (quantidade, tamanho de cada um). */
    int *zerado = calloc(quantos, sizeof(int));
    printf("calloc já vem zerado: zerado[3] = %d\n", zerado[3]);

    free(v);
    free(zerado);

    /* Se você esquecer um free, o LeakSanitizer avisa quando o programa
     * termina. Experimento 1. */
    return 0;
}

/* ============================================================================
 * PILHA x HEAP, LADO A LADO
 *
 *   PILHA (stack)                      HEAP
 *   int x;  dentro de uma função       malloc
 *   ---------------------------        --------------------------------
 *   o compilador cuida                 você cuida
 *   morre no fim do bloco              morre no free
 *   rápida, automática                 mais lenta, manual
 *   tamanho fixo, decidido ao          tamanho decidido ao rodar
 *     compilar
 *   ~8 MB no total, e acaba            limitada pela RAM
 *
 *   +-------------------+
 *   | main: p [0x5a10] -|---> HEAP: 0x5a10 [ 42 ]
 *   +-------------------+                (não pertence a função nenhuma;
 *   | fabricar_numero   |                 sobrevive ao return)
 *   |   (já foi embora) |
 *   +-------------------+
 *
 * AS QUATRO REGRAS
 *
 *   1. Todo malloc tem exatamente um free. Um só.
 *   2. Teste o retorno de malloc contra NULL.
 *   3. Depois do free, ponha NULL. Usar memória liberada é
 *      "use-after-free", e é bug de segurança, não só de correção.
 *   4. Deixe explícito QUEM libera. "Esta função devolve memória alocada,
 *      o chamador é quem dá free" é comentário obrigatório. Em C isso é
 *      contrato social, o compilador não ajuda.
 *
 * PARA ONDE ISSO VAI, EM PPD
 *
 *   Uma thread só pode receber UM argumento, e ele precisa continuar
 *   existindo enquanto a thread roda - depois que a função que criou a
 *   thread já retornou, possivelmente. É o passo-14 de novo.
 *
 *   A resposta é esta: um malloc por thread, cada uma com o SEU bloco, e o
 *   free feito por quem combinou de fazer.
 *
 * EXPERIMENTE:
 *
 *  1. Comente `free(v);` e rode. O LeakSanitizer imprime no fim:
 *     "Direct leak of 20 byte(s) in 1 object(s)", com a pilha da alocação.
 *     Ele te mostra a LINHA DO MALLOC que vazou.
 *
 *  2. Chame `free(p)` duas vezes. O ASan mata com "attempting double-free"
 *     e mostra as três pilhas: onde alocou, onde liberou, onde tentou de
 *     novo. Sem sanitizer, isso corrompe as estruturas internas do malloc e
 *     quebra bem longe dali.
 *
 *  3. Depois de `free(v)`, leia `v[0]` (tire o NULL do caminho). O ASan diz
 *     "heap-use-after-free". Compare com o passo-14: mesmo bug, outra área
 *     de memória.
 *
 *  4. Erro clássico: escreva `malloc(quantos)` em vez de
 *     `malloc(quantos * sizeof(int))`. Você pediu 5 BYTES e usou 5 INTS =
 *     20 bytes. ASan: "heap-buffer-overflow". Sempre multiplique pelo
 *     sizeof.
 *
 *  5. Devolva um VETOR alocado: `int *criar_vetor(int n)` que faz o malloc,
 *     preenche e retorna. Chame de main, use, dê free em main. Escreva no
 *     comentário da função quem libera.
 *
 * -> passo-16: structs, o último tijolo antes das threads
 * ========================================================================= */
