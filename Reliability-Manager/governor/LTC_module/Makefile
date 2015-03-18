obj-m := LTC_module.o 

all:
	make ARCH=arm CROSS_COMPILE=/home/sem15f6/toolchains/arm-eabi-4.6/bin/arm-eabi- -C /scratch/sem15f6/linux M=$(PWD) modules

clean:
	make -C /scratch/sem15f6/linux M=$(PWD) clean
