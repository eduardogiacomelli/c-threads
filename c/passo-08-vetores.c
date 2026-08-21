/* ============================================================================
 * PASSO 8 — vetor não é lista.
 *
 * Um vetor em C é um bloco contíguo de caixas do mesmo tipo, com tamanho
 * decidido na hora de compilar. Não cresce, não sabe o próprio tamanho, não
 * tem .append, e ninguém confere se você saiu dele.
 *
 *     Ctrl+Shift+B      (ou: make 08)
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* 5 caixas de int, GRUDADAS na memória, nesta ordem.
     * O nome `notas` se refere ao bloco inteiro. */
    int notas[5] = {7, 8, 10, 6, 9};

    /* Índices começam em 0, então o último é 4, nunca 5. */
    printf("primeira nota: %d\n", notas[0]);
    printf("última nota:   %d\n", notas[4]);

    /* O vetor não sabe que tem 5 elementos. Você é que sabe.
     * Truque que funciona SÓ AQUI, onde o vetor foi declarado:
     *
     *     sizeof(notas)     = 20 bytes (o bloco inteiro)
     *     sizeof(notas[0])  = 4 bytes  (uma caixa)
     *     20 / 4            = 5
     *
     * Guarde a ressalva "só aqui" — o passo-10 mostra onde isso quebra. */
    size_t tamanho = sizeof(notas) / sizeof(notas[0]);
    printf("\nsizeof(notas) = %zu bytes, cada int = %zu -> %zu elementos\n",
           sizeof(notas), sizeof(notas[0]), tamanho);

    /* O laço clássico de C. Repare: i < tamanho, com <, nunca <=.
     * Trocar isso por <= é o off-by-one, e é o assunto do passo-09. */
    printf("\ntodas as notas:\n");
    for (size_t i = 0; i < tamanho; i++)
        printf("  notas[%zu] = %d\n", i, notas[i]);

    /* Somando. Nada de sum(): você escreve o laço. */
    int soma = 0;
    for (size_t i = 0; i < tamanho; i++)
        soma += notas[i];

    printf("\nsoma = %d, média = %.2f\n", soma, (double) soma / tamanho);
    /* aquele (double) do passo-04 de novo — sem ele a média seria inteira */

    /* Os endereços mostram que o bloco é contíguo: cada int avança 4 bytes. */
    printf("\nonde cada caixa mora:\n");
    for (size_t i = 0; i < tamanho; i++)
        printf("  notas[%zu] em %p\n", i, (void *) &notas[i]);

    /* Inicializar com menos valores que o tamanho: o resto vira zero.
     * Isto é o jeito idiomático de zerar um vetor inteiro. */
    int zerado[5] = {0};
    printf("\nzerado[3] = %d (o {0} preencheu tudo com zero)\n", zerado[3]);

    /* CUIDADO: sem NENHUM inicializador, um vetor local contém LIXO — o que
     * quer que estivesse naquele pedaço de memória antes. Não é zero.
     *     int lixo[5];          <- os 5 valores são imprevisíveis
     */

    return 0;
}

/* ============================================================================
 * O DIAGRAMA
 *
 *     endereço      conteúdo     nome
 *     0x7ffd1000    [  7 ]       notas[0]
 *     0x7ffd1004    [  8 ]       notas[1]
 *     0x7ffd1008    [ 10 ]       notas[2]
 *     0x7ffd100c    [  6 ]       notas[3]
 *     0x7ffd1010    [  9 ]       notas[4]
 *     0x7ffd1014    [ ??? ]      <- fora do vetor. Existe memória aqui, e ela
 *                                   pertence a outra coisa.
 *
 * Cada índice avança 4 bytes porque cada int ocupa 4. O endereço de
 * notas[i] é simplesmente:  início + i * sizeof(int).
 *
 * É uma multiplicação, não uma busca. Por isso acessar notas[9999] é tão
 * rápido quanto notas[0] — e por isso ninguém percebe que é inválido.
 *
 * EXPERIMENTE:
 *
 *  1. Confira a conta acima na saída real: subtraia os endereços impressos.
 *     A diferença é 4 (em hexadecimal: 1000, 1004, 1008...).
 *
 *  2. Declare `int lixo[5];` sem inicializar e imprima os 5. Rode duas vezes.
 *     Em Python isso é impossível; aqui é rotina.
 *
 *  3. Mude para `int notas[5] = {7, 8};`. Imprima todos. Os três últimos são
 *     zero — mas só porque houve um inicializador. Sem `= {...}`, lixo.
 *
 *  4. Tente `notas = zerado;`. Erro de compilação. O nome de um vetor não é
 *     uma variável que você possa reatribuir. Copiar vetor em C é laço ou
 *     memcpy, nunca `=`. Passo-10 explica por quê.
 *
 * -> passo-09, onde a gente sai do vetor de propósito
 * ========================================================================= */
