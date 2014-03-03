all: test lkm
test:
	$(MAKE) -C Test
lkm:
	$(MAKE) -C Message_LKM
clean: clean_test clean_lkm
clean_test:
	$(MAKE) -C Test clean
clean_lkm:
	$(MAKE) -C Message_LKM clean