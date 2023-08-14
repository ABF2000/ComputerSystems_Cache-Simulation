# GCC=gcc -g -Wall -Wextra -pedantic -std=gnu11 
GCC=gcc -g -Wall -pedantic -std=gnu11 -O

all: simulate
rebuild: clean all

simulate: simulate.c
	$(GCC) *.c -o simulate -lm

zip: ../src.zip

../src.zip: clean
	cd .. && zip -r src.zip src/Makefile src/*.c src/*.h

clean:
	rm -rf *.o simulate  vgcore*
