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


int
main (int argc, char **argv)
{
  int len;
  int comm_rank, comm_size, fd_wordlist, fd_keyfile;
  int block_size = 4096;
  char hostname[MAXLEN], wordlist[MAXLEN], cap_file[MAXLEN],
    cache_prefix[MAXLEN], key_file[MAXLEN];
  
  
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  MPI_Get_processor_name (hostname, &len);
  /* set defaults */
  memcpy (wordlist, "password.lst", strlen("password.lst") + 1);
  memcpy (cap_file, "wpa.cap", strlen("wpa.cap") + 1);
  memcpy (cache_prefix, "./", strlen("./") + 1);
  memcpy (key_file, "key_file", strlen("key_file") + 1);
  /* rank 0 parses command-line arguments */
  if (comm_rank == 0)
    {
      /* printf("pid root = %d\n",getpid()); */
      /* sleep(10); */
      printf ("rank 0 is on node %s\nnumber of workers is %d\n", hostname,
	      comm_size - 1);
      if (comm_size < 2)
	{
	  fprintf (stderr, "wpa-massacre requires at least 2 procs\n");
	  MPI_Abort (MPI_COMM_WORLD, 1);
	}
      int c;
      while (1)
	{
	  static struct option long_options[] = {
	    {"wordlist", required_argument, 0, 'w'},
	    {"cap_file", required_argument, 0, 'c'},
	    {"block-size", required_argument, 0, 'b'},
	    {"cache-prefix", required_argument, 0, 'p'},
	    {"key-file", required_argument, 0, 'l'},
	    {0, 0, 0, 0}
	  };
	  int option_index = 0;
	  c = getopt_long (argc, argv, "w:c:b:p:l:", long_options, &option_index);
	  if (c == -1)
	    break;
	  switch (c)
	    {
	    case 'w':
	      if (strlen(optarg) > MAXLEN-1) {
		fprintf(stderr, "pathname of wordlist too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      
	      memcpy(wordlist, optarg, strlen(optarg)+1);
	      break;
	    case 'c':
	      if (strlen(optarg) > MAXLEN-1) {
		fprintf(stderr, "pathname of .cap file too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      memcpy(cap_file, optarg, strlen(optarg)+1);
	      break;
	    case 'p':
	      if (strlen(optarg) > MAXLEN-1) {
		fprintf(stderr, "cache-prefix too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      memcpy(cache_prefix, optarg, strlen(optarg)+1);
	      break;
	    case 'l':
	      if (strlen(optarg) > MAXLEN-1) {
		fprintf(stderr, "pathname of key-file too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      memcpy(key_file, optarg, strlen(optarg)+1);
	      break;
	    case 'b':
	      errno = 0;
	      block_size = strtol(optarg, NULL, 0);
	      if (errno)
		{
		  perror("strtol");
		  MPI_Abort(MPI_COMM_WORLD, 1);
		}
	      else if (block_size <= 0 || block_size == INT_MAX)
		{
		  fprintf(stderr, "invalid block size\n");
		  MPI_Abort(MPI_COMM_WORLD, 1);
		}
	      
	      break;
	    case '?':
	      break;
	    default:
	      MPI_Abort (MPI_COMM_WORLD, 1);
	    }
	}


      if ((fd_wordlist = open(wordlist, O_RDONLY)) == -1) {
	fprintf(stderr, "%s ", wordlist);
	perror(NULL);
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      if ((fd_keyfile = creat(key_file, 0600)) == -1) {
	fprintf(stderr, "%s ", wordlist);
	perror(NULL);
	MPI_Abort(MPI_COMM_WORLD, 1);
      }
      printf("using block size %d\n",  block_size);
    }
  /* dispatch  */
  if (comm_rank == 0)
    {
      /* broadcast arguments to slaves*/
      MPI_Bcast(cap_file, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
      MPI_Bcast(cache_prefix, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
      MPI_Bcast(&block_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

      master(fd_wordlist, fd_keyfile, block_size);
    }
  else
    {
      slave ();
    }
  return 0; 			/* just to make -Wall happy */

}

ssize_t read_failsafe (int fd, void *buf, size_t len)
/* like read(2) but checks for EINTR and fatal read errors  */
{
  void *buf_tmp;	/* we want to leave buf unchanged */
  ssize_t ret;
  size_t len_tmp = len;
  buf_tmp = buf;
  while (len_tmp != 0 && (ret = read (fd, buf_tmp, len_tmp)) != 0)
    {
      if (ret == -1)
	{
	  if (errno == EINTR) continue; 
	  perror ("read"); 
	  return ret;
	}
      len_tmp -= ret; buf_tmp += ret;
    }
  return len - len_tmp;	/* return number of read bytes */
}

ssize_t write_failsafe (int fd, void *buf, size_t len)
{
  void *buf_tmp;
  ssize_t ret;
  size_t len_tmp = len;
  buf_tmp = buf;
  while (len_tmp != 0
	 && (ret = write (fd, buf_tmp, len_tmp)) != 0)
    {
      if (ret == -1)
	{
	  if (errno == EINTR)
	    continue; 
	  perror ("write_failsafe"); 
	  return ret;
	}
      len_tmp -= ret; 
      buf_tmp += ret;
    }
  return len - len_tmp;
}
