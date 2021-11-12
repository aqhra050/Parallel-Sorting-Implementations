#include "merge.h"


int main(int argc, char* argv[]) {

    //storing the arguements without any validation since merge will always be inovked with these arguments from cord
    int numWorkers = atoi(argv[1]);
    int compArg = atoi(argv[2]);
    char* orderMode = argv[3];
    char* outputFileName = argv[4];
    pid_t rootPID = atoi(argv[5]);
    int numRecords = atoi(argv[6]);

    //adapted from the code given for timing, and this takes the start time
    double t1, t2, cpu_time, real_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times(&tb1);//taking the initial time
   
    bool isInfoReadPending[numWorkers];//this will be used to store, whether information on the amount of records a given sorter sorted is yet to be read or not
    memset(isInfoReadPending, 1, numWorkers);//will be true for every sort process (each being associated with a particular index) until read of info is completed

    struct UsrRecord* partialSortedSplices[numWorkers];//this is a two dimensional array that will for each sort process store its sorted records at the relevant index in that the partial sorted results for sort process 0 will be stored in the array at position 0

    struct Message sortMessage[numWorkers];//this is an array that will store the details regarding the total number of records sorted by each sort process
    
    int partialSortedCount[numWorkers];//this is a an array where each element is the amount of records recieved thus far from each sort process
    memset(partialSortedCount, 0, numWorkers * sizeof(int));//this will be zero at the start

    int totalRecords = numRecords;//these are the totalNumber of records to be eventually read and merged
    int recievedRecords = 0;//this is the number of records recieved thus far

    printf("\nPROGRESS:\n");

    //opening the pipe to which all the sorters will be writing 
    int sortedResultRecieveFD = open(RESULTFIFO, O_RDONLY);
    if (sortedResultRecieveFD == -1) {
        fprintf(stderr, "Results FIFO not opening\n");
        exit(31);
    }
    while (recievedRecords < totalRecords) {//we will first recieve all the records, placing the records received from a particular sorter in the order they were sorted in, in a particular array
        struct DataTransferRecord temp;
        int bytesRead = read(sortedResultRecieveFD, &temp, sizeof(struct DataTransferRecord));
        if (bytesRead == sizeof(struct DataTransferRecord)) {//if the read was successful
            if (isInfoReadPending[temp.sorterIndex] == true) {//if this is the first time enocuntering a record from a given sorterIndex, its information must be stored in sortMessage at the appropiate position, and an array of sufficient size allocated for the storage of its records
                sortMessage[temp.sorterIndex].workerID = temp.sorterIndex;
                sortMessage[temp.sorterIndex].recordsSent = temp.totalRecordCount;
                partialSortedSplices[temp.sorterIndex] = malloc(temp.totalRecordCount * sizeof(struct UsrRecord));
                if (partialSortedSplices[temp.sorterIndex] == NULL) {
                    fprintf(stderr, "Memory Allocation to store Partial Sort %d Results has failed.", temp.sorterIndex);
                    exit(32);
                }
                isInfoReadPending[temp.sorterIndex] = false;//the flag of info read pending must then be turned of
            }
            memcpy(partialSortedSplices[temp.sorterIndex] + temp.recordIndex, &temp.tRecord, sizeof(struct UsrRecord));//use of the record index ensure that the record in the 10th position of a given sorter's ouput remains at the 10th position in the array now used to store it merge

            // printf("%7sSort%3d | %6d | ", temp.sorterIndex % 2 ? "bubble" : "merge", temp.sorterIndex, temp.recordIndex);
            // printUsrRecord(partialSortedSplices[temp.sorterIndex] + temp.recordIndex); 
            recievedRecords += 1;//keeps track

            // so that the user can see some progress
            if (!(recievedRecords % 1000)) {
                printf("!%d ", recievedRecords);
                fflush(stdout);
            }
        } else {
            //so that the user can be assured that the program is running
            printf(".");
            usleep(1000);
        }
    }
    close(sortedResultRecieveFD);

    printf("\n");

    struct Timing timingInformation[numWorkers];//will be used to store the timing information for each sort process
    int pendingTimingCount = numWorkers;//stores the number of sort processes for which timing information is yet to be recieved
    //opening the TIMINGFIFO for read
    int timingInfoFD = open(TIMINGFIFO, O_RDONLY);
    if (timingInfoFD == -1) {
        fprintf(stderr, "Timing FIFO not opening\n");
        exit(34);
    }
    //will continue attempting to read until we have timing information on all the sorters
    while (pendingTimingCount) {
        struct Timing temp;
        int bytesRead = read(timingInfoFD, &temp, sizeof(struct Timing));
        if (bytesRead == sizeof(struct Timing)) {//checking if read was successful
            memcpy(timingInformation + temp.workerID, &temp, sizeof(struct Timing));
            pendingTimingCount--;
        } else {
            // printf("Bytes Read: %d\n", bytesRead);
            usleep(1000);
        }
    }
    close(timingInfoFD);

    //actual process of sorting, the generateSoredArray function generates an array of pointers where the pointers have been sorted according to the compArg and comMode, because we want the smallest at everyturn when merging in ascending order we use compMode 4 = '<', it is compMode 2 = '>' for the same reasoning when we want descending 
    struct UsrRecord** sortedArrayPtrs = NULL;
    if (!(strcmp(orderMode, "a")))
        sortedArrayPtrs = generateSortedArray(sortMessage, partialSortedSplices, totalRecords, numWorkers, compArg, 4);
    else 
        sortedArrayPtrs = generateSortedArray(sortMessage, partialSortedSplices, totalRecords, numWorkers, compArg, 2);

    //opening file for result write
    FILE* sortedOutputFile = fopen(outputFileName, "w");
    if (sortedOutputFile == NULL) {
        fprintf(stderr, "%s could not be opened for the ouput of records.\n", outputFileName);
        exit(31);
    }
    //writing out records to the file 
    for (int i = 0; i < totalRecords; i++) {
        fprintf(sortedOutputFile, "%7u %-12s %-12s %u %8.2lf %4u\n", sortedArrayPtrs[i]->residentID, sortedArrayPtrs[i]->firstName, sortedArrayPtrs[i]->lastName, sortedArrayPtrs[i]->numDependents, sortedArrayPtrs[i]->income, sortedArrayPtrs[i]->postalCode);
    }
    fclose(sortedOutputFile);

    //displaying inforamtion about the timings
    for (int workerIndex = 0; workerIndex < numWorkers; workerIndex++) {
        printf("%6sSort#%3d sorted %6d records, requiring %lf real-time (s) and %lf CPU-time (s).\n", timingInformation[workerIndex].workerID % 2 ? "bubble" : "merge", timingInformation[workerIndex].workerID, sortMessage[timingInformation[workerIndex].workerID].recordsSent, timingInformation[workerIndex].realTime, timingInformation[workerIndex].cpuTime);
    }

    free(sortedArrayPtrs);

    //must free the members of the partialSortedSplices

    for (int workerIndex = 0; workerIndex < numWorkers; workerIndex++) {
        free(partialSortedSplices[workerIndex]);
    }

    //taking the end time
    t2 = (double) times(&tb2);
    cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -(tb1.tms_utime + tb1.tms_stime));
    //computing the time elapsed and then displaying the same
    cpu_time = cpu_time / ticspersec;
    real_time = (t2 - t1) / ticspersec;
    printf("Run time for MERGE was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", real_time, cpu_time);

    kill(rootPID, SIGUSR2);

    exit(0);
}

