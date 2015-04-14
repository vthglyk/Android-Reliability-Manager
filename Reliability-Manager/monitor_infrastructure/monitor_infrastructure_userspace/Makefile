
CC=${NDK_TOOLCHAIN}gcc
CFLAGS= --sysroot=${NDK_SYSROOT} 


# Make rules
all: monitor_userspace app_manager benchmark

monitor_userspace: monitor_userspace.c
	$(CC) $(CFLAGS) monitor_userspace.c -o monitor_userspace

app_manager: app_manager.c
	$(CC) $(CFLAGS) app_manager.c -o app_manager

benchmark: benchmark.c
	$(CC) $(CFLAGS) benchmark.c -o benchmark

clean:
	rm -f *.o monitor_userspace app_manager benchmark

