all: lab6

lab6: lab6.o
	gcc -o lab6 lab6.o -lm

gdb:
	gcc lab6.c -g -o lab6 -lm

clean:
	rm -f lab6 lab6.o

run:
	./lab6