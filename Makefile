CC=gcc
CFLAGS=-pthread -I. -Wall
binaries=stim
all: $(binaries)
clean: $(RM) -f $(binaries) *.o
