all: userspace lkm;
userspace:
	$(MAKE) -C userspace
lkm:
	$(MAKE) -C mailbox_LKM
clean: clean_userspace clean_lkm;
clean_userspace:
	$(MAKE) -C userspace clean
clean_lkm:
	$(MAKE) -C mailbox_LKM clean