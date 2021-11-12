#include "mergeSort.h"

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

    // printf("MERGESORT PROCESS\n");

    //parsing the commandLine arguements, storing the values from command line into the above set of variables
    parseSortCommandLineArguement(argc, argv, &inputFileName, &startingRange, &endingRange, &attrNum, &sortingOrder, &workerNumber, &fifoInfoFileName, &rootPID);

    //this will be an array containing the splice of records starting from record # starting range until record # endingRange which is excluded
    struct UsrRecord* unsortedUsrRecordSplice = arrayMake(inputFileName, startingRange, endingRange, workerNumber);
    
    int spliceLength = endingRange - startingRange;//this is the length of the array useful for sorting

    struct UsrRecord** sortedPtrsUsrRecordSplice;//this will store an array of pointers to UsrRecords which have been sorted per the required parameters via a bubbleSort 

    // printf("Sorting in order of %s ", sortingOrder == 'a' ? "Ascending" : "Descending");
    // attributeNameDisplay(attrNum);

    sortedPtrsUsrRecordSplice = mergeSortHelper(unsortedUsrRecordSplice, spliceLength, attrNum, sortingOrder == 'a' ? 4: 2);//the function will return a sorted array of ptrs where the sorting of the ptrs has been ddone with respect to the attrnum and sorting order

    //each sort process will be writing to two pipes, one that it will contain only data of the records, and the other will contain timing information, writing to the latter pipe will occur after data has been transmited
    


    //this is taking the time after only sorting has been completed and computing two different measures
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -(tb1.tms_utime + tb1.tms_stime));
    cpu_time = cpu_time / ticspersec;
    real_time = (t2 - t1) / ticspersec;

    //this is the fifo used to send records from the sort to the merge, being opened
    int sortedResultUploadFD = open(RESULTFIFO, O_WRONLY);
    if (sortedResultUploadFD == -1) {
        fprintf(stderr, "%s FIFO could not be open in Merge Sort %d; %s\n", RESULTFIFO, workerNumber, strerror(errno));
        exit(21);
    }

    //this will write out every elemment of the sorted array of ptrs, note that a unique kind of entry is sent from the sort to the merge, this entry contains the sorter number, the record number, as well as the content of the record itself, in this way all the sorters write out to the same file, but it can be identified where each record came from, so that this data can then be placed in the correct array in merge, and partialSorted records remain partially sorted
    for (int recordCount = 0; recordCount < spliceLength;) {
        struct DataTransferRecord temp;
        temp.recordIndex        = recordCount;
        temp.sorterIndex        = workerNumber;
        temp.totalRecordCount   = spliceLength;
        memcpy(&temp.tRecord, sortedPtrsUsrRecordSplice[recordCount], sizeof(struct DataTransferRecord));

        if (write(sortedResultUploadFD, &temp, sizeof(struct DataTransferRecord)) == sizeof(struct DataTransferRecord)) {//if the write is successful increment, the record count
            // printf("#%d ", recordCount);
            // printUsrRecord(&temp.tRecord);
            recordCount++;
            // if ((recordCount % 1000) == 0) {
            //     printf("S(%d, %d)", workerNumber, recordCount);
            // }
        } else {//otherwise sleep for a bit before trying againg
            usleep(1000);
        }
    }
    //after writing everything that need be written, close the file
    close(sortedResultUploadFD);

    //this will write out the Timing structure into the timing pipe (TIMINGFIFO), the timing information for each sort process will be identified by a workerNumber, and this represents the time taken in sorting alone
    int timingUploadFD = open(TIMINGFIFO, O_WRONLY);
    if (timingUploadFD == -1) {
        fprintf(stderr, "%s FIFO could not be open in Merge Sort %d; %s\n", TIMINGFIFO, workerNumber, strerror(errno));
        exit(33);
    }
    struct Timing timeInfo = {workerNumber, real_time, cpu_time};
    if (write(timingUploadFD, &timeInfo, sizeof(struct Timing)) != sizeof(struct Timing)) {
        fprintf(stderr, "%s FIFO could not be written for timing info of Merge Sort %d\n", TIMINGFIFO, workerNumber);
        exit(34);
    } 
    close(timingUploadFD);
       
    //all dynamically allocated  memory must be freed
    free(inputFileName);
    free(fifoInfoFileName);
    free(unsortedUsrRecordSplice);
    free(sortedPtrsUsrRecordSplice);


    // srand(time(0));
    // int microWait = (rand() % 200) * 100000;
    // usleep(microWait);
    kill(rootPID, SIGUSR1);//sending the SIGUSR1 signal to root   

    //the sort will sort records in the range [startingRecord, endRecord)
    exit(0);
}

