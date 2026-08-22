/* ============================================================================
 * PASSO 17 - argumentos da linha de comando, e o atoi que mente.
 *
 * O Exercício 7 e os dois Desafios da lista de pthreads exigem isto:
 * "o programa recebe parâmetros pela linha de comando (usar argc e argv)".
 *
 * A primeira metade do arquivo está certa. A segunda está ERRADA DE PROPÓSITO,
 * e o erro é silencioso: nada quebra, o número é que sai errado.
 *
 *     make 17                          (sem argumentos)
 *     make 17 ARGS="10 4"              (com dois argumentos)
 *     ./passo-17-argc-argv 1e9 abc     (depois de compilar, o caso interessante)
 *
 * Pelo Ctrl+Shift+B você não consegue passar argumentos. Este é um dos poucos
 * passos que pede o terminal.
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* atoi, atof */

/* Até agora main era `int main(void)`. A outra forma é esta, e ela recebe o
 * que você digitou depois do nome do programa:
 *
 *   argc  ("argument count")  quantas palavras, CONTANDO o nome do programa
 *   argv  ("argument vector") um vetor de strings com essas palavras
 *
 * `char *argv[]` é um vetor de ponteiros pra char - ou seja, um vetor de
 * strings (passo-10 + passo-11). Também se escreve `char **argv`: é a mesma
 * coisa, pelo mesmo motivo do passo-10. */
int main(int argc, char *argv[])
{
    /* argv[0] é SEMPRE o nome com que o programa foi chamado. Por isso
     * `./prog 10 4` dá argc = 3, não 2. Erro clássico de contagem. */
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);

    /* O enunciado pede dois números. Sem eles, não dá pra continuar: imprima
     * o modo de usar e SAIA. Duas convenções importantes aqui:
     *
     *   - a mensagem de erro vai pra stderr (fprintf(stderr, ...)), não pra
     *     stdout. Assim ela aparece na tela mesmo se a saída for redirecionada
     *     pra um arquivo;
     *   - o código de saída é != 0 (passo-01), pra quem chamou saber que
     *     falhou.
     *
     * Use argv[0] na mensagem em vez de escrever o nome na mão: se o binário
     * for renomeado, a mensagem continua certa. */
    if (argc < 3) {
        fprintf(stderr, "\nuso: %s <tamanho> <threads>\n", argv[0]);
        fprintf(stderr, "exemplo: %s 1000 4\n", argv[0]);
        return 1;
    }

    /* ARGUMENTO É SEMPRE STRING. Sempre. Mesmo quando é "10".
     * argv[1] são os bytes '1','0','\0' - não o número dez. Converter é
     * trabalho seu. */
    printf("\nargv[1] como string: \"%s\"\n", argv[1]);
    printf("argv[1][0] é o caractere '%c' (código %d), não o número %c\n",
           argv[1][0], argv[1][0], argv[1][0]);

    /* ================= A PARTIR DAQUI ESTÁ ERRADO =================
     *
     * atoi é a conversão que todo mundo usa primeiro, e ela não tem como
     * relatar erro: o tipo de retorno é int, e todo int é um resultado
     * possível. Então ela chuta:
     *
     *   atoi("abc")  ->  0      não tem número nenhum, devolve 0
     *   atoi("1e9")  ->  1      lê o 1, para no 'e', devolve 1
     *   atoi("")     ->  0
     *   atoi("12abc")->  12     lê o que dá e ignora o resto
     *
     * Repare que 0 é ao mesmo tempo "deu erro" e "o usuário digitou zero".
     * Não dá pra distinguir. */
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    printf("\ncom atoi:  n = %d, m = %d\n", n, m);

    if (n <= 0 || m <= 0) {
        fprintf(stderr, "n e m precisam ser positivos\n");
        return 1;
    }

    /* E aqui o programa segue feliz com o valor errado. */
    printf("vou processar %d elementos com %d threads\n", n, m);
    printf("(confira: era isso mesmo que você digitou?)\n");

    return 0;
}

/* ============================================================================
 * TESTE ESTES QUATRO CASOS, NO TERMINAL
 *
 *     make                                          # compila tudo
 *     ./passo-17-argc-argv                          # falta argumento
 *     ./passo-17-argc-argv 1000 4                   # certo
 *     ./passo-17-argc-argv abc 4                    # <- atoi devolve 0
 *     ./passo-17-argc-argv 1e9 4                    # <- atoi devolve 1
 *
 * O último é o que pega no Desafio 1, que manda passar as iterações em
 * notação científica:
 *
 *     ./calcula-pi 1e9 4
 *
 * Com atoi você não vai calcular 1.000.000.000 de iterações. Vai calcular
 * UMA, em microssegundos, e achar que o seu programa ficou incrivelmente
 * rápido. O bug se disfarça de bom resultado.
 *
 * O DIAGRAMA DO argv
 *
 *   ./prog 1000 4
 *
 *   argc = 3
 *   argv ---> [ 0 ] --> "./prog\0"
 *             [ 1 ] --> "1000\0"
 *             [ 2 ] --> "4\0"
 *             [ 3 ] --> NULL      <- o padrão garante este NULL no fim
 *
 *   Um vetor de ponteiros. Cada elemento é um endereço de string, exatamente
 *   como no passo-11. Por isso `char *argv[]` e `char **argv` são a mesma
 *   declaração.
 *
 * EXPERIMENTE:
 *
 *  1. Rode com `./passo-17-argc-argv um dois três "quatro cinco"`.
 *     Quantos argumentos o shell entregou? As aspas contam como um só. Quem
 *     separa as palavras é o shell, não o seu programa.
 *
 *  2. Imprima `argv[argc]`. É NULL - dá pra percorrer o argv com
 *     `while (*argv != NULL)` em vez de usar argc. Não faça, mas saiba.
 *
 *  3. Rode `./passo-17-argc-argv 99999999999 4` (11 noves). atoi estoura o
 *     int silenciosamente. Compare com o que o passo-18 faz nesse mesmo caso.
 *
 *  4. Troque atoi por atof e imprima com %f. `atof("1e9")` dá
 *     1000000000.000000 - resolve a notação científica, mas continua sem
 *     saber dizer se a entrada era lixo. Meio caminho.
 *
 * -> passo-18, a validação que serve pra entregar
 * ========================================================================= */
