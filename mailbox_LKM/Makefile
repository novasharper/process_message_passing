# Khazhismel Kumykov
# cs3013
obj-m := mailbox_LKM.o
mailbox_LKM-objs := mailbox.o mailbox_util.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean