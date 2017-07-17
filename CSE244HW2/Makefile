all: exe

exe: listdir.o
	gcc listdir.o -o exe

main.o: listdir.c
	gcc -c listdir.c -g -Wall -Wextra -ansi -pedantic-errors

clean:
	rm *.o exe

