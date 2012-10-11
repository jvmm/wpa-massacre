a.out : main.o slave.o master.o 
	mpicc -g -Wall main.o slave.o master.o
main.o : main.c common.h
	mpicc -c -g -Wall main.c
slave.o : slave.c common.h
	mpicc -c -g -Wall slave.c
master.o : master.c common.h
	mpicc -c -g -Wall master.c
clean : 
	rm a.out main.o slave.o master.o