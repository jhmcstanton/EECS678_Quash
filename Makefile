STUDENTID = XXXXXXX
PROGNAME = quash

IDIR = includes

CC = gcc --std=c99
CFLAGS = -Wall -g -Og -I$(IDIR)

# Add files to their respective line to get this makefile to build them
CFILES = quash.c $(IDIR)/hashtable.c
HFILES = quash.h debug.h 

_DEPS = hashtable.h
DEPS  = $(patsubst %, $(IDIR)/%, $(_DEPS))

# Add libraries that need linked as needed (e.g. -lm)
LIBS =

DOXYGENCONF = quash.doxygen

OBJFILES = $(patsubst %.c,%.o,$(CFILES))
EXECNAME = $(patsubst %,./%,$(PROGNAME))

all: doc $(PROGNAME)

$(PROGNAME): $(OBJFILES) 
	$(CC) $(CFLAGS) $^ -o $(PROGNAME) $(LIBS) 

%.o: %.c $(HFILES) $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS)

test: $(PROGNAME)
	$(EXECNAME)

doc: $(CFILES) $(HFILES) $(DOXYGENCONF) README.md
	doxygen $(DOXYGENCONF)

tar: clean
	tar cfzv $(STUDENTID)-quash.tar.gz *

clean:
	-rm -rf $(PROGNAME) *.o *~ doc

.PHONY: clean
