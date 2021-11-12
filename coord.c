#include "coord.h"

int main(int argc, char* argv[]) {

    //the variables needed for timing details
    double t1, t2, cpu_time, real_time, ticspersec;
    struct tms tb1, tb2;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    //taking the start time
    t1 = (double) times(&tb1);

    // printf("In Process Node 'Coord' with PID %u have been invoked by %u\n", getpid(), getppid());

    pid_t rootPID = getppid(); 

    if (argc == 7) {
        //coord should only be invoked from root and hence there is no input validation herein, because it would have been too ineffective to contiually do as much for each subsequent process

        char*   inputFile; 
        char*   sortingOrder;
        char*   outputFile;
        int     numWorkers, attrNum;
        bool    isRandom;

        parseCommandLineArguements(argv, &inputFile, &numWorkers, &isRandom, &attrNum, &sortingOrder, &outputFile);
        
        //multiple sorter proccesses are to be created and they are each to be given their input, the number of such proccess is equivalent to the number of number of workers input

        //must find the length of the file with regards to the number of records
        int numRecords;
        getNumberOfRecords(&numRecords, inputFile);//will store the number of records in the file if records exist into the file numrecords, if no records exist then numRecords will take on the value -1 and the program will terminate

        int* rangeBoundaries; //will be an array containing the range boundaries for the each of the sorters 

        //must do splitting of range here, for each sort process, a starting point and an ending range must be found
        //this splitting must either be uniform or random varying upon the value of isRandom in turn dependent on userInput
        if (isRandom == true) {
            rangeBoundaries = randomRangeSplit(0, numRecords, numWorkers); 
        } else {//random must be false
            rangeBoundaries = uniformRangeSplit(0, numRecords, numWorkers);
        }
        
        printf("\nRange Display\n");
        // Displaying the range boundaries, useful for debugging and verifying output
        for (int i = 0; i < numWorkers; i++) {
            printf("%i [%i, %i) ", i, rangeBoundaries[i], rangeBoundaries[i + 1]);
        }
        printf("\n");        


        //this generates the FIFOS for subsequent processes
        mkfifo(RESULTFIFO, 0777);
        mkfifo(TIMINGFIFO, 0777);

        int     workerIndex = 0;//will be used as a count for the current sort node created
        pid_t   newForkID, mergeForkID;//these will be used in the forking of a sort node, and new merge node respectively
        pid_t   sortNodePIDS[numWorkers];//this will be used to store the process id for each sortNode
        memset(sortNodePIDS, 0, sizeof(sortNodePIDS));
                
        do {//the do while loop will only do a fork when the process doing as much is a parent process and not a dervivate, derivates will take on the value of 0 for the newForkID
            // printf("Sort Fork Attempt %i\n", workerIndex);
            newForkID = fork();
            if (newForkID == -1) {
                fprintf(stderr, "Fork attempting to create coord failed in root.\n");
                exit(7);
            }
            if (newForkID == 0) {    
                //we have two sort programs, one uses the mergeSort algorithm, the other uses the bubbleSort algorithm
                //the sorter that uses the mergeSort algorithm will be invoked for even numbered workers
                //the sorter that uses the bubbleSort algorithm will be invoked for odd numbered workers 
                //each is passed its requisite arguements following suitable conversion to strings
                int execErr;
                
                if (workerIndex % 2 == 0) {
                    execErr = execlp("./mergeSort", "mergeSort", inputFile, integerToString(rangeBoundaries[workerIndex]), integerToString(rangeBoundaries[workerIndex + 1]), integerToString(attrNum), sortingOrder, integerToString(workerIndex), integerToString(rootPID), (char *) NULL);
                } else {
                    execErr = execlp("./bubbleSort", "bubbleSort", inputFile, integerToString(rangeBoundaries[workerIndex]), integerToString(rangeBoundaries[workerIndex + 1]), integerToString(attrNum), sortingOrder, integerToString(workerIndex), integerToString(rootPID), (char *) NULL);
                }

                if (execErr == -1) {
                    fprintf(stderr, "Exec has failed to call sort from coord\n");
                    exit(13);
                }
            } else {
                //the processID of the newly created sort node is stored, and the worker index incremented to reflect the new creation of node
                sortNodePIDS[workerIndex] = newForkID;
                workerIndex++;
            }
        } while (workerIndex < numWorkers && newForkID != 0);

        // //displaying all the pids so the output may be inspected to see if all process are indeed being waited for
        // for (int i = 0; i < numWorkers; i++)
        //     printf("Sort %i has PID %lu\n", i, (long) sortNodePIDS[i]);

        // printf("Merge Fork Attempt\n");

        //This creates the merge nodes, and passes to it its arguements after their conversion to string 
        mergeForkID = fork();
        if (mergeForkID == -1) {
            fprintf(stderr, "Fork attempting to create merge failed in root.\n");
            exit(28);
        } else if (mergeForkID == 0) {
            // printf("Arguements to merge: %d\n", numWorkers);//merge node requires the number of sorters to be known, as well as presumably the sorting mode, sorting attribute, and output file
            int execErr = execlp("./merge", "merge", integerToString(numWorkers), integerToString(attrNum), sortingOrder, outputFile, integerToString(rootPID), integerToString(numRecords), (char *) NULL);
            if (execErr == -1) {
                fprintf(stderr, "Exec has failed to call merge from coord\n");
                exit(27);
            }
        }

        //all the numWorkers # of sort nodes must be waited for, as well as the merge node
        waitForAllChildNodes(numWorkers, mergeForkID);

        //the FIFOS must be released
        unlink(RESULTFIFO);
        unlink(TIMINGFIFO);

        //this is the end time point
        t2 = (double) times(&tb2);
        cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) - (tb1.tms_utime + tb1.tms_stime));
        //these are two distinct calculations
        cpu_time = cpu_time / ticspersec;
        real_time = (t2 - t1) / ticspersec;

        printf("Run time for COORD was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", real_time, cpu_time);

        //release all dynmically allocated memory
        free(inputFile);
        free(outputFile);
        free(sortingOrder);

        exit(0);
    } else {
        printf("Invalid Execution of the Process Node 'Coord'\n");
        exit(4);
    }
}

