#ifndef COORD
#define COORD

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

//for the system programming things
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

//for personal helpful functions
#include "helper.h"

//for function details consult .c file

void parseCommandLineArguements(char* argv[], char** inputFilePtr, int* numWorkersPtr, bool* isRandomPtr, int* attrNumPtr, char** sortingOrderStringPtr, char** outputFilePtr);

void getNumberOfRecords(int* numRecordsPtr, char inputFile[]);

void testingCompartor();

void waitForAllChildNodes(int numNodes, pid_t mergeID);

int* randomRangeSplit(int rangeStart, int rangeEnd, int splitAmount);

int* uniformRangeSplit(int rangeStart, int rangeEnd, int splitAmount);

void testRandomRangeSplit();

#endif
