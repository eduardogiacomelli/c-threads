/* ============================================================================
 * PASSO 16 — struct: várias caixas com um nome só.
 *
 * Este é o último passo antes das threads, e não é coincidência: a única
 * maneira de passar mais de um dado pra uma thread é enfiar tudo numa struct
 * e passar o endereço dela.
 *
 *     Ctrl+Shift+B      (ou: make 16)
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Uma struct é uma RECEITA de layout de memória: estes campos, nesta ordem,
 * grudados. Declarada fora de qualquer função, fica visível ao arquivo todo.
 *
 * `typedef` no começo é o que permite escrever `Aluno x` depois em vez de
 * `struct Aluno x`. É costume, não obrigação. */
typedef struct {
    char   nome[20];
    int    idade;
    double nota;
} Aluno;

/* POR VALOR: recebe uma CÓPIA da struct inteira — todos os 36 bytes.
 * Funciona, e é seguro (não altera o original), mas copia tudo a cada
 * chamada. Para ler, com struct pequena, tudo bem. */
void mostrar(Aluno a)
{
    /* ponto, porque `a` é uma struct, não um ponteiro */
    printf("   %s, %d anos, nota %.1f\n", a.nome, a.idade, a.nota);
}

/* POR PONTEIRO: recebe só o endereço, 8 bytes, e alcança o original.
 * Este é o padrão: por ponteiro quase sempre, e `const` quando é só leitura. */
void aumentar_nota(Aluno *a, double quanto)
{
    /* `a->nota` é abreviação de `(*a).nota`, e lê-se "o campo nota da struct
     * apontada por a". A seta existe porque a forma com parênteses é
     * insuportável de escrever. Os parênteses seriam obrigatórios: `*a.nota`
     * significa outra coisa (o . liga mais forte que o *). */
    a->nota += quanto;
    if (a->nota > 10.0)
        a->nota = 10.0;
}

int main(void)
{
    /* Inicialização por nome de campo: legível, e você não depende da ordem. */
    Aluno ana = { .nome = "Ana", .idade = 20, .nota = 7.5 };

    printf("struct Aluno ocupa %zu bytes ", sizeof(Aluno));
    printf("(%zu do nome + %zu da idade + %zu da nota, mais alinhamento)\n\n",
           sizeof(ana.nome), sizeof(ana.idade), sizeof(ana.nota));

    printf("ana:\n");
    mostrar(ana);

    aumentar_nota(&ana, 2.0);
    printf("depois de +2.0:\n");
    mostrar(ana);

    /* ATRIBUIÇÃO ENTRE STRUCTS COPIA TUDO. Isto é a exceção interessante:
     * struct você pode copiar com `=`; vetor, não (passo-10). Copiar uma
     * struct copia os vetores que estão DENTRO dela. */
    Aluno copia = ana;
    strcpy(copia.nome, "Clone");
    printf("\napós `copia = ana` e mudar o nome da cópia:\n");
    printf("   original: "); mostrar(ana);
    printf("   cópia:    "); mostrar(copia);
    printf("   -> caixas independentes\n");

    /* Vetor de structs: exatamente o passo-08, com um tipo maior. */
    Aluno turma[3] = {
        { .nome = "Bruno", .idade = 21, .nota = 6.0 },
        { .nome = "Carla", .idade = 19, .nota = 9.5 },
        { .nome = "Davi",  .idade = 22, .nota = 8.0 },
    };

    printf("\nturma:\n");
    for (size_t i = 0; i < sizeof(turma) / sizeof(turma[0]); i++)
        mostrar(turma[i]);

    /* STRUCT NO HEAP — o padrão que as threads vão exigir.
     * Junte o passo-15 com este: um bloco por unidade de trabalho, cada um
     * com os seus próprios dados, todos independentes. */
    Aluno *novo = malloc(sizeof(Aluno));   /* sizeof do TIPO, não do ponteiro */
    if (novo == NULL)
        return 1;

    /* Com ponteiro, todo acesso é com seta. */
    strcpy(novo->nome, "Elisa");
    novo->idade = 23;
    novo->nota  = 8.8;

    printf("\naluno alocado no heap:\n");
    mostrar(*novo);        /* *novo desreferencia: passa a struct por valor */
    printf("   (o mesmo, por ponteiro: %s, nota %.1f)\n",
           novo->nome, novo->nota);

    free(novo);

    return 0;
}

/* ============================================================================
 * O DIAGRAMA
 *
 *   Aluno ana = { "Ana", 20, 7.5 };   um bloco contíguo:
 *
 *     0x7ffd1000  [ 'A''n''a' \0 ... ]   nome[20]    20 bytes
 *     0x7ffd1014  [ 20 ]                 idade        4 bytes
 *     0x7ffd1018  [ 7.5 ]                nota         8 bytes
 *                                                    -----------
 *                                        sizeof(Aluno) = 32
 *
 *   O compilador pode inserir bytes vazios entre campos pra alinhar cada
 *   tipo num endereço múltiplo do tamanho dele (padding). Por isso
 *   sizeof(struct) nem sempre é a soma dos campos — confira na saída.
 *
 * PONTO OU SETA?
 *
 *     tenho a struct       ->  ana.nota
 *     tenho o endereço     ->  p->nota      (que é (*p).nota)
 *
 *   Errar isso é erro de compilação, não bug silencioso. O gcc diz
 *   "invalid type argument of '->'". Alívio raro em C.
 *
 * A PONTE PRAS THREADS
 *
 *   pthread_create passa UM argumento, do tipo void *. Precisa mandar três
 *   coisas pra thread? Struct, e passa o endereço:
 *
 *       typedef struct { int id; int *vetor; size_t inicio, fim; } Tarefa;
 *
 *       Tarefa *t = malloc(sizeof(Tarefa));    // um por thread (passo-15)
 *       t->id = i;  ...
 *       pthread_create(&threads[i], NULL, funcao, t);
 *
 *   Reutilizar uma struct só para todas as threads é o bug do passo-14 com
 *   outra roupa: todas leem a mesma caixa, e quem escreveu por último ganha.
 *
 * EXPERIMENTE:
 *
 *  1. Troque `void mostrar(Aluno a)` por `void mostrar(Aluno *a)` e ajuste
 *     os pontos para setas e as chamadas para `&ana`. Compare os dois: 32
 *     bytes copiados contra 8.
 *
 *  2. Dentro de mostrar (versão por valor), escreva `a.nota = 0;`. Imprima
 *     de novo em main. Nada mudou — é o passo-06 outra vez, agora com
 *     struct. Marque o parâmetro como `const Aluno *a` na versão por
 *     ponteiro e tente alterar: o compilador impede. Use const sempre que a
 *     função só lê.
 *
 *  3. Reordene os campos (double primeiro, depois int, depois o nome) e
 *     imprima sizeof(Aluno). O número pode mudar por causa do padding.
 *
 *  4. Faça um vetor de 3 ponteiros para Aluno, cada um com seu malloc,
 *     preencha num laço e libere num segundo laço. Esse é, quase letra por
 *     letra, o esqueleto de um programa com 3 threads.
 *
 * -> Fim dos fundamentos. Vá para o "00 - COMECE AQUI.md" e siga para o
 *    tutorial de pthreads.
 * ========================================================================= */
