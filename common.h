#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <mpi.h>

#define MAXLEN 1000
#define TAG_BLOCK 'b'
#define TAG_REQUEST 'r'
#define TAG_PW 'w'
#define SLAVE_BUSY 1
#define SLAVE_IDLE 0

void master(int fd_wordlist, int block_size);
void slave(void);
ssize_t read_failsafe(int fd, void* buf, size_t len);
ssize_t write_failsafe(int fd, void* buf, size_t len);
