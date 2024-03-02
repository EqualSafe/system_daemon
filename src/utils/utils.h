#ifndef UTILS
#define UTILS
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct popen2 {
    pid_t child_pid;
    int   from_child, to_child;
} popen2_t;

int popen2(const char *cmdline, struct popen2 *childinfo);

#endif