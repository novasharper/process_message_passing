all: mailbox_test1

mailbox.o: mailbox.c mailbox.h
	gcc -Wall -c $(CFLAGS) mailbox.c

mailbox_test1: mailbox_test1.o mailbox.o
	gcc $(CFLAGS) -lm mailbox_test1.o mailbox.o -o mailbox_test1

mailbox_test1.o: mailbox_test1.c
	gcc -Wall -c $(CFLAGS) mailbox_test1.c

clean:
	rm -rf *.o mailbox_test1