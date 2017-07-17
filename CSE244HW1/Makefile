all: list

list: list.o
	gcc list.o -o list

main.o: list.c
	gcc -c list.c

clean:
	rm *.o list

