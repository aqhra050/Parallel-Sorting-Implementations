#include "bubbleSort.h"

int main(int argc, char* argv[]) {

    char* inputFileName;//this is the file from which records are to be read
    char* fifoInfoFileName;//this is the file wherein the information for the partial sort will be written to
    int startingRange;//starting range for records
    int endingRange;//ending range for records
    int attrNum;//attributeNumber
    char sortingOrder;//sorting oder
    int workerNumber;//wokerNumber
    pid_t rootPID;

    //the variables needed for timing details
    double t1, t2, cpu_time, real_time, ticspersec;
    struct tms tb1, tb2;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times(&tb1);

    // printf("BUBBLESORT PROCESS\n");


    //parsing the commandLine arguements, storing the values from command line into the above set of variables
    parseSortCommandLineArguement(argc, argv, &inputFileName, &startingRange, &endingRange, &attrNum, &sortingOrder, &workerNumber, &fifoInfoFileName, &rootPID);

    //this will be an array containing the splice of records starting from record # starting range until record # endingRange which is excluded, and will be sorted later
    struct UsrRecord* unsortedUsrRecordSplice = arrayMake(inputFileName, startingRange, endingRange, workerNumber);

    int spliceLength = endingRange - startingRange;//this is the length of the array, and will prove useful for sorting

    struct UsrRecord** sortedPtrsUsrRecordSplice;//this will store an array of pointers to UsrRecords which have been sorted per the required parameters via a bubbleSort 

    // printf("Sorting in order of %s ", sortingOrder == 'a' ? "Ascending" : "Descending");
    // attributeNameDisplay(attrNum);

    sortedPtrsUsrRecordSplice = bubbleSortHelper(unsortedUsrRecordSplice, spliceLength, attrNum, sortingOrder == 'a' ? 2: 4);//the function will return a sorted array of ptrs where the sorting of the ptrs has been ddone with respect to the attrnum and sorting order

    //each sort process will be writing to two pipes, one that it will contain only data of the records, and the other will contain timing information, writing to the latter pipe will occur after data has been transmited

    //this is taking the time after sorting has been completed and computing two different measures
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -(tb1.tms_utime + tb1.tms_stime));
    cpu_time = cpu_time / ticspersec;
    real_time = (t2 - t1) / ticspersec;

    //this is the fifo used to send records from the sort to the merge, being opened
    int sortedResultUploadFD = open(RESULTFIFO, O_WRONLY);
    if (sortedResultUploadFD == -1) {
        fprintf(stderr, "%s FIFO could not be open in Bubble Sort %d; %s\n", RESULTFIFO, workerNumber, strerror(errno));
        exit(21);
    }

    //this will write out every elemment of the sorted array of ptrs, note that a unique kind of entry is sent from the sort to the merge, this entry contains the sorter number, the record number, as well as the content of the record itself, in this way all the sorters write out to the same file, but it can be identified where each record came from, so that this data can then be placed in the correct array in merge, and partialSorted records remain partially sorted
    for (int recordCount = 0; recordCount < spliceLength;) {
        struct DataTransferRecord temp;
        temp.recordIndex = recordCount;
        temp.sorterIndex = workerNumber;
        temp.totalRecordCount = spliceLength;
        memcpy(&temp.tRecord, sortedPtrsUsrRecordSplice[recordCount], sizeof(struct DataTransferRecord));

        if (write(sortedResultUploadFD, &temp, sizeof(struct DataTransferRecord)) == sizeof(struct DataTransferRecord)) {//if the write is successful increment, the record count
            // printf("@%d #%d \t", temp.sorterIndex, recordCount);
            // // printUsrRecord(&temp.tRecord);
            // if (!(recordCount % 1000)) {
            //     printf("S(%d, %d)", workerNumber, recordCount);
            // }
            recordCount++;;
        } else {//otherwise sleep for a bit before trying againg
            usleep(1000);
        }
    }
    //after writing everything that need be written, close the file
    close(sortedResultUploadFD);

    //this will write out the Timing structure into the timing pipe (TIMINGFIFO), the timing information for each sort process will be identified by a workerNumber, and this represents the time taken in sorting alone
    int timingUploadFD = open(TIMINGFIFO, O_WRONLY);
    if (timingUploadFD == -1) {
        fprintf(stderr, "%s FIFO could not be open in Bubble Sort %d; %s\n", TIMINGFIFO, workerNumber, strerror(errno));
        exit(33);
    }
    struct Timing timeInfo = {workerNumber, real_time, cpu_time};
    if (write(timingUploadFD, &timeInfo, sizeof(struct Timing)) != sizeof(struct Timing)) {
        fprintf(stderr, "%s FIFO could not be written for timing info of Bubble Sort %d\n", TIMINGFIFO, workerNumber);
        exit(34);
    } 
    close(timingUploadFD);
        
    //all dynamically allocated memory must be freed
    free(inputFileName);
    free(fifoInfoFileName);
    free(unsortedUsrRecordSplice);
    free(sortedPtrsUsrRecordSplice);

    // srand(time(0));
    // int microWait = rand() % 10000;//a random amount of small wait is being introduced to prevent the clash of signals
    // usleep(microWait);
    kill(rootPID, SIGUSR1);//sending the SIGUSR1 signal to root   

    exit(0);    
}

struct UsrRecord** bubbleSortHelper(struct UsrRecord* arrayUsrRecords, int lenArray, int argNum, int compMode) {
    //this generates an array of pointer to UsrRecords, and it is these pointers that shall be sorted based on the value their dereferenced data takes and the present values of argNum and compMode
    //it is more effecient to sort an array of pointers to some data, then sort that data when data is a compound structure such as this
    struct UsrRecord** UsrRecordsPtr = malloc(lenArray * sizeof(struct UsrRecord*));
    for (int i = 0; i < lenArray; i++) {
        UsrRecordsPtr[i] = &arrayUsrRecords[i];
    }
    //this array of pointers is then sorted in bubbleSort, and the sorted array returned 
    bubbleSort(UsrRecordsPtr, lenArray, argNum, compMode);
    return UsrRecordsPtr;
}


void    bubbleSort(struct UsrRecord** arrayUsrRecords, int lenArray, int argNum, int compMode) { 
    for (int i = 0; i < lenArray; i++) {
        //everytime we traverse  through the outerloop, there is one less element we need worry about
        int innerLoopLimit = lenArray - 1 - i;
        for (int j = 0; j < innerLoopLimit ; j++) {
            //note that we make use of comparator function which in turn performs the appropiate comparison using the arg num and compMode, the compMode will be selected when the bubbleSortHelper and corresponds to the operations of ==, !=, >, >=, <, <=
            if (comparator(arrayUsrRecords[j], arrayUsrRecords[j + 1], argNum, compMode)) {
                //this has the moving towards the right the largest or smallest varying upon whether the compMode corresponds to an ascending or a descending case
                struct UsrRecord* temp = arrayUsrRecords[j];
                arrayUsrRecords[j] = arrayUsrRecords[j + 1];
                arrayUsrRecords[j + 1] = temp;                
            }
        }
    }
}