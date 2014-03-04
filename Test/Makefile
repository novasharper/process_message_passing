all: mailbox_test

mailbox.o: mailbox.c mailbox.h
	gcc -Wall -c $(CFLAGS) mailbox.c

mailbox_test: mailbox_test.o mailbox.o
	gcc $(CFLAGS) -lm mailbox_test.o mailbox.o -o mailbox_test

mailbox_test.o: mailbox_test.c
	gcc -Wall -c $(CFLAGS) mailbox_test.c

clean:
	rm -rf *.o mailbox_test