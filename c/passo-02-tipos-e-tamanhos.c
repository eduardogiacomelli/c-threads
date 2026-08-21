/* ============================================================================
 * PASSO 2 — todo valor tem um tamanho fixo em bytes.
 *
 * Em Python, um int cresce até onde a memória aguentar. Em C, um int é uma
 * caixa de 4 bytes, e o que não couber é perdido. Essa única frase explica
 * metade dos bugs de C.
 *
 *     Ctrl+Shift+B      (ou: make 02)
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* Cada declaração aqui reserva um espaço de tamanho conhecido na memória
     * e escreve um valor dentro. O tipo diz DUAS coisas ao compilador:
     * quantos bytes reservar, e como interpretar esses bytes. */

    char   letra   = 'A';       /* 1 byte.  Aspas SIMPLES = um caractere.     */
    int    idade   = 25;        /* 4 bytes. O inteiro padrão.                 */
    long   grande  = 25L;       /* 8 bytes no Linux 64 bits.                  */
    float  altura  = 1.75f;     /* 4 bytes. Precisão ruim, evite.             */
    double preciso = 1.75;      /* 8 bytes. Use este pra número com vírgula.  */

    /* sizeof é um OPERADOR, não função — o compilador resolve na hora de
     * compilar e escreve o número direto no binário. Nada é calculado ao
     * rodar. Ele devolve um tipo especial, size_t, cujo % é "%zu". */
    printf("char   ocupa %zu byte  e vale %c\n",  sizeof(char),   letra);
    printf("int    ocupa %zu bytes e vale %d\n",  sizeof(int),    idade);
    printf("long   ocupa %zu bytes e vale %ld\n", sizeof(long),   grande);
    printf("float  ocupa %zu bytes e vale %f\n",  sizeof(float),  altura);
    printf("double ocupa %zu bytes e vale %f\n",  sizeof(double), preciso);

    /* 'A' é literalmente o número 65. Um char É um inteiro pequeno — a única
     * diferença é como o printf resolve mostrá-lo. Mesma caixa, duas leituras. */
    printf("\n'A' como caractere: %c | como número: %d\n", letra, letra);
    printf("'A' + 1 = %c\n", letra + 1);

    /* O limite da caixa de 4 bytes: um int vai até 2147483647.
     * <limits.h> tem essas constantes; aqui escrevi o número pra você ver. */
    printf("\nmaior int possível: %d\n", 2147483647);

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 * A memória é uma fita de bytes. Declarar uma variável é reservar um pedaço
 * dessa fita e dar um nome a ele:
 *
 *     letra   [A]                        1 byte
 *     idade   [25][ 0][ 0][ 0]           4 bytes
 *     grande  [25][ 0][ 0][ 0][0][0][0][0]  8 bytes
 *
 * O nome existe só pra você e pro compilador. No binário só sobram endereços.
 *
 * EXPERIMENTE:
 *
 *  1. Ponha `int enorme = 2147483647 + 1;` e imprima com %d.
 *     O gcc avisa em tempo de compilação ("integer overflow"), e o valor vira
 *     negativo: a caixa encheu e o bit que sobrou caiu no bit de sinal.
 *     Em Python isso simplesmente não acontece.
 *
 *  2. Faça o overflow acontecer só ao rodar, pra escapar do aviso:
 *
 *         int x = 2147483647;
 *         x = x + 1;
 *         printf("%d\n", x);
 *
 *     Agora o UndefinedBehaviorSanitizer te pega ao rodar, com arquivo e
 *     linha: "signed integer overflow". É pra isso que ele está ligado.
 *
 *  3. Troque `char letra = 'A'` por aspas duplas: "A". O gcc reclama.
 *     Aspas simples = um caractere (1 byte). Aspas duplas = uma string
 *     (2 bytes aqui: o 'A' e o terminador). São coisas diferentes — passo-11.
 *
 *  4. Imprima sizeof de um ponteiro: `printf("%zu\n", sizeof(int *));`
 *     Dá 8 em qualquer máquina 64 bits, seja o ponteiro pra que for. Um
 *     endereço é um endereço. Guarde isso pro passo-05.
 *
 * -> passo-03 (que é o passo-02 feito errado, de propósito)
 * ========================================================================= */
