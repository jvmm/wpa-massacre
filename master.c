/* Copyright (C) 2012 wpa-massacre team */
/* This file is part of wpa-massacre */
/* wpa-massacre is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* wpa-massacre is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "common.h"

void master(int fd_wordlist, int fd_key_file,  int block_size)
/* read in wordlist and scatter it to the workers */
{

  off_t size_of_wordlist;
  char *block = NULL;
  int *slave_table = NULL;
  int comm_size;
  char passphrase[MAXLEN];
  struct timespec  started_read, finished_read ;
  long read_time_nsec, j=0;
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
  /* check for empty wordlist */
  if ((size_of_wordlist = lseek(fd_wordlist, 0, SEEK_END)) ==  -1) {
    perror("lseek");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  else {
    if (size_of_wordlist == 0) {
      fprintf(stderr, "error: wordlist is empty!\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else {
      if (lseek(fd_wordlist, 0, SEEK_SET) == -1) {
	perror("lseek");
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      fprintf(stderr, "using %lld byte wordlist\n", (long long) size_of_wordlist);
    }
  }
  
    /*******************/
    /* start main loop */
    /*******************/
    while(1) {
    
      int i, dummy;
      MPI_Status status_probe;
      ssize_t read_bytes;
      ++j;
      if (j%comm_size == 0) {
	started_read = finished_read;
	if (clock_gettime (CLOCK_MONOTONIC, &finished_read) == -1) {
	  perror("clock_gettime");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
	time_diff_nsec(&started_read, &finished_read, &read_time_nsec);
	printf("time for cycle=%.10lf sec\n", (double)read_time_nsec / 1000000000 / comm_size);
	printf("MB/sec = %lf\n", ((double) block_size / 1000000)/ comm_size / ((double) read_time_nsec/1000000000));
      }
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status_probe);
      if (status_probe.MPI_TAG == TAG_REQUEST) {
	MPI_Recv(&dummy, 1, MPI_INT, status_probe.MPI_SOURCE, status_probe.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	slave_table[status_probe.MPI_SOURCE] = SLAVE_IDLE;
	/* read in block and send to slave */

	read_bytes = read_failsafe(fd_wordlist, block, block_size);

	if (read_bytes == -1) {
	  perror("reading wordlist");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
	
	if (read_bytes == 0) {
	  
	  fprintf(stderr, "reached end of wordlist\nwaiting for slaves...\n");
	  /* collect busy slaves */
	  for (i = 1; i < comm_size; ++i) {
	    if (slave_table[i] == SLAVE_BUSY) {
	      MPI_Probe(i, MPI_ANY_TAG, MPI_COMM_WORLD, &status_probe);
	      if (status_probe.MPI_TAG == TAG_REQUEST) {
		MPI_Recv(&dummy, 1, MPI_INT, status_probe.MPI_SOURCE, status_probe.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		fprintf(stderr, "slave %d finished\n",i);
	      }
	      else if (status_probe.MPI_TAG == TAG_PW) {
		MPI_Recv(passphrase, MAXLEN, MPI_CHAR, status_probe.MPI_SOURCE, status_probe.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("rank %d found passphrase:\n%s\n", i, passphrase);
		if (write_failsafe(fd_key_file, passphrase, strlen(passphrase)) == -1) {
		  perror("write to keyfile");
		}
		MPI_Abort(MPI_COMM_WORLD, 0);
	      }
	      else {
		fprintf(stderr, "bad tag\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	    }	      
	  }
      
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
	/* truncate block to last newline */
	if (lseek(fd_wordlist, 0, SEEK_CUR) != size_of_wordlist) {
	  for (i = read_bytes - 1; i >= 0; --i) {
	    if (block[i] == '\n') break;
	    if (i == 0) {
	      fprintf(stderr, "error: a passphrase is longer than the block size\n");
	      MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	  }
	}
	else {
	  /* printf("EOF reached read_bytes=%lld\n", (long long) read_bytes); */
	  /* printf("block is %s\n\n\n",block); */
	  i = read_bytes -1;
	}
	if (lseek(fd_wordlist, (i+1) - read_bytes , SEEK_CUR) == -1) {
	  perror("lseek");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
	/* now we know that we only want to send i+1 bytes */
	MPI_Send(block, i+1, MPI_CHAR, status_probe.MPI_SOURCE, 1, MPI_COMM_WORLD);
	slave_table[status_probe.MPI_SOURCE] = SLAVE_BUSY;
      }
      
      else if (status_probe.MPI_TAG == TAG_PW) {
	MPI_Recv(passphrase, MAXLEN, MPI_CHAR, status_probe.MPI_SOURCE, status_probe.MPI_TAG, MPI_COMM_WORLD, &status_probe);
	printf("rank %d found passphrase:\n%s\n",status_probe.MPI_SOURCE, passphrase);
	if (write_failsafe(fd_key_file, passphrase, strlen(passphrase)) == -1) {
	  perror("write to keyfile");
	}
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      
    }
  }
   
void time_diff_nsec(struct timespec *start, struct timespec *end, long *diff_nsec)
{
  *diff_nsec =  (long) end->tv_sec*1000000000 + end->tv_nsec - (long) start->tv_sec*1000000000 - start->tv_nsec;
}

