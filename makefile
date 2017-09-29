CFLAGS=-g -Wall -lz

all: rdtp

rdtp: project2.o student2.o
	gcc ${CFLAGS} -o rdtp project2.o student2.o

project2.o: project2.h project2.c
	gcc ${CFLAGS} -c project2.c

student2.o: student2.c project2.h
	gcc ${CFLAGS} -c student2.c

clean:
	rm -f *.o rdtp
