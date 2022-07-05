KERN_DIR:=/home/ZYNQ/xilinx-linux/linux-xlnx-xilinx-v2018.3

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-

obj-m:=led.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=`pwd` modules
clean:
	make -C $(KERN_DIR) M=`pwd` clean
