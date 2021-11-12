#include "main.h"

int sortSignalCounts = 0;
void sortSigHandler(int sig) {//will be handling the signals from the sort processes
    signal(SIGUSR1, sortSigHandler);
    sortSignalCounts++;
}
int mergeSignalCounts = 0;
void mergeSigHandler(int sig) {//will be handling the signals from the merge process
    signal(SIGUSR2, mergeSigHandler);
    mergeSignalCounts++;
}

int main(int argc, char* argv[]) {
    //these are the variable that will be used to store user input
    char*   inputFile       = NULL;
    char*   outputFile      = NULL;
    int     numWorkers      = -1;  
    int     attrNum         = -1;
    bool    isRandom        = false;
    char    sortingOrder    = '\0';

    //adapted from the code given for timing, and this takes the start time
    double t1, t2, cpu_time, real_time;
    struct tms tb1, tb2;
    double ticspersec;
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    t1 = (double) times(&tb1);//taking the initial time

    // printf("Num Arguements: %i\n", argc);

    bool succesfulHandling = commandLineArguementHandling(argc, argv, &inputFile, &numWorkers, &isRandom, &attrNum, &sortingOrder, &outputFile);//handles the user input at the command line, storing it in the respective variables if valid, and generating appropiate errors otherwise, a true will be returned in the case of successful
    // printf("Succesful Handling %d\n", succesfulHandling);
    if (succesfulHandling) {//if data has been successfully handled in that the useful input was valid, we can proceed further on
        //these are being displayed below for verification
        printf("Input File:           %s\n", inputFile);
        printf("Number of Workers:    %i\n", numWorkers);
        printf("isRandom:             %s\n", isRandom ? "True" : "False");
        printf("Attribute Number:     %i\n", attrNum);
        printf("Sorting Order:        %c\n", sortingOrder);
        printf("Output File:          %s\n", outputFile);

        //build signal assossciations
        signal(SIGUSR1, sortSigHandler);
        signal(SIGUSR2, mergeSigHandler);

        // printf("In Process Node 'Root'\n");

        pid_t forkID = fork();
        // printf("Fork in root with ID: %i\n", forkID);
        // printf("Fork ID: %i\n", forkID);
        if (forkID == -1) {
            fprintf(stderr, "Fork (attempting to create coord) failed in root.\n");
            exit(5);
        }
        if (forkID == 0) {//child process should be the coord node 
            //must convert to string to pass as arguements, to the coord node 
            char*   sNumWorkers   = integerToString(numWorkers);
            char*   sIsRandom     = integerToString(isRandom);
            char*   sAttrNum      = integerToString(attrNum);
            char    sSorting[2]    = {sortingOrder, '\0'};

            int execErr = execlp("./coord", "coord", inputFile, sNumWorkers, sIsRandom, sAttrNum, sSorting, outputFile, (char *) NULL);
            //a guard to check whether or not the call to exec with coord has been successful, very useful in debugging
            if(execErr == - 1) {
                //dynamically allocated memory must be freed
                fprintf(stderr, "Exec has failed to call coord from root\n");
                free(sNumWorkers);
                free(sIsRandom);
                free(sAttrNum);
                exit(10);
            }
        } else {//root process should wait for coord node process
            int status;
            pid_t waitedProccessID = waitpid(forkID, &status, 0);
            if (waitedProccessID == -1) {//if waiting for some reason has failed, this must be indicated
                fprintf(stderr, "Root has not waited for coord process and an error has occured.\n");
                exit(6);
            }
            printf("Root (parent process) has waited for Coord (child process) %d which has returned with status %d.\n", waitedProccessID, status >> 8); //shifting must be done only byte 1 of bytes 0 1 2 3 of the status int is set

            printf("%2d SIGUSR1 signals have been recieved from various Sort processes.\n", sortSignalCounts);
            printf("%2d SIGUSR2 signals have been recieved from Merge process.\n", mergeSignalCounts);

            //taking the end time
            t2 = (double) times(&tb2);
            cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -(tb1.tms_utime + tb1.tms_stime));
            //computing the time elapsed and then displaying the same
            cpu_time = cpu_time / ticspersec;
            real_time = (t2 - t1) / ticspersec;
            
            //displaying the timing information
            printf("Run time for ROOT was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n", real_time, cpu_time);
            printf("Turn Around Time (REAL time) %lf sec.\n", real_time);


            free(inputFile);
            free(outputFile);

            exit(0);
        }
    } else {
        printf("Invalid Execution of the Process Node 'Root'\n");
        exit(4);
    }
}

