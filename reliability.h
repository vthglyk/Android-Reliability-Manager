
#ifndef _LINUX_RELIABILITY_H
#define _LINUX_RELIABILITY_H

// PIETRO

//----------------------------------- DEFINES -----------------------------------

#define CONFIG_RELIABILITY  //uncomment for enabling reliability control   // HERE IS WHAT BREAKS THE WIFI!!
#define READ_RELIABILITY_STATS // uncomment for enabling reliability_stats_reader.c for reading statistics and saving into the buffer.

#define LTC_SIGNAL

//#define READ_TEMP
//#define DEBUG_EXEC_TIME
//#define DEBUG_EXEC_TIME_2

//#define WIFI_DEBUG

//#define PIETRO_SYSFS
// --------------------------------------------------------------------------------


#ifdef CONFIG_RELIABILITY
// place for the functions inside of reliability_stats_reader


#ifdef READ_RELIABILITY_STATS 
//inline long reliability_update_stats(unsigned int pid_sched);

// note: these values should be the same as in the reliability_stats_module.c
#define RELIABILITY_EXPORT_PAGE 6	//Number of pages allocated for each buffer
#define RELIABILITY_EXPORT_LENGTH 1024	//Number of enries of kind "reliability_stats_data" inside the allocated buffer



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


#endif


extern unsigned long int V_MAX;
extern unsigned long int V_MIN;
extern unsigned long int f_MAX;
extern unsigned long int f_MIN;

extern unsigned long int Vf_table[2][15];



#if 0
#ifndef WIFI_DEBUG
struct perf_req_rel {
	unsigned int HL_flag;   //either H - 1 or L - 0
	unsigned int freq_req;
	};
#endif
#endif


#endif //READ_RELIABILITY_STATS 
#endif //CONFIG_RELIABILITY


