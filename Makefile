CC=gcc
CFLAGS=-pthread -I. -Wall -Wno-int-conversion -D_GNU_SOURCE -fcommon
DEBUGFLAGS=-g

# If DEBUG is defined, append DEBUGFLAGS to CFLAGS
ifeq ($(DEBUG), 1)
  CFLAGS += $(DEBUGFLAGS)
endif

#binaries=queueprodcons cpa pthread_mult
binaries=pcMatrix

all: $(binaries)

pcMatrix: counter.c prodcons.c matrix.c pcmatrix.c 
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) -f $(binaries) *.o
