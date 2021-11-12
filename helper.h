#ifndef HELPER
#define HELPER

#define MAXNAMELEN 20

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

#define RESULTFIFO "/tmp/results"
#define TIMINGFIFO "/tmp/timings"


char* integerToString(int num);//will convert an integer into a string

int numLinesInFiles(char fileName[]);//will read the number of lines in a file


//these structures are used across all the programs 
//used to store the UsrRecord
struct UsrRecord {
    unsigned int residentID;
    char firstName[MAXNAMELEN];
    char lastName[MAXNAMELEN];
    unsigned int numDependents; //one cannot reasonably expect for someone to have more than 4,000,000
    double income;//income is given to nearest .10 for some reason
    unsigned int postalCode;//postal code should be positive and less than 4,000,000     
};
//used to store together workerID and recordsSent, which is useful in the mergerNode
struct Message {
    int     workerID;
    int     recordsSent;
};
//this is used for the transmission of timing information from sort to merge
struct Timing {
    int     workerID;
    double  realTime;
    double  cpuTime;
};
//this is used to transfer the record from sort to merge
struct DataTransferRecord {
    int recordIndex;
    int sorterIndex;
    int totalRecordCount;
    struct UsrRecord tRecord;    
};


void printUsrRecord(struct UsrRecord* UsrRecordPtr); //will be used to display the contents of the tax record
 
bool comparator(struct UsrRecord* lTaxRecrodPtr, struct UsrRecord* rUsrRecordPtr, int comparisonAttribute, int comparisonMode);//will be used to compare the datafields for the left record ptr with that of the right using the given comparisonAttribute field and the comparsion 

char* generateFileName(char header[], int num); //will be used to generate fileNames, appending the num to header and return the string for as much

#endif