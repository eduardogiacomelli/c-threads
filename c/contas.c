/* ============================================================================
 * contas.c — a IMPLEMENTAÇÃO. Aqui mora o código de verdade.
 * ========================================================================= */

/* Um módulo inclui o próprio header. Parece redundante, mas serve pra que o
 * compilador confira se a implementação bate com o que foi prometido: se
 * você mudar o tipo de retorno aqui e esquecer lá, ele acusa na hora. */
#include "contas.h"
/* Aspas, não < >:
 *     "contas.h"   procura primeiro na pasta deste arquivo  -> seu código
 *     <stdio.h>    procura nas pastas do sistema            -> biblioteca
 */

/* `static` numa variável GLOBAL significa "visível só neste arquivo". Nada a
 * ver com o static de dentro de função (passo-14). É assim que um módulo
 * guarda estado sem expô-lo: nenhum outro arquivo consegue mexer neste
 * contador, porque nenhum outro arquivo sabe que ele existe. */
static int chamadas = 0;

long soma_ate(int n)
{
    chamadas++;

    if (n < 1)
        return 0;

    long total = 0;
    for (int i = 1; i <= n; i++)
        total += i;
    return total;
}

int eh_par(int n)
{
    return n % 2 == 0;
}

int quantas_chamadas(void)
{
    return chamadas;
}

/* `static` também funciona em função: esta existe só aqui dentro, não está
 * no header, e nenhum outro arquivo consegue chamá-la. É o "privado" do C.
 * Se ninguém usar, o gcc avisa que ela está sobrando — o que é útil. */