struct UsrRecord** mergeSortHelper(struct UsrRecord* arrayUsrRecords, int lenArray, int argNum, int compMode) {
    //this generates an array of pointer to UsrRecords, and it is these pointers that shall be sorted based on the value their dereferenced data takes and the present values of argNum and compMode
    //it is more effecient to sort an array of pointers to some data, then sort that data when data is a compound structure such as this
    struct UsrRecord** UsrRecordsPtr = malloc(lenArray * sizeof(struct UsrRecord*));
    for (int i = 0; i < lenArray; i++) {
        UsrRecordsPtr[i] = &arrayUsrRecords[i];
    }
    //this array of pointers is then sorted in mergeSort using the merge sort algorithm, and the sorted array returned 
    mergeSort(UsrRecordsPtr, 0, lenArray - 1, argNum, compMode);
    return UsrRecordsPtr;
}


void    mergeSort(struct UsrRecord** arrayUsrRecords, int leftIndex, int rightIndex, int argNum, int compMode) {
    //this algorithm has been adapted from the previous assignment
    if (leftIndex < rightIndex) {
        //a middle index is found, and then the two halves are independtly sorted before being combined together
        int middleIndex = (leftIndex + rightIndex) / 2;
        mergeSort(arrayUsrRecords, leftIndex, middleIndex, argNum, compMode);
        mergeSort(arrayUsrRecords, middleIndex + 1, rightIndex, argNum, compMode);

        //the two sorted halves of the array will now be stored in two distinct arrays
        int leftHalfLength = middleIndex - leftIndex + 1;
        int rightHalfLength = rightIndex - middleIndex;
        struct UsrRecord* leftHalfSorted[leftHalfLength];
        struct UsrRecord* rightHalfSorted[rightHalfLength];
        for (int i = 0; i < leftHalfLength; i++) {
            leftHalfSorted[i] = arrayUsrRecords[i + leftIndex];
        }
        for (int i = 0; i < rightHalfLength; i++) {
            rightHalfSorted[i] = arrayUsrRecords[i + middleIndex + 1];
        }

        //the larger element between the two sorted halves is added at every turn while the sorted halves each do not reach their end, after one half reaches its end the contents of the other will be simply transferred directly into the sorted array being prepared
        int leftHalfCursor = 0;
        int rightHalfCursor = 0;
        int sortedCursor = leftIndex;

        while (leftHalfCursor < leftHalfLength && rightHalfCursor < rightHalfLength) {
            if (comparator(leftHalfSorted[leftHalfCursor], rightHalfSorted[rightHalfCursor], argNum, compMode)) {
                struct UsrRecord* insertion = leftHalfSorted[leftHalfCursor++];
                arrayUsrRecords[sortedCursor] =  insertion;
                sortedCursor++;
            } else {
                struct UsrRecord* insertion = rightHalfSorted[rightHalfCursor++];
                arrayUsrRecords[sortedCursor] =  insertion;
                sortedCursor++;
            }
        }
        while (leftHalfCursor < leftHalfLength) {
            struct UsrRecord* insertion = leftHalfSorted[leftHalfCursor++];
            arrayUsrRecords[sortedCursor] =  insertion;
            sortedCursor++;
        }
        while (rightHalfCursor < rightHalfLength) {
            struct UsrRecord* insertion = rightHalfSorted[rightHalfCursor++];
            arrayUsrRecords[sortedCursor] =  insertion;
            sortedCursor++;
        }
    }
}