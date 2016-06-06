################################################################################
#  Soubor:    Makefile                                                         #
#  Autor:     Radim KUBIŠ, xkubis03                                            #
#  Vytvořeno: 4. dubna 2014                                                    #
#                                                                              #
#  Projekt č. 2 do předmětu Pokročilé operační systémy (POS).                  #
################################################################################

CC=gcc
CFLAGS=-D_REENTRANT -Wall -g -O -lpthread -pedantic -std=c99
NAME=proj02

all:
	$(CC) $(CFLAGS) -o $(NAME) $(NAME).c

run:
	./$(NAME) 10 100

clean:
	rm -f $(NAME)
