#include "common.h"


int
main (int argc, char **argv)
{
  int len;
  int comm_rank, comm_size, fd_wordlist;
  int block_size = 4096;
  char hostname[MAXLEN], wordlist[MAXLEN], cap_file[MAXLEN],
    cache_prefix[MAXLEN];
  
  
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  MPI_Get_processor_name (hostname, &len);
  /* defaults */
  memcpy (wordlist, "password.lst", MAXLEN);
  memcpy (cap_file, "wpa.cap", MAXLEN);
  memcpy (cache_prefix, "./", MAXLEN);
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
	    {"block_size", required_argument, 0, 'b'},
	    {"cache-prefix", required_argument, 0, 'p'},
	    {0, 0, 0, 0}
	  };
	  int option_index = 0;
	  c = getopt_long (argc, argv, "w:c:b:p:", long_options, &option_index);
	  if (c == -1)
	    break;
	  switch (c)
	    {
	    case 'w':
	      if (strlen(optarg) > MAXLEN) {
		fprintf(stderr, "pathname of wordlist too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      
	      memcpy(wordlist, optarg, strlen(optarg)+1);
	      break;
	    case 'c':
	      if (strlen(optarg) > MAXLEN) {
		fprintf(stderr, "pathname of .cap file too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      memcpy(cap_file, optarg, strlen(optarg)+1);
	      break;
	    case 'p':
	      if (strlen(optarg) > MAXLEN) {
		fprintf(stderr, "cache-prefix too long\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	      }
	      memcpy(cache_prefix, optarg, strlen(optarg)+1);
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
      printf("using block size %d\n",  block_size);
    }
  /* dispatch  */
  if (comm_rank == 0)
    {
      /* broadcast arguments to slaves*/
      MPI_Bcast(cap_file, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
      MPI_Bcast(cache_prefix, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
      MPI_Bcast(&block_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

      master(fd_wordlist, block_size);
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
	    continue; perror ("write_failsafe"); return ret;}
      len_tmp -= ret; buf_tmp += ret;}
  return len - len_tmp;}
