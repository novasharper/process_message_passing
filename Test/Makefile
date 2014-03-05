all: mailbox_test mailbox_error_test

mailbox.o: mailbox.c mailbox.h
	gcc -Wall -c $(CFLAGS) mailbox.c

mailbox_test: mailbox.o
	gcc $(CFLAGS) -lm -pthread mailbox_test.c mailbox.o -o mailbox_test

mailbox_error_test: mailbox.o
	gcc $(CFLAGS) -pthread mailbox_error_messages_test.c mailbox.o -o mailbox_error_test

test_k: mailbox_error_test
	./mailbox_error_test
#	@echo "-------------------------------- TESTING PROCESS --------------------------------"
#	@echo "run ./mailbox_error_test - for whatever reason make doesn't allow me to join more"
#	@echo "than 5 threads, while running it normally works fine"
#	@echo "---------------------------------------------------------------------------------"

test_p: mailbox_test
	./mailbox_test

clean:
	rm -rf *.o mailbox_test mailbox_error_test