all: lab8

lab8: lab8.o
	gcc -o lab8 lab8.o -lm

gdb:
	gcc lab8.c -g -o lab8 -lm

clean:
	rm -f lab8 lab8.o 

run:
	./lab8 chair-range.ppm