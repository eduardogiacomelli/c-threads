/* ============================================================================
 * PASSO 6 - ERRADO DE PROPÓSITO. A função que troca dois valores... e não troca.
 *
 * Rode primeiro. O programa não quebra, não avisa nada, o compilador fica
 * calado. Ele só não faz o que diz que faz.
 *
 *     Ctrl+Shift+B      (ou: make 06)
 * ========================================================================= */

#include <stdio.h>

/* A ideia é óbvia e o código parece certo. */
void trocar(int a, int b)
{
    printf("   [dentro] recebi a=%d b=%d\n", a, b);

    int temporario = a;
    a = b;
    b = temporario;

    printf("   [dentro] agora a=%d b=%d - trocou aqui dentro!\n", a, b);
}

int main(void)
{
    int x = 10;
    int y = 20;

    printf("antes:  x=%d y=%d\n", x, y);
    trocar(x, y);
    printf("depois: x=%d y=%d   <- não mudou nada\n", x, y);

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 * C SEMPRE passa argumentos por CÓPIA. Sem exceção, para qualquer tipo.
 *
 * Quando main chama trocar(x, y), o que a função recebe são caixas NOVAS,
 * com o mesmo conteúdo:
 *
 *     main:                      trocar:
 *     0x7ffd1000 [ 10 ]  x       0x7ffd0900 [ 10 ]  a   <- cópia de x
 *     0x7ffd1004 [ 20 ]  y       0x7ffd0904 [ 20 ]  b   <- cópia de y
 *
 * A função troca as CÓPIAS dela:
 *
 *     main:                      trocar:
 *     0x7ffd1000 [ 10 ]  x       0x7ffd0900 [ 20 ]  a
 *     0x7ffd1004 [ 20 ]  y       0x7ffd0904 [ 10 ]  b
 *
 * e quando a função termina, as caixas `a` e `b` deixam de existir. O
 * trabalho todo é jogado fora. `x` e `y` nunca souberam de nada.
 *
 * "Mas em Python uma função consegue mudar minha lista!" - consegue, e é a
 * mesma regra: Python também copia o ARGUMENTO, que no caso de uma lista é
 * uma referência. A cópia da referência aponta pro mesmo objeto. C não tem
 * referência automática: se você quer que a função alcance sua caixa, você
 * entrega o endereço dela na mão.
 *
 * A pergunta que resolve isso, e que vale pra vida inteira em C:
 *
 *     "A função precisa MUDAR algo meu, ou só LER?"
 *
 *     só ler   -> passe o valor
 *     mudar    -> passe o endereço
 *
 * EXPERIMENTE:
 *
 *  1. Imprima os endereços dos dois lados e veja que são caixas diferentes:
 *
 *         dentro de trocar:  printf("   [dentro] &a=%p\n", (void *) &a);
 *         dentro de main:    printf("&x=%p\n", (void *) &x);
 *
 *     Endereços diferentes = caixas diferentes = nada do que a função faz
 *     alcança as suas.
 *
 *  2. Faça trocar() devolver algo (`return a;`). Você consegue devolver UM
 *     valor. Precisa mudar dois. É por isso que ponteiro não é opcional aqui.
 *
 * -> passo-07, a correção
 * ========================================================================= */
