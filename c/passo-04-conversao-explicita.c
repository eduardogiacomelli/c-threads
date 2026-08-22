/* ============================================================================
 * PASSO 4 - a correção do passo-03: cast e o molde certo.
 *
 *     Ctrl+Shift+B      (ou: make 04)
 *
 * Compare com o passo-03 lado a lado. As mudanças são pequenas.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int total = 7;
    int gente = 2;

    /* CORREÇÃO 1 - (double) é um CAST: "trate este valor como double
     * agora, nesta expressão". Ele não muda a variável `total`, que segue
     * int; produz um valor double temporário só para esta conta.
     *
     * Com um operando double, C promove o outro automaticamente, e a divisão
     * vira divisão de ponto flutuante. Basta converter UM dos dois. */
    double media = (double) total / gente;
    printf("média de %d entre %d pessoas: %.2f\n", total, gente, media);

    /* %.2f = ponto flutuante com 2 casas depois da vírgula. O ".2" é
     * arredondamento só na EXIBIÇÃO; o valor guardado continua inteiro de
     * precisão. */

    /* CORREÇÃO 2 - o molde tem que combinar com o que você passa.
     * Decore estes cinco, que são 95% do uso:
     *
     *     %d   int
     *     %ld  long
     *     %f   double  (e float também: float vira double ao ser passado)
     *     %c   char
     *     %s   string (endereço de char, veja passo-11)
     *     %p   um endereço qualquer (passo-05)
     *     %zu  size_t, o que sizeof devolve
     */
    double preco = 19.9;
    int quantidade = 3;
    printf("preço: %.2f   quantidade: %d\n", preco, quantidade);

    /* Precisa mesmo imprimir um double como inteiro? Diga isso explicitamente,
     * com um cast. Aí não é lixo, é truncamento - e está escrito no código
     * que foi de propósito. 19.9 vira 19 (corta, não arredonda). */
    printf("preço truncado: %d\n", (int) preco);

    /* CORREÇÃO 3 - escreva a constante como double e a expressão inteira
     * vira double. 9.0/5 é 1.8. Nenhum cast necessário: o `.0` já resolve. */
    int celsius = 100;
    double fahrenheit = celsius * (9.0 / 5.0) + 32;
    printf("\n100 °C em °F: %.1f\n", fahrenheit);

    /* Compile este arquivo e olhe o painel: zero warnings. É esse o alvo. */
    return 0;
}

/* ============================================================================
 * A REGRA, EM UMA FRASE
 *
 *   Se você quer resultado com vírgula, PELO MENOS UM operando precisa ter
 *   vírgula - por cast `(double)x` ou por literal `9.0`.
 *
 * E a segunda:
 *
 *   O molde do printf é uma promessa. Quebrou a promessa, leu lixo.
 *
 * EXPERIMENTE:
 *
 *  1. Troque `(double) total / gente` por `(double) (total / gente)`.
 *     Volta a dar 3.00. Por quê? O parêntese faz a divisão inteira acontecer
 *     PRIMEIRO; o cast só converte o 3 que já se perdeu. O lugar do cast
 *     importa mais que a presença dele.
 *
 *  2. Imprima `(int) -19.9`. Dá -19, não -20. Cast pra int corta em direção
 *     ao zero. Pra arredondar de verdade existe round(), em <math.h> - e aí
 *     você precisa compilar com -lm.
 *
 *  3. Divida por zero com inteiros: `total / 0` com uma variável zerada
 *     (`int z = 0; total / z;`). O UBSan mata o programa com
 *     "division by zero". Agora faça o mesmo com double: `1.0 / 0.0`.
 *     Imprime `inf` e segue em frente. Regras completamente diferentes para
 *     inteiro e ponto flutuante.
 *
 * -> passo-05: ponteiros, do começo
 * ========================================================================= */