void parseCommandLineArguements(char* argv[], char** inputFilePtr, int* numWorkersPtr, bool* isRandomPtr, int* attrNumPtr, char** sortingOrderStringPtr, char** outputFilePtr) {
    //note no input validation is done herein, since that is the responsible of the main is the root node, the coord will be only be invoked after the verfiyication the validity of the arguements, accordingly herein arguements are only taken from argv and merely stored at the correct positions
    *inputFilePtr   =   malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(*inputFilePtr, argv[1]);
    *numWorkersPtr  =   atoi(argv[2]);
    *isRandomPtr    =   atoi(argv[3]);
    *attrNumPtr     =   atoi(argv[4]);

    *sortingOrderStringPtr = malloc((strlen(argv[5]) + 1) * sizeof(char));
    strcpy(*sortingOrderStringPtr, argv[5]);

    *outputFilePtr = malloc((strlen(argv[6]) + 1) * sizeof(char));
    strcpy(*outputFilePtr, argv[6]);

    // //just a diplay of arguements 
    // printf("Input File:           %s\n", *inputFilePtr);
    // printf("Number of Workers:    %i\n", *numWorkersPtr);
    // printf("isRandom:             %s\n", *isRandomPtr ? "True" : "False");
    // printf("Attribute Number:     %i\n", *attrNumPtr);
    // printf("Sorting Order:        %s\n", *sortingOrderStringPtr);
    // printf("Output File:          %s\n", *outputFilePtr);
}

void getNumberOfRecords(int* numRecordsPtr, char inputFile[]) {
    //gets the number of recors in the file
    if ((*numRecordsPtr = numLinesInFiles(inputFile)) == -1) {
        fprintf(stderr, "The number of records could not be found in coord using numLinesInFiles\n");
        exit(8);
    } else {
        printf("The number of records is %d\n", *numRecordsPtr);
    }
}

void testingCompartor() {
    //was used for testing comparator
    struct UsrRecord me = {100, "Abdullah", "Chaudhry", 0, 2000, 54810};
    struct UsrRecord you = {200, "Farah", "Usman", 1, 1500, 2200};

    printUsrRecord(&me);
    printUsrRecord(&you);

    int comArg[] = {0, 3, 4, 5};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 6; j++) {
            comparator(&me, &you, comArg[i], j);
            sleep(1);
        }
}

void waitForAllChildNodes(int numNodes, pid_t mergeID) {
    //note that it was first considered using  waitpid in a for loop and waiting for each process in the body of the for loop, however this would have the effect for serializing execution, coord process will block and not accept the finishing of other children
    // printf("We presently wait for %i sort processes and the merge processes to finish.\n", numNodes);
    int childNodesExecuting = numNodes + 1;
    int status;
    while (childNodesExecuting) {//while there is a still child node of the coord, we must continue to wait
        pid_t waitedProccessID = wait(&status);
        if (waitedProccessID == -1) {
            fprintf(stderr, "Error when waiting for node, no nodes remaining.\n");
            exit(14);
        } else {
            //an appropiate message is displayed depending on whether the child was the merge node or the sort node
            if (waitedProccessID == mergeID) {
                printf("Coord has waited for merge which has ended with status %d. ", status >> 8);
            } else {
                printf("Coord (parent process) has waited for Sort (child process) %d which has returned with status %d. ", waitedProccessID, status >> 8); //shifting must be done only byte 1 of bytes 0 1 2 3 of the status int is set
            }
            childNodesExecuting--;
            printf("%i Child Processes of Coord remain.\n", childNodesExecuting);
        }
    }
}

