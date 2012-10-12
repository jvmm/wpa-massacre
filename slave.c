#include "common.h"
void slave(void) {
  int fd_cap_file, fd_cache_file;
  int comm_rank, comm_size, len;
  int block_size;
  char cap_file[MAXLEN], cache_prefix[MAXLEN], hostname[MAXLEN], cache_file[MAXLEN], cache_postfix[MAXLEN], key_file[MAXLEN];
  char *block = NULL;
  /* printf("pid = %d\n",getpid()); */
  /* sleep(10); */
  MPI_Comm_rank (MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  MPI_Get_processor_name (hostname, &len);
  MPI_Bcast(cap_file, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Bcast(cache_prefix, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Bcast(&block_size, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
  fprintf(stderr, "rank %d using block size %d\n", comm_rank,  block_size);
  block = malloc(block_size+1);
  if (block == NULL) {
    perror("malloc block in slave");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  if ((fd_cap_file = open(cap_file, O_RDONLY)) == -1) {
    fprintf(stderr, "could not open .cap file %s on rank %d\n", cap_file, comm_rank); 
    perror(NULL);
  }
  /* compose the name of the cache file for this rank */
  memcpy(&cache_prefix[strlen(cache_prefix)], "cache-file-", strlen("cache-file-"));
  memset(cache_file, '\0', MAXLEN);
  memcpy(cache_file, cache_prefix, strlen(cache_prefix));
  sprintf(cache_postfix, "%d", comm_rank);
  memcpy(&cache_file[strlen(cache_prefix)], cache_postfix, strlen(cache_postfix) + 1); /* we need the "+1" for the trailing null character */
  fprintf(stderr, "full path to cache file on rank %d is\n%s\n", comm_rank, cache_file);
  
  if ((fd_cache_file = creat(cache_file, 0600)) == -1) {
    fprintf(stderr, "creat on rank %d\n",comm_rank);
    perror(NULL);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  /* start main loop */
  while(1) {
    /* ask for task */
    int task = 't';
    int len, written;
    MPI_Status status_probe;
    MPI_Send(&task, 1, MPI_INT, 0, TAG_REQUEST, MPI_COMM_WORLD); 
    /* receive block */
    MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status_probe);
    MPI_Get_count(&status_probe, MPI_CHAR, &len);
    MPI_Recv(block, len, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    block[len] = '\n';
    /* write block to cache file */
    if (lseek(fd_cache_file, 0, SEEK_SET) == -1) {
      fprintf(stderr, "error: lseek on rank %d\n", comm_rank);
      perror(NULL);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    written = write_failsafe(fd_cache_file, block, len);
    if (written < len) {
      fprintf(stderr, "error: write on rank %d\n", comm_rank);
      if (written == -1) 
	perror(NULL);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (ftruncate(fd_cache_file, (off_t) len) == -1 ) {
      fprintf(stderr, "error: truncate on rank %d\n", comm_rank);
      perror(NULL);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }    
    /* *************************************  */
    /* unleash aircrack-ng on the cache file  */
    /* *************************************  */
    
    /* assemble the command that will be shelled out to the system */
    char command[MAXLEN];
    memset(command, '\0', MAXLEN);
    memcpy(command, "aircrack-ng -a 2 -w ", strlen("aircrack-ng -a 2 -w "));
    /* wordlist */
    memcpy(&command[strlen(command)], cache_file, strlen(cache_file));
    /* .cap file */
    memset(key_file, '\0', MAXLEN);
    memcpy(key_file, "key_file", strlen("key_file"));
    memcpy(&key_file[strlen(key_file)], cache_postfix, strlen(cache_postfix)+1);
    memcpy(&command[strlen(command)], " -l ", strlen(" -l "));
    memcpy(&command[strlen(command)], key_file, strlen(key_file));
    memcpy(&command[strlen(command)], " ", 1);
    memcpy(&command[strlen(command)], cap_file, strlen(cap_file));
    memcpy(&command[strlen(command)], " 1&>/dev/null", strlen(" 1&>/dev/null"));
    fprintf(stderr, "command on rank %d: %s\n", comm_rank, command);
    unlink(key_file);
    system(command);
    if (!access(key_file, R_OK)) {
      /* found the passphrase */
      /* send password to rank 0*/
      fprintf(stderr, "found password on rank %d\n", comm_rank);
      
    }
	      
  }
  
}
