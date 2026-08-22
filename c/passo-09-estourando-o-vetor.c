/* ============================================================================
 * PASSO 9 - ERRADO DE PROPÓSITO. Escrever fora do vetor.
 *
 * Este é o bug que o C é famoso por deixar passar. O programa compila limpo,
 * roda, e o estrago pode aparecer numa variável que você nem tocou.
 *
 *     Ctrl+Shift+B      (ou: make 09)
 *
 * O AddressSanitizer vai MATAR o programa na primeira linha errada. Leia a
 * mensagem inteira dele - ela é longa e ela é o conteúdo desta aula.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int antes  = 111;
    int v[5]   = {0, 0, 0, 0, 0};
    int depois = 999;

    printf("antes=%d  depois=%d\n", antes, depois);
    printf("v tem 5 caixas: índices válidos de 0 a 4\n\n");

    /* O BUG ESTÁ NO <=.
     *
     * Com i <= 5, o laço roda seis vezes: 0,1,2,3,4 e o 5. Mas v[5] não
     * existe - o vetor acaba no 4. A escrita cai nos 4 bytes seguintes ao
     * vetor, que pertencem a outra coisa.
     *
     * C não confere índice. Nunca. Não existe IndexError. O endereço é
     * calculado (início + 5*4) e a escrita simplesmente acontece. */
    for (int i = 0; i <= 5; i++) {
        printf("escrevendo 7 em v[%d]\n", i);
        v[i] = 7;
    }

    /* Se você chegou aqui, é porque compilou SEM sanitizer. Repare no que
     * pode ter acontecido com as variáveis vizinhas: */
    printf("\nv[0]=%d v[4]=%d\n", v[0], v[4]);
    printf("antes=%d  depois=%d\n", antes, depois);
    printf("uma delas pode ter virado 7 sem você encostar nela.\n");

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 *     0x7ffd1000  [ 7 ]   v[0]
 *     0x7ffd1004  [ 7 ]   v[1]
 *     0x7ffd1008  [ 7 ]   v[2]
 *     0x7ffd100c  [ 7 ]   v[3]
 *     0x7ffd1010  [ 7 ]   v[4]     <- fim do vetor
 *     0x7ffd1014  [ 7 ]   <- v[5]: NÃO É SEU. Escrevemos aqui mesmo assim.
 *
 * O que mora em 0x7ffd1014 depende de como o compilador arrumou a pilha:
 * pode ser `antes`, pode ser `depois`, pode ser espaço de alinhamento sem
 * uso, pode ser o endereço de retorno da função. Muda com o nível de
 * otimização, com a versão do gcc, com a ordem das declarações.
 *
 * Por isso este bug é tão caro: ele não falha onde está. Ele corrompe algo,
 * e o programa quebra três funções depois, num lugar que está correto.
 *
 * O QUE OS SANITIZERS FIZERAM
 *
 * Você recebeu DUAS reclamações, de duas ferramentas diferentes. Leia na
 * ordem em que saíram.
 *
 * Primeiro o UBSan, que sabe o TIPO da variável e vê o índice inválido:
 *
 *     runtime error: index 5 out of bounds for type 'int [5]'
 *
 * Depois o ASan, que não olha tipos: ele põe "zonas vermelhas" (redzones)
 * em volta de cada vetor e vigia todo acesso à memória:
 *
 *     ERROR: AddressSanitizer: stack-buffer-overflow
 *     WRITE of size 4 at 0x...
 *         #0 in main passo-09-estourando-o-vetor.c:34
 *     Address ... is located in stack of thread T0 at offset 52 in frame
 *       This frame has 1 object(s):
 *         [32, 52) 'v' (line 18) <== Memory access at offset 52 overflows
 *                                    this variable
 *
 * Traduzindo a última linha: o vetor `v` ocupa da posição 32 até a 52 da
 * pilha (20 bytes, os 5 ints), e você escreveu exatamente na 52 - o
 * primeiro byte depois do fim. Um passo além, e o ASan viu.
 *
 * Repare também no que NÃO apareceu: os printf antes do erro. O ASan aborta
 * o processo sem esvaziar o buffer do stdout. Se um dia seus printf de
 * depuração "sumirem" num crash, é isso - não é que a linha não rodou.
 *
 * EXPERIMENTE:
 *
 *  1. Conserte: troque `i <= 5` por `i < 5`. Melhor ainda, use o
 *     sizeof do passo-08 pra não repetir o número 5 em dois lugares -
 *     números repetidos no código é como off-by-one nasce.
 *
 *  2. Veja a versão SEM rede. No terminal:
 *
 *         gcc -std=gnu17 -Wall -g passo-09-estourando-o-vetor.c -o /tmp/sem
 *         /tmp/sem
 *
 *     Sem sanitizer o programa provavelmente termina "normalmente", e uma
 *     das variáveis vizinhas está corrompida. Compare as duas saídas. É esta
 *     diferença que justifica manter o sanitizer sempre ligado.
 *
 *  3. Troque `v[i] = 7` por `v[i + 1000] = 7`. Agora você está longe do
 *     vetor. Rode com e sem sanitizer: sem ele, é Segmentation fault seco.
 *
 *  4. Só LEIA fora do vetor (`printf("%d", v[5]);`). O ASan pega igual:
 *     "READ of size 4". Ler fora também é bug, mesmo sem estragar nada.
 *
 * -> passo-10: por que vetor e ponteiro são quase a mesma coisa
 * ========================================================================= */
