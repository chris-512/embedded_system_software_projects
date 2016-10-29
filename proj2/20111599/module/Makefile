obj-m	:= dev_driver.o

KDIR	:= /root/private/week4/4week/kernel
PWD		:= $(shell pwd)

all: driver
driver:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.c
	rm -rf *.order
	rm -rf *.symvers
