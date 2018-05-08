gestore : header.o gestore.o 
	gcc -Wall -pedantic -ggdb -o gestore gestore.o header.o

gestore.o : header.h gestore.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb gestore.c header.c

header.o : header.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb header.c