struct UsrRecord** generateSortedArray(struct Message sortMessages[], struct UsrRecord** sortedSplices, int totalRecordsAmount, int numWorkers, int argNum, int compMode) {
    int sortedCounters[numWorkers];//creating an array that will contain an index for each of the sorted splices, this index will store effectively the current number of records read from each of the sorted partial lists each of which corresponds to one sorter
    //setting things to 0
    for (int i = 0; i < numWorkers; i++)
        sortedCounters[i] = 0;

    //allocated memmory for the array of pointers, the array will contain pointers who have been sorted on the basis of their vlaue
    struct UsrRecord** UsrRecordsSortedPtrs = malloc(sizeof(struct UsrRecord*) * totalRecordsAmount);//this is created the array of uns

    for (int sortedRecordsPtrCount = 0; sortedRecordsPtrCount < totalRecordsAmount; sortedRecordsPtrCount++) {//we iterate continually we have completely created the array of sorted records
        struct UsrRecord* MinRecordPtr = NULL;//this will be used to store the record's ptr where the record has the minimal most value seen thus far (when the order is descending this will actually contain the largest, but it easy to imagine just one case, cause thee comparator is designed to handle the actual detail)
        int minRecordSpliceIndex = -1;//this will be used to store which the sorter whose array had the least value

        //we first have to give a meaningfully value to both, and hence we iterate through until we find a nonempty value so that we can have meaningful comparisons
        for (int i = 0; i < numWorkers; i++) {
            if (sortedCounters[i] < sortMessages[i].recordsSent) {//we have to check that we have not read through all the elements of the given sorted splice
                MinRecordPtr = sortedSplices[i] + sortedCounters[i];
                minRecordSpliceIndex = i;
            }
        }
        //here we perform the actual comparison, selecting the most appropiate record, its pointer, and the sorter from which this was found
        for (int i = 0; i < numWorkers; i++) {
            if (sortedCounters[i] < sortMessages[i].recordsSent && comparator(sortedSplices[i] + sortedCounters[i], MinRecordPtr, argNum, compMode)) {
                MinRecordPtr =  sortedSplices[i] + sortedCounters[i];
                minRecordSpliceIndex = i;
            }
        }

        // printf("#%3d --- MinRecordPtr: %p; minRecordSpliceIndex: %d; lastName: %s\n", sortedRecordsPtrCount, MinRecordPtr, minRecordSpliceIndex, MinRecordPtr->lastName);

        UsrRecordsSortedPtrs[sortedRecordsPtrCount] = MinRecordPtr;//we add this to the sorted array
        sortedCounters[minRecordSpliceIndex] += 1;//we incremement the sortedRecords read from the specific sorter's array so that we don't encounter this value aain
    }
    
