/* ============================================================================
 * PASSO 14 — ERRADO DE PROPÓSITO. Guardar o endereço de uma variável local.
 *
 * Até aqui você viu ONDE as caixas ficam. Este passo é sobre POR QUANTO
 * TEMPO elas existem. É a ideia que faltava, e é a que mais dá trabalho em
 * programas com threads.
 *
 *     Ctrl+Shift+B      (ou: make 14)
 *
 * O ASan mata o programa logo na primeira leitura. O experimento 1 mostra o
 * que teria acontecido sem ele — e é aí que o bug fica assustador.
 * ========================================================================= */

#include <stdio.h>

/* Um ponteiro global, pra guardar um endereço entre chamadas. */
int *guardado;

/* Toda variável local nasce na PILHA (stack) quando a função começa e some
 * quando a função retorna. "Some" não significa apagada: significa que aquele
 * pedaço de memória volta a estar disponível, e a próxima função chamada vai
 * usar o mesmo espaço pras variáveis dela.
 *
 * Guardar &numero é guardar o endereço de uma caixa que está sendo desmontada
 * nesse exato momento. */
void fabricar_numero(void)
{
    int numero = 42;

    printf("   [fabricar]  numero = %d, mora em %p\n",
           numero, (void *) &numero);

    guardado = &numero;      /* <- O BUG. Anotamos um endereço que vai expirar
                              *    daqui a uma linha.
                              *
                              *    O gcc já reclamou disto no painel:
                              *    "storing the address of local variable
                              *    'numero' in 'guardado' [-Wdangling-pointer]".
                              *    Ele viu antes de você rodar. */
}

/* Esta função não tem nada a ver com a de cima. Ela só é chamada depois —
 * e por isso ganha o mesmo pedaço de pilha, com uma variável dela. */
void outra_funcao(void)
{
    int outro = 777;
    printf("   [outra]     outro  = %d, mora em %p\n",
           outro, (void *) &outro);
}

int main(void)
{
    fabricar_numero();

    /* Pode ser que imprima 42. ISSO É O PIOR CASO POSSÍVEL: o valor velho
     * ainda está lá porque ninguém pisou em cima ainda. O bug fica invisível
     * até o dia em que alguém pisa. */
    printf("logo depois:  *guardado = %d\n", *guardado);

    outra_funcao();

    printf("depois de outra função rodar:  *guardado = %d\n", *guardado);
    printf("^ compare os dois endereços impressos acima. São o MESMO.\n");

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 * A pilha é reaproveitada o tempo todo:
 *
 *   1) main chama fabricar_numero. A pilha cresce:
 *
 *        [ main ........................ ]
 *        [ fabricar_numero: numero = 42  ]  <- 0x7fff...bf4
 *
 *   2) fabricar_numero retorna. Aquele quadro é abandonado — mas o número
 *      0x7fff...bf4 que anotamos continua sendo um endereço:
 *
 *        [ main ........................ ]
 *        [ ...disponível, ainda com 42.. ]  <- 0x7fff...bf4
 *
 *   3) main chama outra_funcao, que recebe exatamente o mesmo espaço:
 *
 *        [ main ........................ ]
 *        [ outra_funcao: outro = 777     ]  <- 0x7fff...bf4, agora 777
 *
 *   4) *guardado lê 0x7fff...bf4 e encontra o que a outra função deixou.
 *
 * O ponteiro nunca "soube" que ficou inválido. Um ponteiro é só um número.
 * Ele não tem como saber que a caixa dele foi embora — quem tem que saber é
 * você. O nome disto é DANGLING POINTER (ponteiro pendurado).
 *
 * O QUE O ASan DISSE
 *
 *     ERROR: AddressSanitizer: stack-use-after-return
 *     READ of size 4 at 0x...
 *         #0 in main passo-14-devolvendo-lixo.c:LINHA
 *     Address is located in stack of thread T0 in frame
 *         #0 in fabricar_numero
 *       This frame has 1 object(s):
 *         [32, 36) 'numero' <== Memory access is inside this variable
 *
 * Leia a parte importante: o endereço que main leu pertence ao QUADRO DE
 * OUTRA FUNÇÃO, uma que já retornou. O ASan guarda esse mapa justamente pra
 * conseguir te dizer isso.
 *
 * AS TRÊS DURAÇÕES DE VIDA EM C
 *
 *   automática (pilha)   int x;            morre no fim do bloco { }
 *   estática             static int x;     vive o programa inteiro
 *   alocada (heap)       malloc            vive até VOCÊ chamar free
 *
 * Precisa que sobreviva ao return? Só as duas últimas servem. Passo-15.
 *
 * POR QUE ISSO IMPORTA DEMAIS COM THREADS
 *
 * Troque "a função retornou" por "a thread ainda está rodando" e é o mesmo
 * bug, muito mais difícil de ver:
 *
 *     for (int i = 0; i < 4; i++)
 *         pthread_create(&t[i], NULL, funcao, &i);   // &i: a caixa de main
 *
 * A thread vai ler aquele endereço depois, quando main já mudou o `i` — ou
 * já saiu do laço onde ele existia. Este é o erro nº 1 do trabalho de PPD, e
 * você acabou de ver a mecânica dele sem thread nenhuma envolvida.
 *
 * EXPERIMENTE:
 *
 *  1. O MAIS IMPORTANTE DOS EXPERIMENTOS DESTE ARQUIVO. Compile sem
 *     sanitizer, no terminal:
 *
 *         gcc -std=gnu17 -Wall -g passo-14-devolvendo-lixo.c -o /tmp/s14
 *         /tmp/s14
 *
 *     Nada de erro. O programa roda até o fim, imprime 42 e depois 777, e os
 *     dois endereços impressos são idênticos. Um programa em produção faria
 *     isso em silêncio, com o valor errado, por anos.
 *
 *  2. Tente a versão que todo mundo escreve primeiro: transforme em
 *     `int *fabricar_numero(void)` com `return &numero;`. Duas coisas
 *     acontecem, e as duas valem a pena ver:
 *       - o gcc avisa: "function returns address of local variable";
 *       - e ele SUBSTITUI o retorno por NULL, de propósito, pra você bater
 *         de cara. O programa morre com "load of null pointer".
 *     O compilador conhece esse erro tão bem que sabota a sua versão dele.
 *
 *  3. Troque `int numero = 42;` por `static int numero = 42;` e rode.
 *     Funciona, e continua funcionando depois de outra_funcao — a variável
 *     saiu da pilha e foi pra área estática. Agora pense: e se duas threads
 *     chamarem essa função ao mesmo tempo? Elas compartilham a MESMA caixa.
 *     Trocamos um bug por outro. É por isso que a resposta certa é o
 *     passo-15.
 *
 * -> passo-15, malloc e free
 * ========================================================================= */
