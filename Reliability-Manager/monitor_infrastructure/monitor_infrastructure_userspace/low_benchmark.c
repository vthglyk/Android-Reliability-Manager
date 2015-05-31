// Sched for affinity macros
//#define _GNU_SOURCE
//#define  __USE_GNU
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>

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


#define NUM_PROCESSI 0
#define NUM_CPU 4
#define ITERS   64*1024*1024
#define DIM_H   1024*8
//#define DIM_L 1677772 //1024*16*1204
#define DIM_L   1024*1*1204
#define STEP 8
#define CPU_FATHER  0
#define NTIME 10  //Specify the re-iteration number of the


void figlio(int cpu, unsigned int *A,int lim, int iter_length);
void setCurrentThreadAffinityMask(cpu_set_t mask);
/*void figlio_master(int cpu);*/

//void get_idleness(unsigned int *idle, unsigned int *tot);

main(int argc , char *argv[]) {

        int pidM[NUM_CPU],ppid;
        unsigned int *pointM[NUM_CPU -1],activeCPU;
        int   status,i=0,np=0;
        cpu_set_t mask;
        double endT;
        double astT;
  int iter_length, lim, nactiveCPU=0, temp_activeCPU;
	int bill;

// PARSE INPUT FROM COMMAND LINE
  if ((argc == 1)|(argc == 2)|(argc == 3)){
    printf("ERROR!!! Insufficient number of inputs: Specify the CPU MASK(hex), the buffer locality and the iteration length \n");
    printf("Example ./syntetic FFFF 1024 30 \n");
    printf("Example ./syntetic 00FF 1024 30 \n");
    exit(1);
  }
  if (argc == 4){
    activeCPU = strtol(argv[1],NULL,16);
    lim = atoi(argv[2]);
    iter_length = atoi(argv[3]);
  }
        temp_activeCPU = activeCPU;
        ppid=getpid();
	pidM[0] = ppid;
  /********* Measure the time took by the entire benchmark ************/
//        wtime(&astT);
        /*if(sched_setaffinity( 0, sizeof(mask), &mask ) == -1)
                printf("WARNING: Could not set CPU Affinity, continuing...\n");*/


   /* Load the power virus in each core and assign the thread affinity*/
                for(i=1; i<NUM_CPU; i++){
                        if ((temp_activeCPU)&(1L)){
                                printf("DEBUG - FORK CPU %d\n",i);
                                pidM[i] = fork();
                                if (pidM[i] == 0) {
                                        pointM[i] = (unsigned int*)malloc((lim*1024)*sizeof(unsigned int));
                                        /* For each CPU fork the son process. */
                                        figlio(i, pointM[i],lim,iter_length);
                                        exit(0);
                                }
                                else{
                                        /* If father change its affinity */
                                        /*CPU_ZERO(&mask);
                                        CPU_SET(i, &mask);
                                        if(sched_setaffinity( pidM[i], sizeof(mask), &mask ) == -1)
                                                printf("WARNING: Could not set CPU Affinity, continuing...\n");*/
                                        nactiveCPU = nactiveCPU +1 ;
                                }
                        }
                        temp_activeCPU = temp_activeCPU >> 1;
                 }
                printf("DEBUG - Padre master figli partoriti!!\n");
		
		for(bill = 0; bill < NUM_CPU; bill++) {
			
			printf("pidM[%d] = %d\n", bill, pidM[bill]);
		}
		pointM[0] = (unsigned int*)malloc((lim*1024)*sizeof(unsigned int));
		figlio(0, pointM[0],lim,iter_length);
	
		while(np < nactiveCPU){
                        wait(&status);
                        printf("Padre %d figlio morto!!\n",np);
                        np++;
                }

  /********* Measure the time took by the entire benchmark ************/
  //      wtime (&endT);
        printf("HEADER;TIME;NITER;LIM;CPUS;\n");
        printf("%f;%d;%d;%x\n",(endT -astT), iter_length, lim, activeCPU);
        exit(0);
} /* fine padre */

