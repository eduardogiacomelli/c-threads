# Compila e roda um passo por vez:
#
#     make 01      ... make 37
#     make 17 ARGS="1e9 4"     passa argumentos pra linha de comando
#     make todos   compila tudo (não roda)
#     make limpar  apaga os binários
#
# Dentro do VS Code você não precisa disso: abra o .c e aperte Ctrl+Shift+B.
# Os sanitizers estão LIGADOS. Sem eles, um ponteiro errado dá só
# "Segmentation fault". Com eles, você recebe arquivo, linha e o motivo.

CC     := gcc
# -pthread matches the VS Code task, so `make` and Ctrl+Shift+B build the
# same way. Harmless for the steps that use no threads.
CFLAGS := -std=gnu17 -Wall -Wextra -g -fsanitize=address,undefined -pthread

FONTES := $(wildcard passo-*.c)
PROGS  := $(patsubst %.c,%,$(FONTES))

# Qualquer .c que não seja um passo é um módulo (contas.c) e entra na
# linkagem. É o que faz o passo-19 funcionar.
MODULOS := $(filter-out $(FONTES),$(wildcard *.c))

PASSOS := 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 \
          20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37

.PHONY: todos limpar $(PASSOS)

todos: $(PROGS)

%: %.c
	@$(CC) $(CFLAGS) -o $@ $< $(MODULOS)

$(PASSOS):
	@f=$$(ls passo-$@-*.c 2>/dev/null | head -1); \
	if [ -z "$$f" ]; then echo "não existe passo-$@"; exit 1; fi; \
	echo "=== $$f ==="; \
	$(CC) $(CFLAGS) -o "$${f%.c}" "$$f" $(MODULOS) && ./"$${f%.c}" $(ARGS)

limpar:
	@rm -f $(PROGS)
