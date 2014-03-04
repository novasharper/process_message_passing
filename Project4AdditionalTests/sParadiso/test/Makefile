
#############################################################################
# Makefile for proj4
# Susan Paradiso, CS502
#############################################################################

CC=gcc
CFLAGS=-c -g -pthread -Wall 
LDFLAGS= -pthread
SIM_SOURCES=p4utils.c p4test.c mailbox.c
SIM_OBJECTS=$(SIM_SOURCES:.c=.o)
SIM_EXECUTABLE=p4test 

all : $(SIM_SOURCES) $(SIM_EXECUTABLE)

$(SIM_EXECUTABLE): $(SIM_OBJECTS)
	$(CC) $(LDFLAGS) $(SIM_OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	\rm -rf *.o *.core core p4test