int* randomRangeSplit(int rangeStart, int rangeEnd, int splitAmount) {
    //the split amount identifies the number of ranges to be created
    //the first split will be at element 0 of the returned array, and so forth
    //each element of the array is in effect a boundary so that the first sorter should have starting boundary splitPositions[0] and ending boundary splitPositions[1], second sorter should have starting boundary splitPositions[1] and ending boundary splitPositions[2], ..., and the last sorter should should have starting boundary splitPositions[splitamount - 1] and ending boundary splitPositions[splitamount]
    //the function has been designed so that the range of records though random, is somewhat uniform, which ensures that the divide-and-conquer paradigm of the assignment is met. Indeed, it is ensured that the number of records allocated to any range is never less than 1.

    int* splitPositions = malloc((splitAmount + 1) * sizeof(int));
    int splitDifferential = (float) (rangeEnd - rangeStart) / (float) splitAmount * 1.8;//this differential was calculated after much trial and error so that though the ranges of each sorter are random, they are balanced

    // printf("In randomRangeSplit of %i with differential %i:\n", splitAmount, splitDifferential);  

    if (splitPositions == NULL) {
        fprintf(stderr, "Allocating an array in random range split has failed.\n");
        exit(15);
    }
    int byteLen = sizeof(splitPositions);
    memset(splitPositions, 0, byteLen);
    srand(time(0));
    
    splitPositions[0] = rangeStart;
    for (int i = 1; i < splitAmount; i++) {
        //the modulus of rand is taken by the lesser of splitDifferential and rangeEnd - rangeStart, this way the initial allocations are not too large and the final allocations do not overshoot bounds
        int modulator = (splitDifferential < rangeEnd - rangeStart ? splitDifferential: rangeEnd - rangeStart);
        int diff = rand() %  modulator;//ensuring that we don't have a zero length section
        diff = (rangeEnd - (diff + rangeStart) < splitAmount - i ? 1: diff);
        splitPositions[i] = (diff ? diff : 1) + rangeStart; 
        rangeStart = splitPositions[i];
    }
    splitPositions[splitAmount] = rangeEnd;

    return splitPositions;
}

int* uniformRangeSplit(int rangeStart, int rangeEnd, int splitAmount) {
    //the split amount identifies the number of ranges to be created
    //the first split will be at element 0 of the returned array, and so forth
    //each element of the array is in effect a boundary so that the first sorter should have starting boundary splitPositions[0] and ending boundary splitPositions[1], second sorter should have starting boundary splitPositions[1] and ending boundary splitPositions[2], ..., and the last sorter should should have starting boundary splitPositions[splitamount - 1] and ending boundary splitPositions[splitamount]
    //this splitting will be uniform for all sorters, except the last one which must invariably pick up the slack due to non-divisible numbers


    int* splitPositions = malloc((splitAmount + 1) * sizeof(int));
    int splitDifferential = (rangeEnd - rangeStart + 1) / splitAmount;

    // printf("In uniformRangeSplit of %i with differential %i:\n", splitAmount, splitDifferential);  

    if (splitPositions == NULL) {
        fprintf(stderr, "Allocating an array in random range split has failed.\n");
        exit(15);
    }
    int byteLen = sizeof(splitPositions);
    memset(splitPositions, 0, byteLen);
    
    splitPositions[0] = rangeStart;
    for (int i = 1; i < splitAmount; i++) {
        //the modulus of rand is taken by the lesser of splitDifferential and rangeEnd - rangeStart, this way the initial allocations are not too large and the final allocations do not overshoot bounds
        splitPositions[i] = splitDifferential * i; 
    }
    // printf("splitAmount = %i\n", splitAmount);
    splitPositions[splitAmount] = rangeEnd;
    // printf("rangeEnd = %i\n", rangeEnd);

    return splitPositions;
}

void testRandomRangeSplit() {
    int splits = 10;
    int* rangeSplit = randomRangeSplit(0, 1000, splits);
    printf("SPLITTING RANGE\n");
    for (int i = 0; i <= splits; i++)
        printf("%d ", rangeSplit[i]);
    printf("\n");
    free(rangeSplit);
}
