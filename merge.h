#ifndef MERGE
#define MERGE

//for personal helpful functions
#include "helper.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

//for the system programming things
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

struct UsrRecord** generateSortedArray(struct Message sortMessages[], struct UsrRecord** sortedSplices, int totalRecordsAmount, int numWorkers, int argNum, int compMode);//check .c file for description 

#endif