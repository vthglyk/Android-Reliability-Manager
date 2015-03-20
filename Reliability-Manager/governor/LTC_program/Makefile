CC=${NDK_TOOLCHAIN}gcc

CFLAGS= --sysroot=${NDK_SYSROOT}

# Make rules
all: LTC

LTC: LTC.c
	$(CC) $(CFLAGS) LTC.c  -Wall -g -lc -ldl -lm -o LTC  


clean:
	rm -f *.o LTC core 
