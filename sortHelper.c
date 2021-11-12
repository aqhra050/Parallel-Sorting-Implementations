#include "sortHelper.h"

struct UsrRecord* arrayMake(char fileName[], int startRecordIndex, int endRecordIndex, int workerNumber) {
    //will read the UsrRecords from the file into the array, this array will have length equal to endRecordIndex - startRecordIndex
    //the array reads those records into an array whose number in the file begins from startRecordIndex until the endRecordIndex (exclusive)
    
    int fileCounter = 0, arrayCounter = 0;//fileCounter will store an index of the current record relative to all of the records in the file, arrayCounter will store an index for the current record relative to the start of the array
    int rangeLen = endRecordIndex - startRecordIndex;

    // printf("startRecordIndex: %i, endRecordIndex: %i, rangeLen: %i\n", startRecordIndex, endRecordIndex, rangeLen);

    struct UsrRecord* UsrRecordArr = (struct UsrRecord*) malloc(rangeLen * sizeof(struct UsrRecord));
    if (UsrRecordArr == NULL) {
        fprintf(stderr, "Allocation of arraymake in Sort process %i has failed.\n", workerNumber);
        exit(16);
    }
    FILE* inputFile = fopen(fileName, "r");
    if (inputFile == NULL) {
        fprintf(stderr, "%s could not be opened in the arrayMake() in Sort %i\n", fileName, workerNumber);
        exit(17);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    //the counter is run and lines skipped until we arrive at the appropiate record in file
    // printf("SKIPPED:\n");
    while ((fileCounter != startRecordIndex) && ((nread = getline(&line, &len, inputFile)) != -1)) {
        fileCounter++;

    }

    //by this point in time we should be at the appropiate line in the file, and thereafter we must continue to read until we get to the end record index, the array counter is advanced as each record is added into the file, while the fileCounter also progresses
    while ((fileCounter != endRecordIndex) && ((nread = getline(&line, &len, inputFile)) != -1)) {
        //the input file has a varaible number of space, easiest to first clean the input and then parse its data contents
        char cleanLine[nread + 1];
        bool whiteSpaceFound = false;
        
        //will iterate through the string reducing all space to only one
        for (ssize_t i = 0, j = 0; i <= nread; i++) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n') {
                cleanLine[j] = line[i];
                j++;
                whiteSpaceFound = false;
            } else if (!whiteSpaceFound && line[i] == ' ') {//first time encountering whitespace after a word
                cleanLine[j] = line[i];
                j++;
                whiteSpaceFound = true;
            }
        }
        cleanLine[nread] = '\0';
        //it is trivial to now use scanf when all the multipe spaces have been reduced to one
        sscanf(cleanLine, "%u %s %s %u %lf %u", &UsrRecordArr[arrayCounter].residentID, UsrRecordArr[arrayCounter].firstName, UsrRecordArr[arrayCounter].lastName, &UsrRecordArr[arrayCounter].numDependents, &UsrRecordArr[arrayCounter].income, &UsrRecordArr[arrayCounter].postalCode);

        fileCounter++;
        arrayCounter++;        
    }
    
    // printf("Displaying the records of the array made for verification purposes: \n");
    // for (int i = 0; i < rangeLen; i++) {
    //     printUsrRecord(&UsrRecordArr[i]);
    // }

    fclose(inputFile);

    return UsrRecordArr;
}

void parseSortCommandLineArguement(int argc, char* argv[], char** inputFilePtr, int* startingRangePtr, int* endingRangePtr, int* attrNumPtr, char* sortingOrderPtr, int* workerNumberPtr, char** fifoInfoFilePtr, pid_t* rootPIDPtr) {//parses the command line arguements for programs of the sort variety and then stores the parsed values at the variables pointed, useful for both my sorting functions, will also validate my arguements
    if (argc != 8) {
        printf("ARGC: %d\n", argc);
        fprintf(stderr, "Invalid invocation of Sort process\n");
        exit(21);
    }
    //the sort should be passed, the name of the inputFile 1, the starting range of records 2, the ending range of records 3, the attribute number 4, the sorting order 5, a workerNumber 6, and a fifo file
    
    // printf("This is Sort Node: %s with PID %lu that was brought into existence by %lu\n", argv[1], (long) getpid() ,(long) getppid());

    *inputFilePtr = malloc((strlen(argv[1]) + 1) * sizeof(char));
    strcpy(*inputFilePtr, argv[1]);

    //maybe change atoi to stol
    *startingRangePtr = atoi(argv[2]);
    *endingRangePtr = atoi(argv[3]);
    *attrNumPtr = atoi(argv[4]);
    *sortingOrderPtr = argv[5][0];
    *workerNumberPtr = atoi(argv[6]);

    // *fifoInfoFilePtr = generateFileName(FIFOINFOHEADER, *workerNumberPtr);
    
    *rootPIDPtr = atoi(argv[7]);

    // printf("rootPID: %d\n", *rootPIDPtr);


    // printf("Input File:           %s\n", *inputFilePtr);
    // printf("FIFO Info File:     %s\n", *fifoInfoFilePtr);
    // printf("Attribute Number:     %i\n", *attrNumPtr);
    // printf("Sorting Order:        %c\n", *sortingOrderPtr);

    if (*startingRangePtr < 0) {
        fprintf(stderr, "Invalid argument of startingRange %i in Sort process %i.\n", *startingRangePtr, *workerNumberPtr);
        exit(17);
    }
    if (*endingRangePtr <= *startingRangePtr) {
        fprintf(stderr, "Invalid argument of startRecordIndex %i in Sort process %i.\n", *endingRangePtr, *workerNumberPtr);
        exit(18);
    }
    if (!(*attrNumPtr == 0 || *attrNumPtr ==  3 || *attrNumPtr ==  4 || *attrNumPtr ==  5)) {
        fprintf(stderr, "Invalid argument of attrNum %i in Sort process %i\n", *attrNumPtr, *workerNumberPtr);
        exit(19);
    }
    if (!(*sortingOrderPtr == 'a' || *sortingOrderPtr == 'd')) {
        fprintf(stderr, "Invalid argument of sortingOrder %c in Sort process %i\n", *sortingOrderPtr, *workerNumberPtr);
        exit(20);
    }
}

void attributeNameDisplay(int attrNum) {
    //was a somewhat useful function in debugging, made things more clear when printing
    switch (attrNum)
    {
        case 0:
            printf("RIN\n");
            break;
        case 3:
            printf("NUMDEPENDENTS\n");
            break;
        case 4:
            printf("INCOME\n");
            break;
        case 5:
            printf("POSTALCODE\n");
            break;
        default:
            printf("CANNOT DISPLAY ATTR NAME\n");
            break;
    }
}

