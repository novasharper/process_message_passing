all: userspace lkm;
userspace:
	$(MAKE) -c userspace
lkm:
	$(MAKE) -c mailbox_LKM
clean: clean_userspace clean_lkm;
clean_userspace:
	$(MAKE) -c userspace clean
clean_lkm:
	$(MAKE) -c mailbox_LKM clean