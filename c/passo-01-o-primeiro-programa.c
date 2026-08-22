/* ============================================================================
 * PASSO 1 - o menor programa que existe, linha por linha.
 *
 * Você já sabe programar. O que muda aqui é o modelo de execução: não existe
 * interpretador. Este arquivo de texto vira um binário, e o sistema executa
 * o binário. Nada roda "de fora".
 *
 *     Ctrl+Shift+B      (ou: make 01)
 * ========================================================================= */

/* #include NÃO é `import`. Não carrega nada em tempo de execução.
 *
 * É um comando pro PRÉ-PROCESSADOR, que roda antes do compilador: ele apaga
 * esta linha e cola no lugar dela o conteúdo inteiro do arquivo stdio.h
 * (que fica em /usr/include/stdio.h - pode abrir).
 *
 * O que vem colado ali são DECLARAÇÕES: "existe uma função chamada printf,
 * ela recebe uma string e mais o que vier, e devolve int". Só a assinatura.
 * O código de verdade do printf está compilado dentro da libc, e o LINKER
 * junta os dois no final.
 *
 * Duas etapas, dois erros diferentes:
 *   esqueceu o #include  -> "implicit declaration of function 'printf'"
 *   esqueceu de linkar   -> "undefined reference to 'printf'"
 */
#include <stdio.h>

/* `int main(void)` é onde o sistema começa a executar. Não é convenção do
 * programador: o binário aponta pra cá.
 *
 *   int    -> main devolve um inteiro ao sistema operacional
 *   (void) -> não recebe argumento nenhum. Escrever `main()` vazio é outra
 *             coisa em C (significa "não digo nada sobre os argumentos"),
 *             então escreva (void) quando não quiser argumentos.
 */
int main(void)
{
    /* printf não é `print`. O primeiro argumento é um MOLDE (format string),
     * e cada % dentro dele é um buraco a preencher com os próximos
     * argumentos.
     *
     *   %d = preencha com um int, escrito em decimal
     *   %s = preencha com uma string
     *   \n = quebra de linha. printf NÃO quebra linha sozinho, ao contrário
     *        do print() do Python. Se a saída sair grudada, faltou \n.
     */
    printf("olá, C\n");
    printf("dois mais dois é %d\n", 2 + 2);

    /* O valor devolvido por main é o "código de saída" do programa.
     *   0        = deu tudo certo
     *   qualquer outro = deu erro
     *
     * Quem lê isso é o shell, não você. É assim que `cmd1 && cmd2` sabe se
     * pode rodar o cmd2.
     */
    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 *   passo-01.c  --pré-processador-->  um .c gigante com stdio.h colado
 *               --compilador------->  código de máquina (.o)
 *               --linker----------->  binário executável, com a libc junto
 *               --você------------->  ./passo-01-o-primeiro-programa
 *
 * Um erro pode aparecer em qualquer uma dessas quatro etapas, e as mensagens
 * são bem diferentes. Saber de qual etapa veio o erro economiza muito tempo.
 *
 * EXPERIMENTE:
 *
 *  1. Apague o \n do primeiro printf. Rode. Veja as duas linhas grudarem.
 *
 *  2. Apague a linha #include <stdio.h>. Rode.
 *     O gcc reclama: "implicit declaration of function 'printf'". Ele ainda
 *     assim gera o binário (herança dos anos 70) - mas isso é um aviso de que
 *     ele está adivinhando os tipos. Nunca ignore.
 *
 *  3. Troque `return 0;` por `return 3;`. Rode pelo terminal e pergunte o
 *     código de saída:
 *
 *         make 01 ; echo "saiu com: $?"
 *
 *  4. Troque printf("olá, C\n") por printf("olá, %s\n", "C").
 *     Mesmo resultado, agora com um buraco preenchido.
 *
 * -> passo-02
 * ========================================================================= */
