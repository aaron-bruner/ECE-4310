all: lab1

lab1: lab1.o
	gcc -o avg lab1.o -lrt

gdb:
	gcc -o avg lab1.o -lrt -g

clean:
	rm -f avg avg.o

run_norm:
	./avg bridge.ppm

run_filter:
	./avg bridge.ppm -s 3