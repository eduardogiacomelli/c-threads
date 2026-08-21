/* ============================================================================
 * PASSO 18 — a correção do passo-17: strtol e strtod, que sabem falhar.
 *
 * O Desafio 2 pede explicitamente "teste de consistência da entrada
 * fornecida". Isto aqui é esse teste.
 *
 *     make 18 ARGS="1e9 4"
 *     ./passo-18-validando-a-entrada abc 4
 *     ./passo-18-validando-a-entrada 1e9 0
 *     ./passo-18-validando-a-entrada 99999999999999999999 4
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>     /* strtol, strtod */
#include <errno.h>      /* errno, ERANGE  */
#include <limits.h>     /* LONG_MAX       */

/* A diferença entre atoi e strtol é que strtol tem COMO te contar o que
 * aconteceu, por dois canais:
 *
 *   - `fim`, um ponteiro que ela faz apontar pro primeiro caractere que ela
 *     NÃO conseguiu ler. Se ele aponta pro '\0', ela leu a string inteira;
 *     se aponta pro começo, ela não leu nada;
 *   - `errno`, uma variável global do sistema, que vira ERANGE se o número
 *     não coube no tipo.
 *
 * Por isso ela recebe `char **fim`: é o passo-07 (a função precisa escrever
 * numa variável sua), só que a variável é um ponteiro. Você passa o endereço
 * de um `char *`, e ela escreve lá dentro.
 *
 * Esta função devolve 1 se deu certo e 0 se não deu, escrevendo o resultado
 * em *destino — mesmo padrão. */
int ler_inteiro_positivo(const char *texto, long *destino)
{
    char *fim;

    errno = 0;                                  /* zere ANTES de chamar */
    long valor = strtol(texto, &fim, 10);       /* 10 = base decimal */

    if (fim == texto)                return 0;  /* não leu nada: "abc" */
    if (*fim != '\0')                return 0;  /* sobrou lixo: "12abc" */
    if (errno == ERANGE)             return 0;  /* não coube num long */
    if (valor <= 0)                  return 0;  /* a regra do enunciado */
    if (valor > INT_MAX)             return 0;  /* vai virar int depois? */

    *destino = valor;
    return 1;
}

/* Mesma ideia, mas com strtod, que entende notação científica — o "1e9" que
 * o Desafio 1 exige. strtod devolve double; converta pra long depois. */
int ler_iteracoes(const char *texto, long *destino)
{
    char *fim;

    errno = 0;
    double valor = strtod(texto, &fim);

    if (fim == texto)      return 0;
    if (*fim != '\0')      return 0;
    if (errno == ERANGE)   return 0;
    if (valor < 1.0)       return 0;

    *destino = (long) valor;    /* 1e9 -> 1000000000 */
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "uso: %s <iteracoes> <threads>\n", argv[0]);
        fprintf(stderr, "  iteracoes: inteiro ou notação científica (1e9)\n");
        fprintf(stderr, "  threads:   inteiro positivo\n");
        return 1;
    }

    long iteracoes;
    long threads;

    /* Valide CADA argumento separadamente, com uma mensagem que diz qual
     * deles está errado e o que veio. "entrada inválida" sozinho é inútil
     * pra quem está usando o programa — inclusive pra você, testando. */
    if (!ler_iteracoes(argv[1], &iteracoes)) {
        fprintf(stderr, "erro: iteracoes inválido: \"%s\"\n", argv[1]);
        return 1;
    }
    if (!ler_inteiro_positivo(argv[2], &threads)) {
        fprintf(stderr, "erro: threads inválido: \"%s\"\n", argv[2]);
        return 1;
    }

    /* Regras que dependem dos dois valores juntos vêm depois de ambos
     * validados. Aqui entra o tipo de pergunta que o Exercício 7 faz:
     * mais threads que trabalho faz sentido? */
    if (threads > iteracoes) {
        fprintf(stderr, "erro: %ld threads para %ld iterações não faz sentido\n",
                threads, iteracoes);
        return 1;
    }

    printf("ok: %ld iterações, %ld threads\n", iteracoes, threads);
    printf("cada thread pega %ld, e sobram %ld\n",
           iteracoes / threads, iteracoes % threads);

    /* Esse resto é o detalhe que o Desafio 2 avisa em letras miúdas:
     * "lembre-se de tratar casos onde a divisão do worksize pelo número de
     * threads não seja exata". Rode com 10 e 3 e olhe o que sobra. */

    return 0;
}

/* ============================================================================
 * O PADRÃO, EM TRÊS LINHAS
 *
 *     errno = 0;
 *     long v = strtol(texto, &fim, 10);
 *     deu certo = (fim != texto) && (*fim == '\0') && (errno != ERANGE);
 *
 * O `&fim` é o passo-07 aplicado a um ponteiro:
 *
 *     "1e9\0"
 *      ^   ^
 *      |   +-- se `fim` parar aqui (no \0), leu tudo
 *      +------ se `fim` ficar aqui (no começo), não leu nada
 *
 * Com strtol, "1e9" para no 'e' -> `*fim` é 'e' -> rejeitado, e você fica
 * sabendo. Com atoi, o mesmo caso vira 1 em silêncio. A diferença entre as
 * duas não é a conversão: é o relatório.
 *
 * O CHECKLIST DE VALIDAÇÃO QUE OS ENUNCIADOS PEDEM
 *
 *   [ ] argc é o que você espera (lembre do argv[0])
 *   [ ] cada argumento converteu inteiro, sem sobra
 *   [ ] não estourou o tipo (ERANGE)
 *   [ ] respeita a regra do enunciado (positivo, != 0)
 *   [ ] as combinações fazem sentido (M > N? threads = 0?)
 *   [ ] mensagem em stderr dizendo QUAL argumento e COMO usar
 *   [ ] return != 0
 *
 * EXPERIMENTE:
 *
 *  1. Rode os quatro casos do cabeçalho e leia cada mensagem. Depois rode
 *     `./passo-18-validando-a-entrada 10 3` e olhe o resto da divisão: 1
 *     elemento sem dono. Quem processa esse? Guarde a pergunta — ela volta
 *     no Exercício 6, no 7 e no Desafio 2.
 *
 *  2. Tire o `errno = 0;` e rode com um número gigante duas vezes seguidas.
 *     errno é global e ninguém zera pra você: ele guarda o erro da última
 *     função que falhou, possivelmente de outro lugar do programa.
 *
 *  3. Troque a base de strtol pra 16 e passe "ff". Dá 255. A base é
 *     parâmetro; base 0 faz ela adivinhar pelo prefixo (0x, 0).
 *
 *  4. Escreva a validação como o enunciado do Desafio 2 pede — threads e
 *     worksizetotal — e teste com entrada vazia, negativa, zero, texto e
 *     um número absurdo. São cinco testes de 10 segundos que salvam a nota.
 *
 * -> passo-19: quando um arquivo só não basta
 * ========================================================================= */
