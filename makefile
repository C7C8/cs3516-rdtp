CFLAGS=-g -Wall -lz

all: rdtp

tarsubmit: rdtp output-trace-50k.txt 
	tar -cvzf crmyers_project2_ABP.tar.gz *.c *.h makefile README.txt output-trace-50k.txt

rdtp: project2.o student2.o
	gcc ${CFLAGS} -o rdtp project2.o student2.o

project2.o: project2.h project2.c
	gcc ${CFLAGS} -c project2.c

student2.o: student2.c project2.h
	gcc ${CFLAGS} -c student2.c

output-trace-50k.txt: rdtp
	rm -f output-trace-50k.txt
	echo "/** GENERATED BY PARAMS: 50000 0.25 0.25 0.25 500 1 1 0 **/" > output-trace-50k.txt
	./rdtp 50000 0.25 0.25 0.25 500 1 1 0 >> output-trace-50k.txt

clean:
	rm -f *.o rdtp
