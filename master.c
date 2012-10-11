#include "common.h"

void master(int fd_wordlist, size_t block_size)
/* read in wordlist and scatter it to the workers */
{
  /* check for empty wordlist */
  off_t position;
  char *block = NULL;
  int *slave_table = NULL;
  int comm_size, i;
  if ((block = malloc(block_size)) == NULL) {
    perror("malloc");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  if ((slave_table = malloc(comm_size*sizeof(int))) == NULL) {
    perror("malloc");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  memset(slave_table, SLAVE_IDLE, comm_size*sizeof(int));
  for (i=0; i<comm_size; ++i) {
    printf("slave=%d\n",slave_table[i]);
  }
  if ((position = lseek(fd_wordlist, 0, SEEK_END)) ==  -1) {
    perror("lseek");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  else {
    if (position == 0) {
      fprintf(stderr, "wordlist is empty!\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else 
      if (lseek(fd_wordlist, 0, SEEK_SET) == -1) {
	perror("lseek");
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
  }
  /* start main loop */
  while(1) {
    
    int i, dummy;
    MPI_Status status_probe, status;
    ssize_t received_bytes;
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status_probe);
    if (status_probe == TAG_REQUEST) {
      MPI_Recv(&dummy, 1, MPI_INT, status_probe.MPI_SOURCE, status_probe.MPI_TAG, MPI_COMM_WORLD, &status);
      slave_table[status.MPI_SOURCE-1] = SLAVE_IDLE;
      /* read in block and send to slave */
      received_bytes = read_failsafe(fd_wordlist, block, block_size);
      if (received_bytes == -1) {
	perror("reading wordlist");
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      if (received_bytes == 0) {
	/* reached end of file */
	fprintf(stderr, "reached end of wordlist\nwaiting for slaves...\n");
	for (i = 0; i < comm_size; ++i) {
	  if (slave_table[i] == SLAVE_BUSY) {
	    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status_probe);
	  }
      }
      /* truncate block to last newline */
      for (i = received_bytes - 1; i >= 0; --i) {
	if (block[i] == '\n') break;
	if (i == 0) {
	  fprintf(stderr, "error: a passphrase is longer than the block size");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
      }
      if (lseek(fd_wordlist, (block_size-1) - i , SEEK_CUR) == -1) {
	perror("lseek");
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      /* now we know that we only want to send i+1 bytes */
      MPI_Send(block, i+1, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
      slave_table[status.MPI_SOURCE - 1] = SLAVE_BUSY;
      
      }
    }
  }
      
}
