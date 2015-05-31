#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>



#define SELECT_CPU 	1
#define READY 		2
#define S_READY 	3
#define PASS_H_DIM	4


void main(int argc, char ** argv){

	int H_table_dim = 0;
	int fd;
	long unsigned int pid[100];
	long unsigned int tmp;

	printf("Please give the pids of the high priority applications\n");
	printf("If you want to stop give a number <= 0\n");

	do {
		scanf("%lu", &tmp);
		pid[H_table_dim] = tmp;
		if(tmp > 0)
			H_table_dim++;

	} while(tmp > 0);

	// open monitor driver
	fd = open("/dev/monitor",O_RDWR);
	if( fd == -1) {
                printf("Monitor driver open error...\n");
                exit(0);
        }

	printf("Attempting to ioctl PASS H_table_dim = %u!\n", H_table_dim);
	sleep(1);

	ioctl(fd, PASS_H_DIM, &H_table_dim);
	printf("ioctl PASS ok!, H_table_dim = %d\n", H_table_dim);

	write(fd, pid, sizeof(long unsigned int)*H_table_dim);
	printf("write H_table ok!\n" );

	sleep(1);
	printf("You chose to exit the app_manager\n");
	close(fd);
}
