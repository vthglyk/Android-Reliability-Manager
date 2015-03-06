

//---------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Sched for affinity macros
#define  __USE_GNU
//#include <sched.h>

#include <sched.h>
#include <unistd.h>
#include <string.h>

#include <sys/syscall.h>
#include <pthread.h>

#include <sys/wait.h> //for wait()

/* 	
define macros used in ioctl
note: they must the same as the one defined in reliability_stats_module
*/
#define PASS_H_DIM	9
#define SELECT_CPU	4
#define READY		1
#define S_READY		5
#define UPDATE_V_LTC	6
#define GET_DELTA_LI	7
#define GET_V_LI	8


#define CPU_SETSIZE 1024
#define __NCPUBITS  (8 * sizeof (unsigned long))
typedef struct
{
   unsigned long __bits[CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

#define CPU_SET(cpu, cpusetp) \
  ((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
  memset((cpusetp), 0, sizeof(cpu_set_t))






void setCurrentThreadAffinityMask(cpu_set_t mask)
{
    int err, syscallres;
	pid_t pid = gettid();
	printf("debug funct pid = %u\n", pid);
    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
    if (syscallres)
    {
        err = errno;

    }
}

/*
define the structure of data that is passed
it must be the same as the one defined in reliability.h
*/
struct reliability_stats_data {
		unsigned int cpu;
		unsigned long int j; //jiffies
		unsigned long int cycles;
		unsigned long int instructions;
		unsigned int pid_sched;
		unsigned int pid_gov;
  		unsigned long int t_LI;
		unsigned int HL_flag;
		unsigned long int V_STC;
		unsigned long int V_LI;
		unsigned long int V_LTC;
		unsigned long int V_APP;
		unsigned long int f_APP;  //the one inside of the governor
		char temp0;
		char temp1;
		char temp2;
};


/*
define the length of the buffer to export data
the length must be the same of the one defined in reliability_stats_module.
*/
#define RELIABILITY_EXPORT_LENGTH 1024	

#define NUM_CPU 4


// son declaration
void son(int cpu, int max_read, int fd);



// ---------------- MAIN FUNCTION ----------------- //

/*
(description)
1) open the device for reading statistics
2) performs a continuous polling on the 4 cpus until the value 
   of "ready" (read through ioctl) changes. This means that 
   in the kernel space one buffer is become full.
*/

void main(int argc, char ** argv){


	long unsigned int tmp ,H_table[10];
	int fd, i, num_active_cpus, H_table_dim = 0;
	int max_read = 35;
	int np = 0, ipid, status;
	int pidM[NUM_CPU -1];
        cpu_set_t mask;
	FILE *fpid;


	printf("Starting father and son :   it's not tiiiimeeee to make a chaaaange.....\n");
	fd = open("dev/rel" , O_RDWR);

	if( fd == -1) {
		printf("open error...\n");
		exit(0);
	}

	//handle and write to kernel space the H_table
	
	if ( ( fpid = fopen("data/H_table.txt","r") ) == NULL ){
		printf("file not found...\n");
		exit(0);
	}

	printf("PIETRO DEBUG userspace : H_table[%u] = %lu, H_table_dim = %u\n", (H_table_dim ) , H_table[H_table_dim] , H_table_dim);
	
	 while (   ( fscanf(fpid, "%lu", &tmp ) ) != EOF ) { ;
		H_table[H_table_dim] = tmp;
		H_table_dim++;
		printf("PIETRO DEBUG userspace : tmp = %lu,  H_table[%d] = %lu, H_table_dim = %u\n", 
					tmp	 ,(H_table_dim -1) , H_table[0] , H_table_dim);
	}
	fclose(fpid);

	printf("PIETRO DEBUG userspace : attempting to ioctl PASS H_table_dim = %u!\n", H_table_dim);
	sleep(1);

	
	ioctl(fd, PASS_H_DIM, &H_table_dim);



	printf("PIETRO DEBUG userspace : ioctl PASS ok!\n");
	//sleep(1);




	write(fd, H_table, sizeof(long unsigned int)*H_table_dim);
	printf("PIETRO DEBUG userspace : write H_table ok!\n" );

	sleep(1);

	num_active_cpus = atoi(argv[1]);
	printf ("NUMBER OF ACTIVE CPU IS %u\n", num_active_cpus );

	// note: you loop only on the number of active cpus! 
	//this is passed as a parameter when the program is launched
        for(i=0; i<num_active_cpus; i++) {
		pidM[i] = fork();


                if (pidM[i] != 0) {
			printf("debug : pidM[%u] = %u\n", i , pidM[i]);
                        son(i,max_read,fd);

			while(np < num_active_cpus){
			        waitpid(pidM[np] ,  &status , 0);
			        printf("Father %d - son is dead!! , pid = %u\n",np, pidM[np]);
			        np++;
		        }
                        exit(0);
                }
                else{        		
			CPU_ZERO(&mask);
		        CPU_SET(i, &mask);
	                setCurrentThreadAffinityMask(mask);

                }
		sleep(1);
         }
	
         

	printf("FATHER Probe End \n");
	close(fd);
        exit(0);

}




void son(int cpu, int max_read, int fd){

	// variables definition
	int len, i;	
	int  ready , ready_old = 1 ;
	char buf[3], stmp[30] = "data/parallel_cpu_";
	char *log;
	FILE *fp;
	struct reliability_stats_data *log_struct;

struct timeval tv;

time_t time_before , time_after;


	sprintf(buf,"%d",cpu);
	strcat(stmp,buf);
	strcat(stmp,".txt");
	printf("filename = %s\n",stmp);
	if((fp = fopen(stmp,"w")) == NULL){
		printf("file not found...\n");
		exit(0);
	}	

	printf("Starting son on CPU %u \n", cpu);

	log = malloc (sizeof(struct reliability_stats_data)*RELIABILITY_EXPORT_LENGTH);

	
	ioctl(fd, SELECT_CPU, &cpu);
	ioctl(fd, S_READY, &ready);

	// start the reading cycle
	while (ready_old <= max_read){

		ioctl(fd, SELECT_CPU, &cpu);
		ioctl(fd, S_READY, &ready);
		printf("ready old = %u and ready = %u", ready_old, ready);
		while ( (ready == ready_old)  &&   (ready != 0) ){
			printf("CPU %u Not ready yet...\n", cpu);
			sleep(4);  //let it sleep for a while before polling again
			ioctl(fd, SELECT_CPU, &cpu);
			ioctl(fd, S_READY, &ready);
			printf("READY = %u\n", ready);
		}

/*
 gettimeofday(&tv, NULL); 
 time_before=tv.tv_usec;
*/

		printf("Ready for reading!!\n");
		//flush the buffer

		ioctl(fd, SELECT_CPU, &cpu);
		len = read(fd , log , RELIABILITY_EXPORT_LENGTH);
		// verify correct reading
		if (len == -1){
			printf("Error while reading buffer...\n");
			exit(1);
		}
		
		printf("Start writing data on file !! \n");
		log_struct = (struct reliability_stats_data *)log;
		
		for(i = RELIABILITY_EXPORT_LENGTH-1 ; i>=0 ; i--){
			fprintf(fp,
					"%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n", 
						log_struct[i].cpu,
						log_struct[i].j, //jiffies
						log_struct[i].cycles,
						log_struct[i].instructions,
						log_struct[i].pid_sched,
						log_struct[i].pid_gov,
						log_struct[i].t_LI,
						log_struct[i].HL_flag,
						log_struct[i].V_STC,
						log_struct[i].V_LI,
						log_struct[i].V_LTC,
						log_struct[i].V_APP,
						log_struct[i].f_APP,
						log_struct[i].temp0,
						log_struct[i].temp1,
						log_struct[i].temp2 );  //the one inside of the governor

		}

/*
gettimeofday(&tv, NULL); 
 time_after=tv.tv_usec;

printf("\n\nTIME FOR FLUSHING ONE BUFFER= %d\n\n" ,time_after - time_before );
*/

		ready_old = ready;
	}
	fclose(fp);

	printf("SON Probe End !! \n");
	free(log);
	exit(1);
}


