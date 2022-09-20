all: lab2

lab1: lab2.o
	gcc -o lab2 lab2.o

gdb:
	gcc -o lab2 lab2.o -g

clean:
	rm -f lab2 lab2.o

run:
	./lab2

file:
	./lab2 parenthood.ppm parenthood_e_template.ppm parenthood_gt.txt