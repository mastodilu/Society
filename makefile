gestore : header.o gestore.o a b
	gcc -Wall -pedantic -ggdb -o gestore gestore.o header.o

gestore.o : header.h gestore.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb gestore.c header.c

header.o : header.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb header.c

a : header.o a.o
	gcc -Wall -pedantic -ggdb -o a a.o header.o

b : header.o b.o
	gcc -Wall -pedantic -ggdb -o b b.o header.o

a.o : header.h a.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb a.c header.c

b.o : header.h b.c
	gcc -c -Wall -Wextra -Wconversion -std=gnu11 -ggdb b.c header.c