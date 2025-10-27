main: main.o
	gcc main.o -o main

main.o: main.c
	gcc -c main.c -o main.o

.PHONY: clean
clean:
	rm main.o main