    return UsrRecordsSortedPtrs;
}

// Using Select Attempt
// while (pendingSortersResultsToBeRead) {

    //     // printf("PRE: pendingSortersResultsToBeRead: %d\n", pendingSortersResultsToBeRead);

    //     fd_set resultFDS;
    //     int maxResultFD = -1;
            
    //     // printf("RESET RESULT-FDS\n");
    //     FD_ZERO(&resultFDS);

    //     for (int workerIndex = 0; workerIndex < numWorkers; workerIndex++) {//setting up the fd_set structure and finding the maximum fileDescriptor
    //         FD_SET(fifoResultFDS[workerIndex], &resultFDS);
    //         if (fifoResultFDS[workerIndex] > maxResultFD)
    //             maxResultFD = fifoResultFDS[workerIndex];
    //     }

    //     select(maxResultFD + 1, &resultFDS, NULL, NULL, &timeOut);

    //     for (int workerIndex = 0; workerIndex < numWorkers; workerIndex++) {
    //         // for (int i = 0; i < numWorkers; i++) {
    //         //     printf("OPENED at %d\n", fifoResultFDS[i]);
    //         // }
    //         // fprintf(stdout, "HI FROM OUTSIDE IF IN RESULT LOOP %d\n", workerIndex);
    //         // fprintf(stdout, "isResultReadPending[workerIndex] = %d ; !isInfoReadPending[workerIndex] = %d ; FD_ISSET(%d, &resultFDS) = %d\n", isResultReadPending[workerIndex], !isInfoReadPending[workerIndex], fifoResultFDS[workerIndex], FD_ISSET(fifoResultFDS[workerIndex], &resultFDS));
    //         // sleep(1);
    //         if (isResultReadPending[workerIndex] && !isInfoReadPending[workerIndex] && FD_ISSET(fifoResultFDS[workerIndex], &resultFDS)) {
    //             // printf("HI FROM INSIDE IF IN RESULT LOOP %d\n", workerIndex);
    //             int resultReceiveFD = fifoResultFDS[workerIndex];

    //             // struct UsrRecord temp;
    //             struct UsrRecord* entryPtr = partialSortedSplices[workerIndex] + partialSortedCount[workerIndex];
                
    //             ssize_t bytesRead = read(resultReceiveFD, entryPtr, sizeof(struct UsrRecord));

    //             char* outputDumpFile = generateFileName(RESULTHEADER, workerIndex);
    //             FILE* outputFileFD = fopen(outputDumpFile, "a");
    //             if (outputFileFD == NULL) {
    //                 fprintf(stderr, "FILE NOT OPENING\n");
    //                 exit(23);
    //             }

    //             printf("Initial Bytes Read: %lu\n", bytesRead);
    //             // printf("Writing partialSortResult%s to for verification\n", outputDumpFile);


    //             while (bytesRead == sizeof(struct UsrRecord) && partialSortedCount[workerIndex] < sortMessage[workerIndex].recordsSent) { 
    //                 printf("%5d\n", partialSortedCount[workerIndex]);
    //                 fprintf(outputFileFD, "From %2d | %2d | ", sortMessage[workerIndex].workerID, partialSortedCount[workerIndex]);
    //                 // printUsrRecord(partialSortedSplices[workerIndex] + partialSortedCount[workerIndex]);   
    //                 // memcpy(partialSortedSplices[workerIndex] + partialSortedCount[workerIndex], &temp, sizeof(struct UsrRecord));            
    //                 // printf("Sort %d | %d: ", sortMessage[workerIndex].workerID, partialSortedCount[workerIndex]);
    //                 fprintf(outputFileFD, "%7u %-12s %-12s %u %8.2lf %4u\n", entryPtr->residentID, entryPtr->firstName, entryPtr->lastName, entryPtr->numDependents, entryPtr->income, entryPtr->postalCode);
    //                 // printf("#%2d | Bytes Read %lu\n", partialSortedCount[workerIndex], bytesRead);
    //                 // printf("Sort%d: #%2d of %2d @%p | \n", sortMessage[workerIndex].workerID, partialSortedCount[workerIndex], sortMessage[workerIndex].recordsSent, &(partialSortedSplices[workerIndex][partialSortedCount[workerIndex]]));
    //                 // printUsrRecord(entryPtr);
    //                 partialSortedCount[workerIndex] = partialSortedCount[workerIndex] + 1;//increment the count of records read for the sort at the workerIndex
    //                 // entryPtr = &(partialSortedSplices[workerIndex][partialSortedCount[workerIndex]]);
    //                 // bytesRead = read(resultReceiveFD, &entryPtr, sizeof(struct UsrRecord));//reading the record into the particular sorter processes array
    //                 entryPtr = partialSortedSplices[workerIndex] + partialSortedCount[workerIndex];
    //                 bytesRead = read(resultReceiveFD, entryPtr, sizeof(struct UsrRecord));//reading the record into the particular sorter processes array
    //                 // printUsrRecord(&temp);
    //             }

    //             printf("I AM OUT\n");

    //             usleep(100000);

    //             if (partialSortedCount[workerIndex] == sortMessage[workerIndex].recordsSent) {
    //                 pendingSortersResultsToBeRead--;
    //                 // printf("POST: pendingSortersResultsToBeRead:%d\n", pendingSortersResultsToBeRead);
    //                 isResultReadPending[workerIndex] = false;
    //                 close(resultReceiveFD);
    //             }


    //             free(outputDumpFile);
    //             fclose(outputFileFD);

    //             // printf("PRE-CLOSE FIFO\n");
    //             // printf("PRE-FREE STR\n");
    //             // printf("DEATH\n");
    //         }
    //     }

    //     // printf("ULTIMATE DEATH\n");
    // }

    // printf("ULTIMATEST DEATH\n");





    
        // fd_set FDS;
        // int maxFD = -1;
        // struct timeval timeOut = {0, 1000};

        // printf("RESET FDS\n");
        // FD_ZERO(&FDS); // Clear FD set for select
        // for (int i = 0; i < numWorkers; i++) {
        //     FD_SET(fifoFDS[i], &FDS);
        //     if (fifoFDS[i] > maxFD)
        //         maxFD = fifoFDS[i];
        // }

        // select(maxFD + 1, &FDS, NULL, NULL, &timeOut);

        // for (int workerIndex = 0; workerIndex < numWorkers; workerIndex++) {
        //     if (isReadPending[workerIndex] && FD_ISSET(fifoFDS[workerIndex], &FDS)) {
        //         int UsrRecordReceive = fifoFDS[workerIndex];   

        //         struct MessageHeader msg;
        //         struct UsrRecord tempUsrRecord;
        //         struct TimingHeader sortTimeInfo;
                
        //         if (read(UsrRecordReceive, &msg, sizeof(struct MessageHeader)) == sizeof(struct MessageHeader)) {
        //             printf("Successfully Read Header, %d records have been by Sort %d through %s\n", msg.recordsSent, msg.workerID, fifoResultFileNames[workerIndex]);
        //         } else {
        //             fprintf(stderr, "Header could not be read for Sort %i\n", workerIndex);
        //             sleep(1);
        //             continue;
        //             // exit(22);
        //         }

        //         char* outputDumpFile = generateFileName(RESULTHEADER, workerIndex);
                
        //         printf("Writing partialSortReceivedResults to %s for verification\n", outputDumpFile);

        //         FILE* outputFileFD = fopen(outputDumpFile, "w");
        //         if (outputFileFD == NULL) {
        //             fprintf(stderr, "FILE NOT OPENING\n");
        //             exit(23);
        //         }

        //         int recordCounter;
        //         for (recordCounter = 0; recordCounter < msg.recordsSent && (read(UsrRecordReceive, &tempUsrRecord, sizeof(tempUsrRecord)) == sizeof(tempUsrRecord)); recordCounter++) {
        //             fprintf(outputFileFD, "Sort %d | %d: ", msg.workerID, msg.recordsSent);
        //             fprintf(outputFileFD, "%7u %-12s %-12s %u %8.2lf %4u\n", tempUsrRecord.residentID, tempUsrRecord.firstName, tempUsrRecord.lastName, tempUsrRecord.numDependents, tempUsrRecord.income, tempUsrRecord.postalCode);
        //         }

        //         if (read(UsrRecordReceive, &sortTimeInfo, sizeof(struct TimingHeader)) == sizeof(struct TimingHeader)) {
        //             printf("Runtime for %sSort%d was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", msg.workerID % 2 ? "bubble" : "merge", msg.workerID, sortTimeInfo.realTime, sortTimeInfo.cpuTime);
        //         } else {
        //             fprintf(stderr, "Timing Header could not be read for Sort %i\n", workerIndex);
        //             sleep(1);
        //             // exit(24);
        //         }

        //         char smallBuff[1];

        //         if (read(UsrRecordReceive, &smallBuff, sizeof(char)) == 0 && recordCounter == msg.recordsSent) {
        //             printf("READING for %sSort%d COMPLETE\n", msg.workerID % 2 ? "bubble" : "merge", msg.workerID);
        //             isReadPending[workerIndex] = false;
        //             pendingSortersToBeRead--;   
        //         } else {
        //             fprintf(stderr, "GREATER READING OF DATA SHOULD NOT BE POSSIBLE BY THE DETAILS OF THE HEADERS, YET THERE IS STILL DATA TO BE READ FROM %s \n", fifoResultFileNames[workerIndex]);
        //             continue;
        //             // exit(25);
        //         }

        //         fclose(outputFileFD);
        //         free(outputDumpFile);
        //         close(UsrRecordReceive);
        //     }
        // }