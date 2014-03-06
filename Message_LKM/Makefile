# Khazhismel Kumykov
# cs3013
obj-m := Message_LKM.o
mailbox_LKM-objs := module.o mailbox_manager.o mailbox.o message.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

unload:
	sudo rmmod Message_LKM

load:
	sudo insmod Message_LKM.ko
