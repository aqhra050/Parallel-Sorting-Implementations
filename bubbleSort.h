#ifndef BUBBLESORT
#define BUBBLESORT

#include "sortHelper.h"//this is a file containing functions that are necessary to both sorter programs, this helps prevents redundancy

//more commments explaining the function are found in their function definitions as it is more apppropiate therein
struct UsrRecord** bubbleSortHelper(struct UsrRecord* arrayUsrRecords, int lenArray, int argNum, int compMode);//will generate an array of pointers to taxxRecords, which will then be sorteed through invocation of bubbleSort 
void    bubbleSort(struct UsrRecord** arrayUsrRecords, int lenArray, int argNum, int compMode);//will sort the array of tax records using the bubblsort algorithm

#endif