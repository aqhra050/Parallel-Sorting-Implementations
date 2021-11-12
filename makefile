all: myhie coord mergeSort bubbleSort merge

myhie: main.o helper.o
	gcc -Wall -g -o myhie main.o helper.o

coord: coord.o helper.o
	gcc -Wall -g -o coord coord.o helper.o

mergeSort: mergeSort.o helper.o sortHelper.o
	gcc -Wall -g mergeSort.o helper.o sortHelper.o -o mergeSort

bubbleSort: bubbleSort.o helper.o sortHelper.o
	gcc -Wall -g bubbleSort.o helper.o sortHelper.o -o bubbleSort

merge: merge.o helper.o
	gcc -Wall -g merge.o helper.o -o merge

main.o: main.c helper.o
	gcc -Wall -g -c main.c

coord.o: coord.c helper.o mergeSort.o bubbleSort.o merge.o
	gcc -Wall -g -c coord.c

mergeSort.o: helper.o mergeSort.c sortHelper.o
	gcc -Wall -g -c mergeSort.c

bubbleSort.o: helper.o bubbleSort.c sortHelper.o
	gcc -Wall -g -c bubbleSort.c

merge.o: merge.c helper.o
	gcc -Wall -g -c merge.c

helper.o: helper.c 
	gcc -Wall -g -c helper.c

sortHelper.o: sortHelper.c
	gcc -Wall -g -c sortHelper.c

clean:
	rm -f *.o myhie coord mergeSort bubbleSort merge