/*
void figlio_master(int cpu) {
        int j=0,i=0,eP=0 ;
        //unsigned int pointM[1024*1024*2];
        unsigned int * pointM;
        int ipid=0, status;
        int lim;
        cpu_set_t mask;
        printf("DEBUG - Master figlo %d \n",cpu);
        if (cpu == 15 ){
                pointM = (unsigned int*)malloc((1024*1024*16)*sizeof(unsigned int));
                lim = 6;

        }else{
                 pointM = (unsigned int*)malloc((1024*1024*16)*sizeof(unsigned int));
                 lim = 6;
        }


        while(eP <= NUM_PROCESSI){
                        if ((ipid = fork())< 0){
                                printf("Failed to fork process %d",eP);
                                exit(1);
                        }
                        if (ipid == 0){
                                figlio(eP,cpu, pointM,lim);

                        }else {
                                CPU_ZERO(&mask);
                                CPU_SET(cpu, &mask);
                                if(sched_setaffinity( ipid, sizeof(mask), &mask ) == -1)
                                        printf("WARNING: Could not set CPU Affinity, continuing...\n");
                                wait(&status);
                                eP ++;
                                //printf("DEBUG - ddFM: CPU%d eP= %d diff=%d \n", cpu,eP);
                        }
        }
        free(pointM);
        //printf("MASTER: %d eP= %d going to die \n", cpu,eP);
        exit(0);
}*/

void figlio(int cpu, unsigned int *A,int lim, int iterLength) {

        unsigned int * esi_reg ;
        unsigned int * ebx_reg ;
        unsigned int eax_reg = 0;
        unsigned int *pippo;
        unsigned long int niter=1L;
        double intStT;
        double intEndT;
        int miopid;
        int t = 0;
        int ii=0;
        cpu_set_t mask;

        //miopid=getpid();

        /***** Already done in the father ***********
        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);

        if(sched_setaffinity( miopid, sizeof(mask), &mask ) == -1)
                                printf("WARNING: Could not set CPU Affinity, continuing...\n");
        */

        // Ubcommet if you want to repete the power virus NTIMES
        //for (ii=0;ii<NTIME;ii++){
                /*niter = (1L << iterLength) ; // 64 M cycle * 8

                eax_reg = 1L;
                ebx_reg = &A[1024*lim];
                esi_reg = &A[0];

                A[0] = 0;

                //wtime (&intStT);
                asm volatile ("mov %1,%4 \n\t"
                        " a_st1: add (%1),%3 \n\t"
                        " mov %3, (%1) \n\t     "
                        " add $128, %1 \n\t     "
                        " cmp %2, %1  \n\t      "
                        " cmovnbe %4, %1 \n\t   "
                        " loop a_st1      "
                        : "=c"(niter), "=S"(esi_reg), "=b"(ebx_reg), "=a"(eax_reg),"=D"(esi_reg)
                        : "c"(niter), "S"(esi_reg), "b"(ebx_reg), "a"(eax_reg),"D"(esi_reg));

		
                //wtime (&intEndT);
                printf("%d\tsize = %d\tCPU = %d\tPID = %d\tPr_n %d\tDT = %f\teax= %d, esi= %d \n",ii,lim*1024, cpu, miopid, (intEndT -intStT), eax_reg,esi_reg);*/
	
        //}
        //exit(0);
	printf("CPU%d: START\n", cpu);
	while(1) {
		t++;
		if (t > 200000000) {
			t = 0;
			printf("Hello from CPU%d\n",cpu);
			sleep(1);
		}
	}			
	printf("t = %d", t);	
}

void setCurrentThreadAffinityMask(cpu_set_t mask)
{
    int err, syscallres;
    pid_t pid = gettid();
    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
    if (syscallres)
    {
        err = errno;
        printf("Error in the syscall setaffinity: mask=%d=0x%x err=%d=0x%x", mask, mask, err, err);
    }
}

