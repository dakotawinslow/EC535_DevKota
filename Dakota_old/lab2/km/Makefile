# obj-m += hello-1.o
# obj-m += hello-2.o
# obj-m += hello-3.o
# obj-m += hello-4.o
# obj-m += hello-5.o
# obj-m += timer-1.o
obj-m += mytimer.o
# obj-m += test.o

ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
KERNEL_SRC := /home/winslowd/EC535/lab2/stock-linux-4.19.82-ti-rt-r33
DEST_DIR := /home/winslowd/EC535/lab2/km

# Add the C standard flag
# ccflags-y := -std=c11
ccflags-y += -fcompare-debug-second

all:
	make -C $(KERNEL_SRC) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
# make -C $(KERNEL_SRC) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_MOD_PATH=$(DEST_DIR) modules_install

clean:
	make -C $(KERNEL_SRC) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean