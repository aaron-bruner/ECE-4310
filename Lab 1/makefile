all: lab1

lab1: lab1.o
	gcc -o avg lab1.o

gdb:
	gcc -o avg lab1.o -g

clean:
	rm -f avg avg.o

run_norm:
	./avg bridge.ppm 234 321

run_filter:
	./avg bridge.ppm 324 65 -s 3