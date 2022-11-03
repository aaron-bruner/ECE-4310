all: lab5

lab5: lab5.o
	gcc -o lab5 lab5.o -lm

gdb:
	gcc lab5.c -g -o lab5 -lm

clean:
	rm -f lab5 lab5.o

run:
	./lab5

file:
	./lab5 hawk.ppm hawk_init.txt