/* ============================================================================
 * PASSO 7 - a correção: entregue o ENDEREÇO das caixas.
 *
 *     Ctrl+Shift+B      (ou: make 07)
 *
 * Três mudanças em relação ao passo-06. Só três. Ache as três antes de ler.
 * ========================================================================= */

#include <stdio.h>

/* MUDANÇA 1 - os parâmetros agora são ENDEREÇOS de int, não int.
 * A função não recebe mais "dois números"; recebe "onde moram dois números". */
void trocar(int *a, int *b)
{
    /* MUDANÇA 2 - todo acesso passa por *.
     *
     *   a   é o endereço  (a caixa lá de main)
     *   *a  é o conteúdo  (o número dentro dela)
     *
     * Escrever `a = b` aqui trocaria só os endereços dentro desta função -
     * seria o passo-06 de novo, com um passo a mais de indireção. O que
     * queremos é trocar os CONTEÚDOS. */
    printf("   [dentro] recebi os endereços %p e %p\n", (void *) a, (void *) b);
    printf("   [dentro] que contêm %d e %d\n", *a, *b);

    int temporario = *a;   /* leia lá em main e guarde aqui  */
    *a = *b;               /* escreva em main o que está no outro */
    *b = temporario;       /* escreva em main o valor guardado    */
}

int main(void)
{
    int x = 10;
    int y = 20;

    printf("antes:  x=%d y=%d\n", x, y);
    printf("(x mora em %p, y mora em %p)\n", (void *) &x, (void *) &y);

    /* MUDANÇA 3 - passe &x e &y, não x e y.
     * Esse & é a diferença entre "aqui está o valor" e "aqui está a chave da
     * minha caixa, pode mexer". */
    trocar(&x, &y);

    printf("depois: x=%d y=%d   <- trocou de verdade\n", x, y);

    return 0;
}

/* ============================================================================
 * O DIAGRAMA
 *
 * A cópia continua acontecendo! C não mudou de regra. O que é copiado agora
 * é o ENDEREÇO, e uma cópia de endereço aponta pro mesmo lugar que o
 * original:
 *
 *     main:                        trocar:
 *     0x7ffd1000 [ 10 ]  x   <---- 0x7ffd0900 [ 0x7ffd1000 ]  a
 *     0x7ffd1004 [ 20 ]  y   <---- 0x7ffd0908 [ 0x7ffd1004 ]  b
 *
 * `a` é uma caixa nova, sim, e morre no fim da função - mas enquanto ela
 * existe, `*a` alcança a caixa de main. Uma cópia do endereço da sua casa
 * ainda leva à sua casa.
 *
 * ESSA É A ÚNICA MANEIRA DE UMA FUNÇÃO EM C MUDAR ALGO DE FORA.
 * Por isso C é cheio de & nas chamadas - e é por isso que scanf pede &:
 *
 *     scanf("%d", &n);      "aqui está onde guardar o que o usuário digitar"
 *
 * Você acabou de entender scanf. Esquecer o & ali é o erro nº 1 de iniciante:
 * scanf recebe o VALOR de n como se fosse um endereço, e escreve num lugar
 * aleatório da memória.
 *
 * EXPERIMENTE:
 *
 *  1. Dentro de trocar, tire os asteriscos: `int t = a; a = b; b = t;`.
 *     O gcc reclama de tipo (int recebendo int *) e a troca não acontece.
 *     Você trocou os endereços dentro da função, que é o passo-06 outra vez.
 *
 *  2. Chame `trocar(x, y)` sem os &. O gcc avisa: "passing argument 1 makes
 *     pointer from integer without a cast". Rode assim mesmo e veja o
 *     sanitizer matar o programa: 10 foi usado como se fosse um endereço.
 *
 *  3. Chame `trocar(&x, &x)`. Os dois ponteiros vão pra mesma caixa. Trace
 *     no papel o que acontece com o valor antes de rodar. Depois rode.
 *
 *  4. Escreva `void zerar(int *n)` que põe 0 na caixa apontada, e chame com
 *     `zerar(&x)`. É o mesmo padrão com um parâmetro só. Repita até ficar
 *     automático - pthread_create vai pedir exatamente isso de você.
 *
 * -> passo-08: vetores
 * ========================================================================= */
