#
#   Makefile
#   
#   Created on: Dec 11, 2011 
#   Andrew Foltan
#   CS-502_Cisco
#   Project4
#

all: test1 test2 test3 test4 test5 test6 test7 test8 test9 pidlisttest

test1: mailbox.c test_utils.c test1.c
	gcc -g -Wall -o test1 mailbox.c test_utils.c test1.c

test2: mailbox.c test_utils.c test2.c
	gcc -g -Wall -o test2 mailbox.c test_utils.c test2.c -lpthread

test3: mailbox.c test_utils.c test3.c
	gcc -g -Wall -o test3 mailbox.c test_utils.c test3.c -lpthread

test4: mailbox.c test_utils.c test4.c
	gcc -g -Wall -o test4 mailbox.c test_utils.c test4.c -lpthread

test5: mailbox.c test_utils.c test5.c
	gcc -g -Wall -o test5 mailbox.c test_utils.c test5.c -lpthread

test6: mailbox.c test_utils.c pidlist.c test6.c 
	gcc -g -Wall -lrt -o test6 mailbox.c test_utils.c pidlist.c test6.c

test7: mailbox.c test_utils.c pidlist.c test7.c 
	gcc -g -Wall -lrt -o test7 mailbox.c test_utils.c pidlist.c test7.c -lpthread

test8: mailbox.c test_utils.c pidlist.c test8.c 
	gcc -g -Wall -lrt -o test8 mailbox.c test_utils.c pidlist.c test8.c -lpthread

test9: mailbox.c test_utils.c test9.c
	gcc -g -Wall -o test9 mailbox.c test_utils.c test9.c

pidlisttest: pidlist.c pidlist_test.c
	gcc -g -lrt -Wall -o pidlisttest pidlist.c pidlist_test.c
	
clean:
	rm -f test1 test2 test3 test4 test5 test6 test7 test8 test9
	rm -f pidlisttest



