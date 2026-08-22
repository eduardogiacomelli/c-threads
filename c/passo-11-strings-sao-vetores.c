/* ============================================================================
 * PASSO 11 - string em C é um vetor de char terminado por '\0'.
 *
 * Não existe tipo string. Não existe .length. O que existe é uma convenção:
 * a string acaba quando aparece um byte zero. Toda função de string do C
 * confia nessa convenção - e é ela que vai quebrar no passo-12.
 *
 *     Ctrl+Shift+B      (ou: make 11)
 * ========================================================================= */

#include <stdio.h>
#include <string.h>     /* strlen, strcpy, strcmp: man 3 string */

int main(void)
{
    /* Duas formas de escrever a mesma coisa. A de cima é o que você usa;
     * a de baixo mostra o que ela realmente é. */
    char nome[6] = "Ana";
    char igual[6] = {'A', 'n', 'a', '\0', 0, 0};

    printf("nome  = %s\n", nome);
    printf("igual = %s\n", igual);

    /* O '\0' (byte zero) não é o caractere '0'. É o valor numérico 0, e ele
     * ocupa uma posição no vetor. "Ana" precisa de 4 bytes, não 3. */
    printf("\nbyte a byte de nome[6]:\n");
    for (size_t i = 0; i < 6; i++)
        printf("  nome[%zu] = %3d  '%c'\n", i, nome[i],
               nome[i] ? nome[i] : ' ');

    /* strlen CONTA os bytes até achar o zero. É um laço, custa O(n) - não é
     * um campo guardado como o len() do Python. Chamar strlen dentro da
     * condição de um laço percorre a string a cada volta. */
    printf("\nstrlen(nome)  = %zu  <- caracteres, o \\0 não conta\n",
           strlen(nome));
    printf("sizeof(nome)  = %zu  <- bytes reservados, o \\0 conta\n",
           sizeof(nome));

    /* Duas declarações que parecem iguais e não são:
     *
     *   char v[] = "oi";    um VETOR seu, na pilha, com uma cópia do texto.
     *                       Você pode escrever nele.
     *
     *   char *p  = "oi";    um PONTEIRO pro texto que está gravado numa área
     *                       SÓ LEITURA do binário. Escrever ali mata o
     *                       programa. Por isso o certo é `const char *`.
     */
    char meu[] = "oi";
    const char *literal = "oi";

    meu[0] = 'O';                      /* legal: o vetor é seu */
    printf("\nmeu     = %s  (dá pra alterar)\n", meu);
    printf("literal = %s  (const: o compilador te impede de alterar)\n",
           literal);

    /* Comparar strings com == compara ENDEREÇOS, não conteúdo. É quase
     * sempre um bug. Use strcmp, que devolve 0 quando são iguais.
     * (Sim: ZERO significa igual. Leia como "diferença zero".) */
    char a[] = "abc";
    char b[] = "abc";

    /* Escrevi com ponteiros porque `a == b` direto entre dois vetores faz o
     * gcc avisar ("comparison between two arrays"). Com `char *` ele deixa
     * passar caladinho - e é assim que o bug chega no código de verdade. */
    char *pa = a;
    char *pb = b;
    printf("\npa == pb       ? %s  <- compara endereços: caixas diferentes\n",
           pa == pb ? "sim" : "não");
    printf("strcmp(a,b)==0 ? %s  <- compara conteúdo\n",
           strcmp(a, b) == 0 ? "sim" : "não");

    /* Copiar também é função, não `=`. `destino = origem` entre vetores nem
     * compila (passo-10). strcpy copia byte a byte ATÉ O '\0', inclusive. */
    char destino[10];
    strcpy(destino, "Ana");
    printf("\ndestino após strcpy = %s\n", destino);

    return 0;
}

/* ============================================================================
 * O DIAGRAMA
 *
 *     char nome[6] = "Ana";
 *
 *     0x7ffd1000  [ 'A' ]  65
 *     0x7ffd1001  [ 'n' ]  110
 *     0x7ffd1002  [ 'a' ]  97
 *     0x7ffd1003  [ \0  ]  0     <- AQUI a string acaba
 *     0x7ffd1004  [  0  ]        <- reservado, sobrando
 *     0x7ffd1005  [  0  ]
 *
 *     printf("%s") recebe o endereço 0x7ffd1000 e imprime byte a byte até
 *     encontrar o zero. Se o zero não estiver lá, ele continua lendo pela
 *     memória adentro. Isso é o passo-12.
 *
 * A CONTA QUE VOCÊ SEMPRE PRECISA FAZER
 *
 *     texto de N caracteres  ->  vetor de N+1 bytes, no mínimo.
 *
 * Esquecer o +1 é o bug de string mais comum que existe.
 *
 * EXPERIMENTE:
 *
 *  1. Apague o '\0' à mão: `nome[3] = 'X';` antes do printf. Rode.
 *     Agora printf não encontra o terminador em nome[3] e segue lendo o
 *     resto do vetor e o que vier depois. Com sorte, o ASan te pega lendo
 *     fora. Sem sorte, aparece lixo na tela.
 *
 *  2. Declare `char apertado[3] = "Ana";` - 3 letras em 3 bytes, sem espaço
 *     pro terminador. O gcc até deixa (com aviso). Imprima com %s e veja o
 *     que acontece.
 *
 *  3. Tente escrever num literal:
 *
 *         char *p = "oi";     // sem const, pra o compilador deixar
 *         p[0] = 'O';
 *
 *     Segmentation fault na hora. Aquela memória é só-leitura de verdade,
 *     o sistema operacional garante. Por isso literal se escreve
 *     `const char *`.
 *
 *  4. Imprima `strcmp("abc", "abd")` e `strcmp("abd", "abc")`. Negativo e
 *     positivo: é ordem alfabética, no estilo "a - b". Zero é igual.
 *
 * -> passo-12: estourando uma string
 * ========================================================================= */
