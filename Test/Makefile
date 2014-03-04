all: mailbox_test mailbox_error_test

mailbox.o: mailbox.c mailbox.h
	gcc -Wall -c $(CFLAGS) mailbox.c

mailbox_test: mailbox.o
	gcc $(CFLAGS) -lm -pthread mailbox_test.c mailbox.o -o mailbox_test

mailbox_error_test: mailbox.o
	gcc $(CFLAGS) -pthread mailbox_error_messages_test.c mailbox.o -o mailbox_error_test

test_p: mailbox_test
	./mailbox_test

clean:
	rm -rf *.o mailbox_test mailbox_error_test