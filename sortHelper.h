#ifndef SORTHELPER
#define SORTHELPER

#include "helper.h"
#include <time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

struct UsrRecord* arrayMake(char fileName[], int startRecordIndex, int endRecordIndex, int workerNumber);

void parseSortCommandLineArguement(int argc, char* argv[], char** inputFilePtr, int* startingRangePtr, int* endingRangePtr, int* attrNumPtr, char* sortingOrderPtr, int* workerNumberPtr, char** fifoInfoFilePtr, pid_t* rootPIDPtr);

void attributeNameDisplay(int attrNum);


#endif