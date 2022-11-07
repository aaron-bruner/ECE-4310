all: lab5

lab5: lab5.o
	gcc -o lab5 lab5.o -lm

gdb:
	gcc lab5.c -g -o lab5 -lm

clean:
	rm -f lab5 lab5.o hawk_final* hawk_norm* hawk_source* coordinates.txt
	rm -f -r images

run:
	./lab5
	mkdir -p images
	mv hawk_final* images/
	mv hawk_normalized.ppm images/
	mv hawk_sourceArrows.ppm images/

file:
	./lab5 hawk.ppm hawk_init.txt