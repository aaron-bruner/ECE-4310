all: lab3

lab1: lab3.o
	gcc -o lab3 lab3.o

gdb:
	gcc lab3.c -g -o lab3

clean:
	rm -f lab3 lab3.o

run:
	./lab3

file:
	./lab3 parenthood.ppm parenthood_e_template.ppm msf_e.ppm parenthood_gt.txt