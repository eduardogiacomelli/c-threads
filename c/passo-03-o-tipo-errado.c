/* ============================================================================
 * PASSO 3 - ERRADO DE PROPÓSITO. Contas com tipos misturados.
 *
 * Rode ANTES de ler a explicação. Olhe a saída errada com os próprios olhos,
 * e repare que o gcc te avisou (leia o painel de problemas do VS Code).
 *
 *     Ctrl+Shift+B      (ou: make 03)
 *
 * Este programa COMPILA e RODA. C deixa. É por isso que o erro é perigoso.
 * ========================================================================= */

#include <stdio.h>

int main(void)
{
    int total = 7;
    int gente = 2;

    /* BUG 1 - divisão entre dois int é divisão INTEIRA.
     * Não existe 3.5 aqui: o resultado é 3, e o 0.5 é jogado fora ANTES de
     * chegar no double. Declarar o destino como double não salva nada - a
     * conta já aconteceu. */
    double media = total / gente;
    printf("média de %d entre %d pessoas: %f\n", total, gente, media);

    /* BUG 2 - printf não sabe os tipos do que você passou. Ele CONFIA no
     * molde: %d significa "vá buscar um int no lugar onde ints são
     * entregues". Você entregou um double, que viaja por outro caminho.
     * printf pega o que estiver naquele lugar, e imprime. */
    double preco = 19.9;
    printf("preço com %%d: %d      <- lixo\n", preco);

    /* BUG 3 - o contrário: %f vai buscar um double, e você entregou um int.
     * Olhe bem o número que sai. Ele não é aleatório: é o `preco` da linha
     * de cima, que ficou parado no lugar de onde %f lê. A explicação está
     * no rodapé, e é mais interessante que "deu lixo". */
    int quantidade = 3;
    printf("quantidade com %%f: %f  <- lixo\n", quantidade);

    /* BUG 4 - o mesmo BUG 1 escondido dentro de uma expressão maior.
     * (9/5) vira 1, não 1.8. A fórmula está certa; a aritmética, não. */
    int celsius = 100;
    double fahrenheit = celsius * (9 / 5) + 32;
    printf("\n100 °C em °F: %.1f   (o certo é 212.0)\n", fahrenheit);

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 * Duas regras, e as quatro linhas erradas saem delas:
 *
 * 1. O TIPO DO RESULTADO VEM DOS OPERANDOS, NÃO DO DESTINO.
 *
 *        int / int  ->  int      (7/2 = 3, o resto some)
 *
 *    O compilador calcula 3 e só DEPOIS converte pra double, virando 3.0.
 *    A parte fracionária nunca existiu.
 *
 * 2. printf NÃO SABE O QUE VOCÊ PASSOU.
 *
 *    Os argumentos chegam sem etiqueta nenhuma. O molde é a única instrução
 *    de onde buscá-los e como lê-los. E aqui entra um detalhe da máquina que
 *    explica a saída esquisita: no x86-64, inteiros e ponto flutuante viajam
 *    por CAMINHOS DIFERENTES - registradores separados.
 *
 *        entregas de inteiro:        [ ? ][ ? ]        <- %d lê daqui
 *        entregas de ponto flutuante:[ 19.9 ]          <- %f lê daqui
 *
 *    No BUG 2 você entregou 19.9 pelo caminho do ponto flutuante e mandou o
 *    %d ler pelo caminho dos inteiros: ele leu o que estava sobrando ali -
 *    aquele número grande sem sentido.
 *
 *    No BUG 3 você entregou um int e mandou o %f ler pelo caminho do ponto
 *    flutuante: ele encontrou o 19.9 que ainda estava lá do printf anterior.
 *    Por isso a quantidade "3" saiu como 19.900000.
 *
 *    Não é aleatório e não é um número corrompido. É o valor errado, lido do
 *    lugar errado, de um jeito perfeitamente determinístico - e é por isso
 *    que esse bug engana: ele parece estável.
 *
 * O gcc AVISOU nos dois casos de printf (-Wformat, ligado pelo -Wall).
 * Em C, warning não é ruído: é o compilador vendo o bug antes de você.
 *
 * EXPERIMENTE:
 *
 *  1. Leia a saída do compilador no painel de baixo. Ache as duas linhas
 *     "format '%d' expects argument of type 'int', but argument 2 has type
 *     'double'". Ele te disse exatamente onde.
 *
 *  2. Rode duas vezes. A saída é a MESMA, e o 19.900000 do BUG 3 continua
 *     ali. Agora inverta a ordem: ponha o printf do BUG 3 ANTES do BUG 2.
 *     O número muda, porque agora não tem mais um double "recém-entregue"
 *     pro %f encontrar. O bug depende do código vizinho.
 *
 *  3. Troque `int gente = 2;` por `double gente = 2;` e rode. A média
 *     conserta sozinha. Por quê? Porque agora um dos operandos é double, e
 *     C promove o outro antes de dividir. Essa é a chave do passo-04.
 *
 * -> passo-04, a correção
 * ========================================================================= */
