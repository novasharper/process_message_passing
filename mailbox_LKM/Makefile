# Khazhismel Kumykov
# cs3013
obj-m := mailbox_LKM.o
mailbox_LKM-objs := module.o mailbox_manager.o mailbox.o message.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean