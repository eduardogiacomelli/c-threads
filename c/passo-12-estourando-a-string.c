/* ============================================================================
 * PASSO 12 - ERRADO DE PROPÓSITO. strcpy num destino pequeno demais.
 *
 * Este é, literalmente, o bug que gerou a maior parte das falhas de
 * segurança da história do software. E ele é só o passo-09 com char.
 *
 *     Ctrl+Shift+B      (ou: make 12)
 *
 * O AddressSanitizer mata o programa. Leia o relatório inteiro dele.
 * ========================================================================= */

#include <stdio.h>
#include <string.h>

int main(void)
{
    /* Espaço para 7 caracteres + o '\0'. */
    char nome[8];
    char sobrenome[8] = "Silva";

    printf("nome[] tem %zu bytes -> cabem 7 letras + o terminador\n\n",
           sizeof(nome));

    /* BUG 1 - strcpy não pergunta o tamanho do destino. NÃO TEM COMO: ela
     * recebe só dois endereços. Ela copia da origem até achar o '\0', e
     * escreve tudo isso no destino, doa a quem doer.
     *
     * "Eduardo Giacomelli" tem 18 caracteres + terminador = 19 bytes.
     * O destino tem 8. Onde vão os outros 11? Na memória depois de nome[]. */
    strcpy(nome, "Eduardo Giacomelli");

    printf("nome      = %s\n", nome);
    printf("sobrenome = %s   <- olhe bem\n", sobrenome);

    /* BUG 2 - o mesmo erro por outro caminho: concatenar sem conferir se
     * cabe. strcat vai até o '\0' do destino e escreve a partir dali. */
    char pequeno[10] = "12345";
    strcat(pequeno, "67890abcdef");
    printf("pequeno   = %s\n", pequeno);

    return 0;
}

/* ============================================================================
 * O QUE ACONTECEU
 *
 *     char nome[8];          char sobrenome[8] = "Silva";
 *
 *     antes do strcpy:
 *     0x..00 [?][?][?][?][?][?][?][?]  [S][i][l][v][a][\0][0][0]
 *            \___________ nome _____/  \_________ sobrenome ____/
 *
 *     strcpy(nome, "Eduardo Giacomelli") escreve 19 bytes a partir de 0x..00:
 *
 *     0x..00 [E][d][u][a][r][d][o][ ]  [G][i][a][c][o][m][e][l][l][i][\0]
 *            \___________ nome _____/  \___ onde sobrenome morava ____/
 *                                       \-- e ainda passou disso
 *
 * O `sobrenome` foi sobrescrito por uma variável que não tem relação nenhuma
 * com ele. Nenhuma linha do programa menciona `sobrenome` - e ele mudou.
 *
 * Agora imagine que, em vez de `sobrenome`, ali estivesse o endereço de
 * retorno da função. Quem controla o texto de entrada passa a controlar para
 * onde o programa desvia. É esse o mecanismo por trás de "buffer overflow"
 * nas notícias de segurança.
 *
 * O RELATÓRIO DO ASan
 *
 *     ERROR: AddressSanitizer: stack-buffer-overflow
 *     WRITE of size 19 at 0x...
 *         #0 in memcpy
 *         #1 in main passo-12-estourando-a-string.c:30
 *     This frame has 3 object(s):
 *       [32,  40) 'nome' (line 18) <== Memory access at offset 40 overflows
 *                                      this variable
 *       [64,  72) 'sobrenome' (line 19)
 *       [96, 106) 'pequeno' (line 37)
 *
 * "WRITE of size 19" num objeto de 8 bytes: a conta está toda ali. E repare
 * que o ASan lista as três variáveis da pilha com os limites exatos de cada
 * uma - é assim que você descobre em QUEM você pisou.
 *
 * (O strcpy virou memcpy no relatório: o gcc troca por uma versão otimizada
 * quando conhece o tamanho. É a mesma linha 30 do seu código.)
 *
 * A REGRA
 *
 *   Nunca use strcpy, strcat ou sprintf com dado de tamanho que você não
 *   controla. Existem versões que recebem o tamanho do destino - passo-13.
 *
 * EXPERIMENTE:
 *
 *  1. Comente o strcpy e rode só o strcat. Mesmo tipo de erro, outra função.
 *     O ASan aponta o strcat.
 *
 *  2. Aumente nome para char nome[64] e rode. Passa limpo. Note o incômodo:
 *     o programa está "certo" só porque o nome coube desta vez. Bug de
 *     tamanho é sempre bug de "e se o dado for maior?".
 *
 *  3. Rode sem sanitizer e compare:
 *
 *         gcc -std=gnu17 -Wall -g passo-12-estourando-a-string.c -o /tmp/s12
 *         /tmp/s12
 *
 *     Pode imprimir tudo "normalmente", com sobrenome corrompido - ou
 *     quebrar. Depende do humor do compilador naquele dia.
 *
 *  4. Copie exatamente 7 letras ("Eduardo") pro nome[8]. Cabe, com o
 *     terminador no oitavo byte. Agora tente 8 letras. Um byte a mais, e o
 *     ASan pega. Essa é a margem com que você trabalha em C.
 *
 * -> passo-13, o jeito certo
 * ========================================================================= */
