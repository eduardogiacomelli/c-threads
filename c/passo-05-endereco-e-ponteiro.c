/* ============================================================================
 * PASSO 5 — & e *, com o diagrama do lado.
 *
 * Nada aqui é difícil. É só notação nova para uma ideia que você já usa em
 * Python sem ver: toda variável mora em algum lugar, e esse lugar tem número.
 *
 *     Ctrl+Shift+B      (ou: make 05)
 *
 * Este passo não quebra nada. Ele é a base dos passos 06 até 16.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    /* Uma variável é uma CAIXA com um endereço.
     *
     *     endereço            conteúdo
     *     0x7ffd1234    ->    [ 25 ]      <- a variável `idade`
     *
     * Duas coisas separadas, e a confusão inteira de ponteiros vem de
     * misturá-las: o ENDEREÇO (onde a caixa está) e o VALOR (o que tem
     * dentro dela). */
    int idade = 25;

    /* & = "me dê o endereço de". %p imprime endereço, em hexadecimal.
     * O (void *) no cast é o que o %p espera formalmente. */
    printf("idade  vale     %d\n",  idade);
    printf("idade  mora em  %p\n", (void *) &idade);

    /* Um PONTEIRO é uma caixa como qualquer outra. O que ela guarda por
     * acaso é um endereço.
     *
     *     0x7ffd1234    ->  [ 25 ]              <- idade
     *     0x7ffd9999    ->  [ 0x7ffd1234 ]      <- p, guarda o ENDEREÇO
     *                                              de idade
     *
     * Leia `int *p` como: "p é do tipo 'endereço de int'". */
    int *p = &idade;

    printf("\np      vale     %p   <- é o endereço de idade\n", (void *) p);
    printf("p      mora em  %p   <- p também é uma caixa\n",   (void *) &p);

    /* * na frente de um ponteiro = "vá até esse endereço e leia o que tem lá".
     * A palavra pra isso é DESREFERENCIAR.
     *
     * Cuidado com a sobrecarga do símbolo:
     *     int *p        na DECLARAÇÃO -> "p é um ponteiro"
     *     *p            no USO        -> "o conteúdo apontado por p"
     * Mesmo caractere, dois significados. */
    printf("*p     vale     %d   <- o que tem no endereço guardado em p\n", *p);

    /* Escrever através do ponteiro altera a caixa original. Não existe cópia
     * envolvida: p aponta pra caixa da idade, e é nela que gravamos. */
    *p = 30;
    printf("\ndepois de *p = 30, idade agora é %d\n", idade);

    /* Aquilo que o passo-02 antecipou: o tamanho do ponteiro não depende do
     * que ele aponta. Endereço é endereço — 8 bytes numa máquina 64 bits. */
    double d = 3.14;
    char   c = 'x';
    printf("\nsizeof(int *)    = %zu\n", sizeof(int *));
    printf("sizeof(double *) = %zu\n", sizeof(double *));
    printf("sizeof(char *)   = %zu\n", sizeof(char *));

    /* Então pra que serve o tipo do ponteiro, se todos têm 8 bytes?
     * Pra saber QUANTOS bytes ler no *p, e como interpretá-los.
     * `int *` lê 4 bytes como inteiro. `double *` lê 8 como ponto flutuante.
     * O tipo é uma instrução de leitura pro compilador, não algo guardado
     * na memória. */
    double *pd = &d;
    char   *pc = &c;
    printf("*pd = %.2f   *pc = %c\n", *pd, *pc);

    /* Um ponteiro que não aponta pra nada tem um valor combinado: NULL.
     * Nunca deixe um ponteiro sem inicializar — um ponteiro com lixo aponta
     * pra um endereço qualquer, e escrever nele destrói o que estiver lá. */
    int *vazio = NULL;
    printf("\nvazio = %p (isto é o NULL)\n", (void *) vazio);
    if (vazio == NULL)
        printf("testar antes de usar é o hábito que te salva\n");

    return 0;
}

/* ============================================================================
 * O DIAGRAMA COMPLETO DESTE PROGRAMA
 *
 *     0x7ffd1234  [ 30 ]            idade     (era 25, mudamos por *p)
 *     0x7ffd9999  [ 0x7ffd1234 ]    p         guarda o endereço de idade
 *                    |
 *                    +-----> aponta de volta pra caixa de cima
 *
 *     &idade  = 0x7ffd1234    o endereço da caixa
 *      idade  = 30            o conteúdo da caixa
 *          p  = 0x7ffd1234    a cópia do endereço, guardada em p
 *         *p  = 30            o conteúdo lá onde p aponta
 *         &p  = 0x7ffd9999    o endereço da caixa DO PONTEIRO
 *
 * Se você consegue ler essas cinco linhas sem hesitar, os próximos onze
 * passos são só consequência.
 *
 * EXPERIMENTE:
 *
 *  1. Rode duas vezes. Os endereços mudam. Isso é o ASLR, uma proteção do
 *     sistema: a cada execução o programa nasce num lugar diferente da
 *     memória. Endereço nunca é algo pra você decorar ou comparar entre
 *     execuções.
 *
 *  2. Escreva `printf("%d\n", *vazio);` no fim. Rode. O sanitizer mata o
 *     programa e diz exatamente onde:
 *     "load of null pointer of type 'int'".
 *     Sem sanitizer, isto seria só "Segmentation fault (core dumped)".
 *
 *  3. Aponte dois ponteiros pra mesma caixa (`int *q = &idade;`) e mude por
 *     um (`*q = 99;`). Leia pelo outro (`*p`). Mesma caixa, dois nomes. É
 *     exatamente isso que acontece entre threads — e é por isso que threads
 *     precisam de mutex.
 *
 *  4. Tente `int *errado = idade;` (sem o &). O gcc reclama de conversão
 *     inteiro->ponteiro. Leia a mensagem com calma: ele está dizendo que
 *     você tentou usar o VALOR 30 como se fosse um ENDEREÇO.
 *
 * -> passo-06, que é onde ponteiro deixa de ser curiosidade e vira necessidade
 * ========================================================================= */
