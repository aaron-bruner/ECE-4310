all: lab5

lab5: lab5.o
	gcc -o lab5 lab5.o

gdb:
	gcc lab5.c -g -o lab5

clean:
	rm -f lab5 lab5.o

run:
	./lab5

file:
	./lab5 hawk.ppm hawk_init.txt