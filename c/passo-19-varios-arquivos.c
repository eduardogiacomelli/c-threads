/* ============================================================================
 * PASSO 19 - quando um arquivo só não basta: header + implementação.
 *
 * Este passo tem TRÊS arquivos: contas.h, contas.c e este aqui.
 *
 *     make 19
 *
 * ATENÇÃO: o Ctrl+Shift+B NÃO funciona aqui, e o erro que ele dá é o
 * conteúdo da aula. Aperte assim mesmo, leia o erro, e depois volte.
 * ========================================================================= */

#include <stdio.h>
#include "contas.h"     /* aspas = arquivo meu; < > = do sistema */

int main(void)
{
    /* Este arquivo NÃO sabe como soma_ate funciona. Ele só viu a assinatura
     * no header: recebe int, devolve long. É o suficiente pra compilar.
     *
     * Essa é a ideia toda: separar "o que dá pra usar" de "como está feito".
     * Você faz isso desde sempre com printf - nunca leu o código dele. */
    printf("soma_ate(10)   = %ld\n", soma_ate(10));
    printf("soma_ate(100)  = %ld\n", soma_ate(100));
    printf("eh_par(7)      = %d\n",  eh_par(7));

    /* O estado privado do módulo, acessível só pela função que ele expõe. */
    printf("\nsoma_ate foi chamada %d vezes\n", quantas_chamadas());

    /* Tente `chamadas++` aqui. Não compila: aquela variável é `static` no
     * contas.c, então este arquivo nem sabe que ela existe. */

    return 0;
}

/* ============================================================================
 * AS DUAS ETAPAS, AGORA VISÍVEIS
 *
 * No passo-01 eu disse que compilar tem etapas separadas. Com um arquivo só
 * isso é invisível. Com dois, não:
 *
 *     COMPILAR (cada .c vira um .o, separadamente e sem saber do outro)
 *
 *       passo-19-varios-arquivos.c --> passo-19.o
 *          "existe soma_ate, devolve long. Vou anotar uma chamada pendente
 *           pra ela e deixar o endereço em branco."
 *
 *       contas.c --> contas.o
 *          "aqui está o código de soma_ate, neste offset."
 *
 *     LINKAR (junta os .o e resolve os pendentes)
 *
 *       passo-19.o + contas.o --> executável
 *          "a chamada pendente aponta pro código que veio do contas.o."
 *
 * Daí os dois erros mais confusos do C, que agora têm endereço certo:
 *
 *   "implicit declaration of function 'soma_ate'"
 *        -> erro de COMPILAÇÃO. Faltou o #include: este arquivo nunca viu a
 *           assinatura.
 *
 *   "undefined reference to 'soma_ate'"
 *        -> erro de LINKAGEM. A assinatura ele viu; o código não. Você
 *           esqueceu de passar contas.c (ou contas.o) no comando.
 *
 * Decore a diferença. "undefined reference" nunca se resolve com #include.
 *
 * NA MÃO, PRA VER ACONTECENDO
 *
 *     # os dois de uma vez (o que o make faz):
 *     gcc -std=gnu17 -Wall -Wextra -g passo-19-varios-arquivos.c contas.c -o prog
 *
 *     # ou em duas etapas, que é o que projetos de verdade fazem:
 *     gcc -c contas.c                     # -c = só compile, não linke
 *     gcc -c passo-19-varios-arquivos.c
 *     gcc passo-19-varios-arquivos.o contas.o -o prog
 *
 * A vantagem da segunda: mudou só um .c, recompila só ele. É pra isso que
 * existe o Makefile.
 *
 * E COM PTHREADS
 *
 *     gcc ... programa.c -o programa -pthread
 *
 * A flag `-pthread` (sem o "l") faz as duas coisas: liga a biblioteca no
 * linker e ajusta o compilador. Esquecer dela dá exatamente
 * "undefined reference to 'pthread_create'" - que agora você sabe ler: a
 * declaração veio do #include <pthread.h>, o código não veio de lugar nenhum.
 *
 * QUANDO SEPARAR EM ARQUIVOS
 *
 * Nos exercícios da lista de pthreads, quase nunca: um programa de 120 linhas
 * cabe num arquivo só, e o professor pediu um programa, não uma biblioteca.
 * Vale a pena quando você tem uma função que quer reaproveitar em vários
 * exercícios - a de medir tempo, por exemplo, que aparece no 7 e nos dois
 * desafios.
 *
 * O que você precisa daqui, mesmo, é saber LER os erros de linkagem. Eles vão
 * aparecer.
 *
 * EXPERIMENTE:
 *
 *  1. Aperte Ctrl+Shift+B neste arquivo. Ele compila só ${file} e você recebe
 *     "undefined reference to `soma_ate'". Confirme no painel que o erro é do
 *     linker (a mensagem vem do `ld` ou do `collect2`), não do compilador.
 *
 *  2. Apague o `#include "contas.h"` e rode `make 19`. Agora é o outro erro:
 *     "implicit declaration". Os dois erros, o mesmo programa, causas
 *     opostas.
 *
 *  3. Tire as guardas (#ifndef/#define/#endif) do contas.h e inclua o header
 *     duas vezes seguidas neste arquivo. Com só declarações de função o gcc
 *     tolera; agora ponha um `typedef struct { int x; } Ponto;` no header e
 *     repita: "redefinition of 'Ponto'". É pra isso que a guarda existe.
 *
 *  4. Mude o tipo de retorno de soma_ate no contas.c (de long pra int) e
 *     deixe o header como está. `make 19` acusa o conflito, porque contas.c
 *     inclui o próprio header. Foi por isso que ele o incluiu.
 *
 * -> Fim dos fundamentos. Volte ao "00 - COMECE AQUI.md" e siga pra lista de
 *    pthreads.
 * ========================================================================= */
