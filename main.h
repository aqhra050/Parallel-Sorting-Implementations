#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

//for the system programming things
#include <sys/times.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

//for personal helpful functions
#include "helper.h"

bool commandLineArguementHandling(int argc, char* argv[],  char** inputFilePtr, int* numWorkersPtr, bool* isRandomPtr, int* attrNumPtr, char* orderPtr, char** outputFilePtr);

void sortSigHandler(int sig);  //will be handling the signals from the sort processes
void mergeSigHandler(int sig); //will be handling the signals from the merge process
