CC=gcc 
CFLAGS=-I../include

all: wrapper suspecter

wrapper: wrapper.c
	$(CC) -o wrapper wrapper.c

suspecter: suspecter.c
	$(CC) -o suspecter suspecter.c
	
clean:
	rm -f wrapper suspecter