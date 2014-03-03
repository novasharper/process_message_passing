all: userspace lkm Project4_SampleTests
userspace:
	$(MAKE) -C userspace
Project4_SampleTests:
	$(MAKE) -C Project4_SampleTests
lkm:
	$(MAKE) -C mailbox_LKM
clean: clean_userspace clean_lkm clean_Project4_SampleTests
clean_userspace:
	$(MAKE) -C userspace clean
clean_lkm:
	$(MAKE) -C mailbox_LKM clean
clean_Project4_SampleTests:
	$(MAKE) -C Project4_SampleTests clean