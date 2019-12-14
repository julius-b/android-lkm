obj-m += lkm1.o
obj-m += lkm_simple.o

KERNEL_PATH := $(HOME)/git/kernel/
CCPATH := $(HOME)/git/aarch64-linux-android-4.9/bin/
#CCPATH_EXT := $(CCPATH)arm64-linux-androideabi
CROSS_COMPILE := aarch64-linux-android-
ARCH=arm64
SUBARCH=arm64

export PATH := ${CCPATH}:${PATH}

#CFLAGS += -fPIC
#CFLAGS += -fno-pic

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) CFLAGS=$(CFLAGS) -C $(KERNEL_PATH) M=$(PWD) modules -j$(nproc --all)

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
