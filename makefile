all: escalona
 
escalona: escalona.o
	gcc -o escalona escalona.o

escalona.o: escalona.c
	gcc -c escalona.c
 
clean:
	rm -rf *.o *~

purge: clean
	rm escalona