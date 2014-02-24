# Alfredo Porras
# July 12th, 2011
# CS 3013
# Project 4 - makefile

CFLAGS = -g -Wall

all: testmailbox1 testmailbox2 testmailbox3 testmailbox4 testmailbox5 testmailbox6 testmailbox7


mailbox.o: mailbox.c mailbox.h
	gcc -Wall -c $(CFLAGS) mailbox.c

testmailbox1: testmailbox1.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox1.o mailbox.o -o testmailbox1

testmailbox1.o: testmailbox1.c
	gcc -Wall -c $(CFLAGS) testmailbox1.c

testmailbox2: testmailbox2.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox2.o mailbox.o -o testmailbox2

testmailbox2.o: testmailbox2.c
	gcc -Wall -c $(CFLAGS) testmailbox2.c

testmailbox3: testmailbox3.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox3.o mailbox.o -o testmailbox3

testmailbox3.o: testmailbox3.c
	gcc -Wall -c $(CFLAGS) testmailbox3.c

testmailbox4: testmailbox4.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox4.o mailbox.o -o testmailbox4

testmailbox4.o: testmailbox4.c
	gcc -Wall -c $(CFLAGS) testmailbox4.c

testmailbox5: testmailbox5.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox5.o mailbox.o -o testmailbox5

testmailbox5.o: testmailbox5.c
	gcc -Wall -c $(CFLAGS) testmailbox5.c

testmailbox6: testmailbox6.o mailbox.o
	gcc $(CFLAGS) -lm -pthread testmailbox6.o mailbox.o -o testmailbox6

testmailbox6.o: testmailbox6.c
	gcc -Wall -c $(CFLAGS) -pthread testmailbox6.c

testmailbox7: testmailbox7.o mailbox.o
	gcc $(CFLAGS) -lm testmailbox7.o mailbox.o -o testmailbox7

testmailbox7.o: testmailbox7.c
	gcc -Wall -c $(CFLAGS) testmailbox7.c

clean:
	rm -f mailbox.o testmailbox1.o testmailbox1 testmailbox2.o testmailbox2 testmailbox3.o testmailbox3 testmailbox4.o testmailbox4 testmailbox5.o testmailbox5 testmailbox6.o testmailbox6 testmailbox7.o testmailbox7