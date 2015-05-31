CC=${NDK_TOOLCHAIN}gcc

CFLAGS= --sysroot=${NDK_SYSROOT}
LDFLAGS =  -lm -lc -ldl



# Installation variables
EXEC_NAME = LTC

# Files needed for the build
OBJS 	= LTC.o chi_square_density.o ln_gamma_function.o gamma_function.o

# Make rules
all: LTC
.c.o:
	$(CC) $(CFLAGS) -c $<

LTC: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(EXEC_NAME) ${OBJS}   


clean:
	rm -f *.o $(EXEC_NAME) 
