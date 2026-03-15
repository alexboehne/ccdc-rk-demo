obj-m += sock-test.o 

CURRENT_DIR := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURRENT_DIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURRENT_DIR) clean