// the argueents -r and -s are optional, by default r will take on the value of false and OutputFile will take on the value output.txt, and these values will be taken in main
// below are all the acceptable invocations of the program, and note that the flags can be arranged in any arbitrary order, it is only necessary that the value of any given flag if it is neccessary and appropiate, follow on the from the flag
// myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile 
// myhie -i InputFile -k NumOfWorkers -a AttributeNumber -o Order -s OutputFile
// myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order
// myhie -i InputFile -k NumOfWorkers -a AttributeNumber -o Order
// this following function will parse the user input, and it is necessary that an inputFile, numOfWorkers, an attributeNumber, and sort order (ascending or descending denoted by a or d)
bool commandLineArguementHandling(int argc, char* argv[],  char** inputFilePtr, int* numWorkersPtr, bool* isRandomPtr, int* attrNumPtr, char* orderPtr, char** outputFilePtr) {
    
    //flags containing information on which inputs have been parsed
    bool setInputFile = false; bool setNumWorks = false; bool setRandom = false; bool setAttrNum = false; bool setOrder = false; bool setOutputFile = false;

    //program can only accept the command line invocations similar to those above and these have particular lengths
    if (argc == 9 || argc == 10 || argc == 11 || argc == 12) {

        for (int i = 1; i <  argc; i++) {
            //will iterate through the entire command line invokation of arguements ignoring the first element which is necessarily the program name and the last which necessarily cannot be a flag
            //will active the flags in token and try to validate input at every turn
            char* token = malloc((strlen(argv[i]) + 1) * sizeof(char));
            if (token == NULL) {
                fprintf(stderr, "MEMORY ALLOCATION FAILED FOR TOKEN IN MAIN\n");
                exit(1);
            }
            strcpy(token, argv[i]);
            
            //checking for each flag
            if (!(strcmp(token, "-i"))) {//checking flag for inputfile, storing the input file (which should be at the subsequent command line arguement), and validating whether or not it opens (if it does then the inputFileName is valid)
                if (i == argc - 1) {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE '-i' MUST BE FOLLOWED BY AN INPUT FILE NAME, IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }
                FILE* taxInputFile = fopen(argv[i + 1], "r");
                //checking whether or not the file could be successfuly openeded
                if (taxInputFile == NULL) {
                    fprintf(stderr, "FILE %s could not be opened.\n", argv[i + 1]);
                    return false;
                }
                fclose(taxInputFile);//our use of the file has ended, only an existence check was presently necessary

                *inputFilePtr = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));//allocating memory at the inputFilePtr, and storing the value therein
                if (*inputFilePtr == NULL) {
                    fprintf(stderr, "MEMORY ALLOCATION FAILED FOR inputFile IN MAIN\n");
                    exit(1);
                }
                strcpy(*inputFilePtr, argv[i + 1]);

                setInputFile = true;

            } else if (!(strcmp(token, "-k"))) {//handle the number of workers, by attempting to convert the command line arguement that follows, if this fails then an error message is displayed
                if (i == argc - 1) {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE '-k' MUST BE FOLLOWED BY NUMBER OF WORKERS, IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }   
                char* end;
                int temp = (int) strtol(argv[i + 1], &end, 10);
                if (end == argv[i + 1]) {
                    fprintf(stderr, "Number of workers (%s) could either not be converted to integer.\n", argv[i + 1]);
                    return false;
                } else if (temp <= 0) {
                    fprintf(stderr, "Number of workers must be greater than zero, (%s) has been inputted.\n", argv[i + 1]);
                    return false;
                }
                *numWorkersPtr = temp;
                setNumWorks = true;
            } else if (!(strcmp(token, "-r"))) {//setting the random flag to be true, if appropiate
                *isRandomPtr = true;
                setRandom = true;
            } else if (!(strcmp(token, "-a"))) {//taking the attribute number and storing it, will check if the attribute number is convertable to int and in (0, 3, 4, 5), if not error
                if (i == argc - 1) {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE '-k' MUST BE FOLLOWED BY AN ATTRIBUTE NUMBER, IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }
                char* end;
                int temp = (int) strtol(argv[i + 1], &end, 10);
                if (end == argv[i + 1]) {
                    fprintf(stderr, "Attribute Number (%s) could either not be converted to integer.\n", argv[i + 1]);
                    return false;
                }
                if (temp == 0 || temp == 3 || temp == 4 || temp == 5) {
                    *attrNumPtr = temp;
                    setAttrNum = true;
                } else {
                    fprintf(stderr, "Attribute Number '%s' not of 0, 3, 4, or 5.\n", argv[i + 1]);
                    return false;
                }
            } else if (!(strcmp(token, "-o"))) {//this will store the sort order (either ascending or descending as denoted by 'a' or 'd')
                if (i == argc - 1) {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE '-o' MUST BE FOLLOWED BY A SORT ORDER, IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }
                if (!strcmp(argv[i + 1], "a")  || !strcmp(argv[i + 1], "d")) {
                    *orderPtr = argv[i + 1][0];
                    setOrder = true;
                } else {
                    fprintf(stderr, "Order '%s' not of 'a'(scending) or 'd'(escending).\n", argv[i + 1]);
                    return false;
                }
            } else if (!(strcmp(token, "-s"))) {//will store the output filename (no validation for this is needed)
                if (i == argc - 1) {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE '-s' MUST BE FOLLOWED BY A OUTPUT FILENAME, IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }
                if (argv[i + 1][0] == '-') {
                    fprintf(stderr, "INCORRECT INVOCATION SINCE OUTPUT FILENAME CANNOT BEGIN WITH '-', IT CANNOT BE THE FINAL COMMANDLINE ARG.\n");
                    return false;
                }
                *outputFilePtr = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
                strcpy(*outputFilePtr, argv[i + 1]);
                setOutputFile = true;
            }

            free(token);
        }
        
        // myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order -s OutputFile
        // myhie -i InputFile -k NumOfWorkers -a AttributeNumber -o Order -s OutputFile
        // myhie -i InputFile -k NumOfWorkers -r -a AttributeNumber -o Order
        // myhie -i InputFile -k NumOfWorkers -a AttributeNumber -o Order

        printf("INPUT FLAGS: setInputFile %d setNumWorks %d setRandom %d setAttrNum %d setOrder %d setOutputFile %d\n", setInputFile, setNumWorks, setRandom, setAttrNum, setOrder, setOutputFile);

        switch (argc)//handling invalid number of arguements
        {
            case 9:
                if (!(setInputFile && setNumWorks && setAttrNum && setOrder)) {
                    fprintf(stderr, "Invalid Invokation of program with 8 arguements, either the '-i', '-k', '-a', '-o' flags or some combination of them is missing.\n");
                    return false;
                }
                break;
            case 10:
                if (!(setInputFile && setNumWorks && setRandom && setAttrNum && setOrder)) {
                    fprintf(stderr, "Invalid Invokation of program with 10 arguements, either the '-i', '-k', 'r', '-a', '-o' flags or some combination of them is missing.\n");
                    return false;
                }
                break;
            case 11:
                if (!(setInputFile && setNumWorks && setAttrNum && setOrder && setOutputFile)) {
                    fprintf(stderr, "Invalid Invokation of program with 11 arguements, either the '-i', '-k', '-a', '-o', '-s' flags or some combination of them is missing.\n");
                    return false;
                }
                break;
            case 12:
                if (!(setInputFile && setNumWorks && setRandom && setAttrNum && setOrder && setOutputFile)) {
                    fprintf(stderr, "Invalid Invokation of program with 12 arguements, either the '-i', '-k', '-r', '-a', '-o', '-s' flags or some combination of them is missing.\n");
                    return false;
                }
                break;
            default:
                break;
        }

        if (setOutputFile == false) {//a default output file is necessary and this will take on the value output.txt
            char defaultOutputFile[] = "output.txt";
            printf("sizeof(defaultOutputFile): %li\n", sizeof(defaultOutputFile));
            *outputFilePtr = malloc(sizeof(defaultOutputFile));
            strcpy(*outputFilePtr, defaultOutputFile);
        }

        return true;

    } else {
        fprintf(stderr, "Program has been invoked with an invalid number of arguments; must be 9, 11, or 12.\n");
        return false;
    }
}