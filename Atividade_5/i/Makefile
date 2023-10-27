prog: get.o main.o
	gcc -o prog get.o main.o

get.o: get.c
	gcc -c get.c

main.o: main.c
	gcc -c main.c

clean:
	rm -f prog *.o
