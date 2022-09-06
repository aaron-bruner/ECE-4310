all: lab1

lab1: lab1.o
	gcc -o avg lab1.o -lrt

gdb:
	gcc -o avg lab1.o -lrt -g

clean:
	rm -f avg avg.o lab1.o slidingWindow.ppm separableFilter.ppm convolution.ppm

run_norm:
	./avg bridge.ppm

run_filter:
	./avg bridge.ppm -s 3

diff:
	@echo "Difference between 2D Convolution Image and Separable Filter Image"
	diff convolution.ppm separableFilter.ppm
	@echo "Difference between 2D Convolution Image and Sliding Window Image"
	diff convolution.ppm slidingWindow.ppm
	@echo "Difference between Separable Filter Image and Sliding Window Image"
	diff separableFilter.ppm slidingWindow.ppm