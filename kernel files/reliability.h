
#ifndef _LINUX_RELIABILITY_H
#define _LINUX_RELIABILITY_H

// PIETRO

//----------------------------------- DEFINES -----------------------------------

#define CONFIG_RELIABILITY  //uncomment for enabling reliability control   // HERE IS WHAT BREAKS THE WIFI!!
#define MONITOR_ON // uncomment for enabling monitor_userspace.c for reading statistics and saving into the buffer.
#define LTC_SIGNAL //enables communication with LTC
#define RELIABILITY_SCHED_ON //enables the integration of the scheduler to the reliability governor

//#define READ_TEMP
//#define DEBUG_EXEC_TIME
//#define DEBUG_EXEC_TIME_2

//#define WIFI_DEBUG

//#define PIETRO_SYSFS
// --------------------------------------------------------------------------------


#ifdef CONFIG_RELIABILITY
// place for the functions inside of reliability_stats_reader


#ifdef MONITOR_ON 

// note: these values should be the same as in the reliability_stats_module.c
#define MONITOR_EXPORT_PAGE 6	//Number of pages allocated for each buffer
#define MONITOR_EXPORT_LENGTH 1024 //Number of enries of kind "monitor_stats_data" inside the allocated buffer. It has to be same as in the driver and in the userspace program
#define EXYNOS_TMU_COUNT      5 // this should be the same as in exynos_thermal.c
#define NUM_CLUSTERS 2
#define CPU_PER_CLUSTER 4

struct monitor_stats_data {
                unsigned int cpu;
                unsigned long int j; //jiffies
                unsigned long int cycles;
                unsigned long int instructions;
                unsigned int temp[EXYNOS_TMU_COUNT] ;
                unsigned int power_id;
                unsigned int power ;
                unsigned int pid ;
                unsigned int volt ;
                unsigned int freq ;
                unsigned int fan ;  //fan speed
                int task_prio;
                int task_static_prio;
                unsigned int test ;
		unsigned int HL_flag;
};


#endif


extern unsigned long int V_MAX[NUM_CLUSTERS];
extern unsigned long int V_MIN[NUM_CLUSTERS];
extern unsigned long int f_MAX[NUM_CLUSTERS];
extern unsigned long int f_MIN[NUM_CLUSTERS];
extern unsigned int temp_monitor[EXYNOS_TMU_COUNT];
extern unsigned long int Vf_table[NUM_CLUSTERS][2][13];



#if 0
#ifndef WIFI_DEBUG
struct perf_req_rel {
	unsigned int HL_flag;   //either H - 1 or L - 0
	unsigned int freq_req;
	};
#endif
#endif


#endif //MONITOR_ON 
#endif //CONFIG_RELIABILITY



