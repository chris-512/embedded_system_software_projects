#ifndef _SETTING_HEADER_
#define _SETTING_HEADER_

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "macros.h"

#define _GNU_SOURCE

typedef enum { INPUT_PROCESS, OUTPUT_PROCESS } ProcessType;
typedef enum { INPUT_STREAM, OUTPUT_STREAM } StreamType;
typedef enum { IN2MAIN, MAIN2OUT } PipeType;
typedef enum { MODE_NEXT, MODE_PREV, MODE_RETURN } ModeType;

typedef ModeType (*wrapperFunc)(int **ipc_pipe);
typedef int BOOL;

#endif
