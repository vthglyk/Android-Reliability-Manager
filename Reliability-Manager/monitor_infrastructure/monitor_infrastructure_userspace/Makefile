
CC=${NDK_TOOLCHAIN}gcc
CFLAGS= --sysroot=${NDK_SYSROOT} 


# Make rules
all: monitor_userspace app_manager benchmark low_benchmark bill power low_test high_test very_high_test little_benchmark big_benchmark

monitor_userspace: monitor_userspace.c
	$(CC) $(CFLAGS) monitor_userspace.c -o monitor_userspace

app_manager: app_manager.c
	$(CC) $(CFLAGS) app_manager.c -o app_manager

power: power.c
	$(CC) $(CFLAGS) power.c -o power

benchmark: benchmark.c
	$(CC) $(CFLAGS) benchmark.c -o benchmark

low_benchmark: low_benchmark.c
	$(CC) $(CFLAGS) low_benchmark.c -o low_benchmark

bill: bill.c
	$(CC) $(CFLAGS) bill.c -o bill

low_test: low_test.c
	$(CC) $(CFLAGS) low_test.c -o low_test

high_test: high_test.c
	$(CC) $(CFLAGS) high_test.c -o high_test

very_high_test: very_high_test.c
	$(CC) $(CFLAGS) very_high_test.c -o very_high_test

little_benchmark: little_benchmark.c
	$(CC) $(CFLAGS) little_benchmark.c -o little_benchmark

big_benchmark: big_benchmark.c
	$(CC) $(CFLAGS) big_benchmark.c -o big_benchmark

clean:
	rm -f *.o monitor_userspace power app_manager benchmark low_benchmark bill low_test high_test very_high_test little_benchmark big_benchmark

