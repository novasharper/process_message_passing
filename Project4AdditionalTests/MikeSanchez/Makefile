# Michael Sanchez
# CS 502 - Cisco
# Makefile

CC=gcc
CFLAGS = -g -Wall

all: testmailbox1 testmailbox2 testmailbox3 testmailbox4 testmailbox5 testmailbox6 testmailbox7 mailboxTest

mailbox.o: mailbox.c mailbox.h
	$(CC) $(CFLAGS) -c mailbox.c

mailboxTest: mailboxTest.o mailbox.o
	    $(CC) $(CFLAGS) -lm -pthread mailboxTest.o mailbox.o -o mailboxTest

mailboxTest.o:
	$(CC) $(CFLAGS) -c mailboxTest.c

testmailbox1: testmailbox1.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox1.o mailbox.o -o testmailbox1

testmailbox1.o: 
	$(CC) -c $(CFLAGS) testmailbox1.c

testmailbox2: testmailbox2.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox2.o mailbox.o -o testmailbox2

testmailbox2.o: 
	$(CC) -c $(CFLAGS) testmailbox2.c

testmailbox3: testmailbox3.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox3.o mailbox.o -o testmailbox3

testmailbox3.o: 
	$(CC) -c $(CFLAGS) testmailbox3.c

testmailbox4: testmailbox4.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox4.o mailbox.o -o testmailbox4

testmailbox4.o:
	$(CC) -c $(CFLAGS) testmailbox4.c

testmailbox5: testmailbox5.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox5.o mailbox.o -o testmailbox5

testmailbox5.o: 
	$(CC) -c $(CFLAGS) testmailbox5.c

testmailbox6: testmailbox6.o mailbox.o
	$(CC) $(CFLAGS) -lm -pthread testmailbox6.o mailbox.o -o testmailbox6

testmailbox6.o: 
	$(CC) -c $(CFLAGS) -pthread testmailbox6.c

testmailbox7: testmailbox7.o mailbox.o
	$(CC) $(CFLAGS) -lm testmailbox7.o mailbox.o -o testmailbox7

testmailbox7.o: 
	$(CC) -Wall -c $(CFLAGS) testmailbox7.c

clean:
	rm -f *.o mailboxTest testmailbox1 testmailbox2 testmailbox3 testmailbox4 testmailbox5 testmailbox6 testmailbox7

