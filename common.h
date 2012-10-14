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
#include <time.h>
#include <mpi.h>

#define MAXLEN 1000
#define TAG_BLOCK 'b'
#define TAG_REQUEST 'r'
#define TAG_PW 'w'
#define SLAVE_BUSY 1
#define SLAVE_IDLE 0

void master(int fd_wordlist, int fd_key_file, int block_size);
void slave(void);
ssize_t read_failsafe(int fd, void* buf, size_t len);
ssize_t write_failsafe(int fd, void* buf, size_t len);
void time_diff_nsec(struct timespec *start, struct timespec *end, long *diff_nsec);
