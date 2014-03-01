# Khazhismel Kumykov
# cs3013
obj-m := the_module.o
the_module-objs := test_lkm.o mailbox_hashtable.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
