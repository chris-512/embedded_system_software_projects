obj-m := stopwatch.o

KDIR := /root/private/week4/4week/kernel
PWD := $(shell pwd)

default :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean : 
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .*.cmd
	rm -f *.symvers
	rm -f *.order
