/**
 * Have MPI ranks repeatedly write an amount of bytes to a file, followed
 * by a pause. This is meant to emulate the IO behaviour of periodic
 * applications. Each rank writes to a different file. The process stops
 * at the end of the configured duration.
 *
 * Equivalent to ior -F -w
 **/


#include <errno.h>
#include <getopt.h>           /* getopt_long */
#include <limits.h>           /* INT_MAX */
#include <stdio.h>            /* (f)printf */
#include <stdlib.h>           /* strtol, mkdtemp */
#include <string.h>           /* strerror */
#include <time.h>             /* timespec */
#include <unistd.h>           /* close, unlink */

#include <mpi.h>
#include "tmio_c.h"

#define TIMESPEC_GET(t)                                                 \
  if (clock_gettime(CLOCK_MONOTONIC, &(t))) {                           \
    fprintf(stderr, "clock_gettime error: %s\n", strerror(errno));      \
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);                            \
  }

#define TIMESPEC_DIFF(end,start,r) {                    \
    (r).tv_sec = (end).tv_sec - (start).tv_sec;         \
    (r).tv_nsec = (end).tv_nsec - (start).tv_nsec;      \
    if ((r).tv_nsec < 0) {                              \
      (r).tv_sec--;                                     \
      (r).tv_nsec += 1000000000L;                       \
    }                                                   \
  }

#define BUFLEN           (1024*1024) /* 1MiB is already 1/8th of the stack */
#define FILEPATH_MAXLEN  256
#define NBYTES_DEFAULT   (16*1024*1024)
#define PAUSE_DEFAULT    2
#define DURATION_DEFAULT 4

static int strtoint_unit(const char *s);
static void usage(char *name);


int
main(int argc, char *argv[])
{
  int nbytes = NBYTES_DEFAULT;
  long pausetime = PAUSE_DEFAULT;
  long totaltime = DURATION_DEFAULT;

  (void)nbytes;

  static struct option longopts[] = {
    { "nbytes", required_argument, NULL, 'n' },
    { "pause",  required_argument, NULL, 'p' },
    { "total",  required_argument, NULL, 't' },
    { NULL,     0,                 NULL,  0  },
  };

  int ch;
  char *p;

  while ((ch = getopt_long(argc, argv, "n:p:t:", longopts, NULL)) != -1) {
    switch (ch) {
     case 'n':
      nbytes = strtoint_unit(optarg);
      if (nbytes == -1) {
        fputs("invalid argument: nbytes\n", stderr);
        exit(EXIT_FAILURE);
      }
      break;
     case 'p':
      pausetime = strtol(optarg, &p, 0);
      if (errno != 0 || p == optarg || *p != '\0') {
        fputs("invalid argument: pause\n", stderr);
        exit(EXIT_FAILURE);
      }
      break;
     case 't':
      totaltime = strtol(optarg, &p, 0);
      if (errno != 0 || p == optarg || *p != '\0') {
        fputs("Invalid argument: total\n", stderr);
        exit(EXIT_FAILURE);
      }
      break;
    case 0:
      continue;
    default:
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (argc - optind < 1) {
    fputs("missing base directory\n", stderr);
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  MPI_Init(&argc, &argv);

  int rc, n, ok = 1, fd = -1;
  int rank, nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  char filepath[FILEPATH_MAXLEN];
  n = snprintf(filepath, FILEPATH_MAXLEN, "%s/writer.XXXXXX", argv[optind]);
  if (n < 0 || n >= FILEPATH_MAXLEN) {
    fputs("error building file name\n", stderr);
    goto fail;
  }
  fd = mkstemp(filepath);
  if (fd == -1) {
    fprintf(stderr, "mkstemp: %s\n", strerror(errno));
    goto fail;
  }

  MPI_File fh = MPI_FILE_NULL;
  rc = MPI_File_open(MPI_COMM_SELF, filepath, MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
  if (rc != MPI_SUCCESS) {
    fprintf(stderr, "MPI_File_open %s: error\n", filepath);
    goto fail;
  }

  MPI_File_set_errhandler(fh, MPI_ERRORS_ARE_FATAL);

  int buf[BUFLEN];
  for (size_t i=0; i<BUFLEN; i++) {
    memcpy(buf+i, &rank, sizeof(int));
  }

  struct timespec start, end, res = { 0 };

  while (totaltime > 0) {
    TIMESPEC_GET(start);
 	sleep(pausetime);
    MPI_File_seek(fh, 0, MPI_SEEK_SET);
    for (size_t i=0; i < nbytes/sizeof(buf); i++) {
      MPI_File_write(fh, buf, BUFLEN, MPI_INT, MPI_STATUS_IGNORE);
    }
    TIMESPEC_GET(end);
    TIMESPEC_DIFF(end, start, res);

    MPI_Barrier(MPI_COMM_WORLD);
    totaltime -= res.tv_sec;

    /* tmio
     * XX tmio probably has a barrier itself */
    iotrace_summary();
  }

end:
  if (fd != -1)
    close(fd);
  if (fh != MPI_FILE_NULL)
    MPI_File_close(&fh);
  if (unlink(filepath)) {
  	fprintf(stderr, "unlink %s: %s\n", filepath, strerror(errno));
  }

  if (!ok) {
   MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  MPI_Finalize();
  return EXIT_SUCCESS;

fail:
  ok = 0;
  goto end;
}

static int
strtoint_unit(const char *s) {
  char *end;
  int sh;

  long val = strtol(s, &end, 0);
  if (errno != 0 || end == s) {
    return -1;
  }

  switch(*end) {
  case 'K':
  case 'k':
    sh=10;
    break;
  case 'm':
  case 'M':
    sh=20;
    break;
  case 'g':
  case 'G':
    sh=30;
    break;
  case '\0':
    sh=0;
    break;
  default:
    return -1;
  }

  /* overflow */
  if (val > INT_MAX>>sh) {
  	return -1;
  }
  return val<<sh;
}

static void
usage(char *name)
{
  fprintf(stderr, "usage: %s [[--nbytes|-n bytes] [--pause|-p pause(s)] [--total|-t duration (s)] directory\n", name);
}
