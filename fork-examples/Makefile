# Save this as 'Makefile' in the directory with your source
# Type 'make' to compile 'mysignal.c'
# Type 'make clean' to remove the executable and any *.o files
# Type 'info make' to get more information about how make works

EXECUTABLES = mysignal forkexample forkexample2 forkexample3

all: ${EXECUTABLES}

mysignal: mysignal.c
	gcc -o mysignal mysignal.c

forkexample: forkexample.c
	gcc -o forkexample forkexample.c

forkexample2: forkexample2.c
	gcc -o forkexample2 forkexample2.c

forkexample3: forkexample3.c
	gcc -o forkexample3 forkexample3.c


clean:
	rm -f *.o ${EXECUTABLES}
