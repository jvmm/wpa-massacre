#include "common.h"
void slave(void) {
  int fd_cap_file, fd_cache_file;
  int comm_rank, comm_size, len;
  size_t block_size;
  char cap_file[MAXLEN], cache_prefix[MAXLEN], hostname[MAXLEN], cache_file[MAXLEN], cache_postfix[MAXLEN];
  MPI_Comm_rank (MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
  MPI_Get_processor_name (hostname, &len);
  MPI_Bcast(cap_file, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Bcast(cache_prefix, MAXLEN, MPI_CHAR, 0, MPI_COMM_WORLD);
  MPI_Bcast(&block_size, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
  fprintf(stderr, "rank %d using block size %u\n", comm_rank, block_size);
  if ((fd_cap_file = open(cap_file, O_RDONLY)) == -1) {
    fprintf(stderr, "could not open .cap file %s on rank %d\n", cap_file, comm_rank); 
    perror(NULL);
  }
  /* compose the name of the cache file for this rank */
  memcpy(&cache_prefix[strlen(cache_prefix)], "cache-file-", strlen("cache-file-"));
  memcpy(cache_file, cache_prefix, strlen(cache_prefix));
  sprintf(cache_postfix, "%d", comm_rank);
  memcpy(&cache_file[strlen(cache_prefix)], cache_postfix, strlen(cache_postfix) + 1); /* we need the "+1" for the trailing null character */
  fprintf(stderr, "full path to cache file on rank %d is\n%s\n", comm_rank, cache_file);
  MPI_Finalize();
}
