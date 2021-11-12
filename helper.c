#include "helper.h"

char* integerToString(int num) {
    // https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c/23840699
    // will convert an integer to string, very useful when passing arguements in exec calls
    int len = snprintf(NULL, 0, "%d", num);
    char* str = malloc((len + 1 ) * sizeof(char));
    snprintf(str, len + 1, "%d", num);

    return str;
}

int numLinesInFiles(char fileName[]) {
    //this gets the number of lines in the filename
    //the man page for getLine was used for the following snippet of code and then adapted to calculate the number of lines in the file

    int numLines = 0;//will be used to store the number of lines

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    FILE* inputFile = fopen(fileName, "r");
    if (inputFile == NULL) {
        fprintf(stderr, "%s could not be opened in the numLinesInFiles() in Coord\n", fileName);
        return -1;
    }

    while ((nread = getline(&line, &len, inputFile)) != -1) {
        numLines++;
        // printf("Retrieved line of length %zu:\n", nread);
        // fwrite(line, nread, 1, stdout);
    }

    free(line);
    fclose(inputFile);

    return numLines;
}

void printUsrRecord(struct UsrRecord* UsrRecordPtr) {
    printf("%7u %-12s %-12s %u %8.2lf %4u\n", UsrRecordPtr->residentID, UsrRecordPtr->firstName, UsrRecordPtr->lastName, UsrRecordPtr->numDependents, UsrRecordPtr->income, UsrRecordPtr->postalCode);
}


bool comparator(struct UsrRecord* lTaxRecrodPtr, struct UsrRecord* rUsrRecordPtr, int comparisonAttribute, int comparisonMode) {
    //this is a comparsion function that enables different types of comparisons across all the comparsion attributes 
    //comparisonAttribute may only take the values of 0, 3, 4, 5 which respectively correspond to comparsion fields of residentID, numDependents, income, postalCode
    //comparisonMode may only take the values of 0 is =, 1 is !=, 2 is >, 3 is >=, 4 is <, 5 is <=
    double lCompVal;//the value of the thing to be compared in the left record of the expression
    double rCompVal;//the vlaue of the thing to be compared in the right record of the expression
    bool comparsionResult;//will contain the return value for the comparison

    switch (comparisonAttribute) {
        case 0:
            lCompVal = lTaxRecrodPtr->residentID;
            rCompVal = rUsrRecordPtr->residentID;
            // printf("RIN           TAKEN - (%f ", lCompVal);
            break;
        case 3:
            lCompVal = lTaxRecrodPtr->numDependents;
            rCompVal = rUsrRecordPtr->numDependents;
            // printf("NUMDEPENDENTS TAKEN - (%f ", lCompVal);
            break;
        case 4:
            lCompVal = lTaxRecrodPtr->income;
            rCompVal = rUsrRecordPtr->income;
            // printf("INCOME        TAKEN - (%f ", lCompVal);
            break;
        case 5:
            lCompVal = lTaxRecrodPtr->postalCode;
            rCompVal = rUsrRecordPtr->postalCode;
            // printf("POSTALCODE    TAKEN - (%f ", lCompVal);
            break;
        default:
            fprintf(stderr, "Invalid comparisonAttribute %i in comparator.\n", comparisonAttribute);
            exit(11);
            break;
    }
    switch (comparisonMode) {
        case 0:
            comparsionResult = (lCompVal == rCompVal);
            // printf("=");
            break;
        case 1:
            comparsionResult = (lCompVal != rCompVal);
            // printf("!=");
            break;
        case 2:
            comparsionResult = (lCompVal > rCompVal);
            // printf(">");
            break;
        case 3:
            comparsionResult = (lCompVal >= rCompVal);
            // printf(">=");
            break;
        case 4:
            comparsionResult = (lCompVal < rCompVal);
            // printf("<");
            break;
        case 5:
            comparsionResult = (lCompVal <= rCompVal);
            // printf("<=");
            break;
        default:
            fprintf(stderr, "Invalid comparisonMode in comparator.\n");
            exit(12);
            break;
    }

    // printf(" %f) is %i\n", rCompVal, comparsionResult);

    return comparsionResult;    
}

char* generateFileName(char header[], int num) {
    //can be used to generate file naems
    char* sNum = integerToString(num);
    char* newFName = malloc((strlen(header) + strlen(sNum) + 1) * sizeof(char));
    strcpy(newFName, header);
    strcat(newFName, sNum);
    
    free(sNum);

    return newFName;
}

