/* ============================================================================
 * contas.h — o CONTRATO do módulo. Leia junto com passo-19.
 *
 * Um header não contém código que roda. Ele contém DECLARAÇÕES: as
 * assinaturas das funções que outro arquivo pode chamar. É a lista do que
 * este módulo oferece, e é a única parte que os outros arquivos enxergam.
 *
 * O que fica aqui:  o que os outros precisam saber (assinaturas, structs,
 *                   #define, typedef)
 * O que fica no .c: como as coisas funcionam de verdade
 * ========================================================================= */

/* GUARDA DE INCLUSÃO. Sem estas três linhas, um header incluído duas vezes
 * (direto e através de outro header) faz o compilador ver as mesmas
 * declarações duas vezes.
 *
 * O nome é convenção: o do arquivo, em maiúsculas, com underline.
 * Na primeira vez, CONTAS_H não está definido -> define e segue.
 * Na segunda, já está definido -> o pré-processador pula tudo até o #endif. */
#ifndef CONTAS_H
#define CONTAS_H

/* Soma todos os inteiros de 1 até n. Devolve 0 se n < 1. */
long soma_ate(int n);

/* 1 se n é par, 0 se é ímpar. */
int eh_par(int n);

/* Quantas vezes soma_ate foi chamada desde o início do programa.
 * (Existe só pra mostrar como um módulo guarda estado privado.) */
int quantas_chamadas(void);

#endif /* CONTAS_H */
