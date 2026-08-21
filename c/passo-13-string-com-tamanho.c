/* ============================================================================
 * PASSO 13 — a correção: toda função de escrita recebe o tamanho do destino.
 *
 *     Ctrl+Shift+B      (ou: make 13)
 *
 * A ideia é sempre a mesma: quem escreve precisa saber onde parar. Como o
 * destino não sabe o próprio tamanho, você informa.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>

int main(void)
{
    char nome[8];

    /* snprintf é a ferramenta padrão. Ela:
     *   - recebe o tamanho do destino (sizeof(nome), calculado pelo
     *     compilador — nunca escreva o 8 na mão, senão os dois números
     *     saem de sincronia no dia em que você mudar o vetor);
     *   - corta o que não couber;
     *   - SEMPRE termina com '\0';
     *   - e devolve quantos bytes ela QUERIA ter escrito.
     *
     * Esse retorno é o detector de truncamento: se ele for >= o tamanho do
     * destino, o texto não coube inteiro.
     *
     * O gcc avisa aqui ("output truncated") — ele consegue, porque a string
     * é uma constante que ele lê na hora de compilar. Com um nome digitado
     * pelo usuário ele não teria como saber, e o aviso não viria. Por isso o
     * teste abaixo, em tempo de execução, é que é a proteção de verdade. */
    int queria = snprintf(nome, sizeof(nome), "%s", "Eduardo Giacomelli");

    printf("nome    = \"%s\"  (%zu bytes disponíveis)\n", nome, sizeof(nome));
    printf("queria escrever %d bytes -> ", queria);
    if (queria >= (int) sizeof(nome))
        printf("NÃO COUBE, foi truncado\n");
    else
        printf("coube inteiro\n");

    /* Truncar silenciosamente é melhor que corromper memória, mas ainda é um
     * dado errado. Em código de verdade você trata o caso: erro, ou aloca
     * maior (passo-15). O importante é que agora você SABE que aconteceu. */

    /* snprintf também é o jeito de montar texto com números juntos —
     * é o printf, escrevendo num vetor em vez da tela. */
    char linha[64];
    int  idade = 25;
    double nota = 8.75;
    snprintf(linha, sizeof(linha), "%s tem %d anos e nota %.1f",
             "Ana", idade, nota);
    printf("\nlinha   = \"%s\"\n", linha);

    /* Concatenar com segurança: escreva a partir do fim atual, com o espaço
     * QUE SOBRA como limite. As duas contas com strlen são o cuidado todo. */
    size_t usado = strlen(linha);
    snprintf(linha + usado, sizeof(linha) - usado, " (turma B)");
    printf("linha   = \"%s\"\n", linha);

    /* E o caso mais fácil de todos, que muita gente esquece: se o texto é
     * uma constante que você escreveu, deixe o compilador contar. O [] vazio
     * reserva exatamente o necessário, com o terminador. */
    char certo[] = "Eduardo Giacomelli";
    printf("\ncerto[] = \"%s\" (%zu bytes, contados pelo compilador)\n",
           certo, sizeof(certo));

    return 0;
}

/* ============================================================================
 * O RESUMO PRÁTICO
 *
 *     em vez de              use
 *     ---------              ---
 *     strcpy(d, s)           snprintf(d, sizeof(d), "%s", s)
 *     strcat(d, s)           snprintf(d + strlen(d), sizeof(d) - strlen(d), ...)
 *     sprintf(d, ...)        snprintf(d, sizeof(d), ...)
 *     gets(d)                nunca. Foi REMOVIDA do padrão C11 de tão ruim.
 *                            Para ler uma linha: fgets(d, sizeof(d), stdin)
 *
 * E a regra que gera todas elas:
 *
 *     sizeof(destino) só funciona onde o vetor foi DECLARADO (passo-10).
 *     Numa função que recebeu `char *d`, sizeof(d) é 8. Aí o tamanho tem que
 *     vir como parâmetro, exatamente como o `n` do vetor de inteiros.
 *
 * EXPERIMENTE:
 *
 *  1. Imprima `nome` byte a byte como no passo-11 e confirme que o '\0' está
 *     no lugar certo, na última posição. snprintf garante isso; strncpy, a
 *     função que parece a escolha óbvia, NÃO garante — é por isso que este
 *     passo recomenda snprintf e não strncpy.
 *
 *  2. Troque sizeof(nome) por 8 escrito na mão. Funciona. Agora mude a
 *     declaração para char nome[4] e rode: o 8 continua lá, mentindo, e o
 *     ASan pega o estouro. Números mágicos duplicados são bug adormecido.
 *
 *  3. Escreva uma função `void saudacao(char *destino, size_t tam,
 *     const char *quem)` que monta "olá, X" com snprintf. Chame com
 *     sizeof do vetor lá de main. Este é o padrão de API do C inteiro:
 *     ponteiro + tamanho, sempre em par.
 *
 *  4. Leia `man 3 snprintf` e ache a descrição do valor de retorno. Confira
 *     que bate com o que este programa fez. Aprender a ler o man é metade
 *     de aprender C.
 *
 * -> passo-14: o tempo de vida das variáveis, e o bug que ele causa
 * ========================================================================= */
