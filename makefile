MPI_CC = mpicc
CFLAGS = -g -Wall
a.out : main.o slave.o master.o 
	$(MPI_CC) -g -Wall main.o slave.o master.o -lrt
main.o : main.c common.h
	$(MPI_CC) $(CFLAGS) -c $<
slave.o : slave.c common.h
	$(MPI_CC) $(CFLAGS) -c $<
master.o : master.c common.h
	$(MPI_CC) $(CFLAGS) -c $<
clean : 
	rm a.out main.o slave.o master.o