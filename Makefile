# Makefile for msidemo

ifeq ($(KERNELRELEASE),)

#    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    KERNELDIR ?= /lib/modules/4.16.0-041600-generic/build

    PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order *.rc .cache.mk .ethtool.o.d *ur-safe

.PHONY: modules modules_install clean

else

    obj-m	:= msi_demo.o

msi_demo-objs := msidemo.o

endif


