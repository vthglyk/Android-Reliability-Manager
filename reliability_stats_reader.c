
#include <linux/reliability.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/slab.h>




DECLARE_PER_CPU(unsigned long int, Vf_APP_index);
DECLARE_PER_CPU(unsigned long int, delta_SI);  //in principle : scheduling tick;
DECLARE_PER_CPU(unsigned long int, t_LI);
DECLARE_PER_CPU(unsigned long int, V_LI);
DECLARE_PER_CPU(unsigned long int, delta_LI);
DECLARE_PER_CPU(unsigned long int, V_LTC);
DECLARE_PER_CPU(unsigned long int, V_STC);
DECLARE_PER_CPU(unsigned long int, V_APP);
DECLARE_PER_CPU(unsigned long int, f_APP);
DECLARE_PER_CPU(unsigned int, HL_flag);
DECLARE_PER_CPU(unsigned int, pid_gov);
DECLARE_PER_CPU(unsigned int, reliability_gov_ready);


#ifdef READ_RELIABILITY_STATS

DEFINE_PER_CPU(struct reliability_stats_data *, reliability_stats_data);  // address of the buffer #1: to be declared in the reliability_stats_module as well
DEFINE_PER_CPU(struct reliability_stats_data *, reliability_stats_data2);  // address of the buffer #2: to be declared in the reliability_stats_module as well
DEFINE_PER_CPU(int , reliability_stats_index) = 0; //index inside of the stats buffer
DEFINE_PER_CPU(int , reliability_stats_start) = 0; //incremented each time a buffer is filled. useful in conditions.

EXPORT_PER_CPU_SYMBOL(reliability_stats_data);
EXPORT_PER_CPU_SYMBOL(reliability_stats_data2);
EXPORT_PER_CPU_SYMBOL(reliability_stats_index);
EXPORT_PER_CPU_SYMBOL(reliability_stats_start);


//declaration of functions for register reading

#ifdef READ_TEMP
unsigned long temp_gov[10] = {	0,0,0,0,0,
				0,0,0,0,0};
EXPORT_SYMBOL(temp_gov);
#endif

#ifdef LTC_SIGNAL
pid_t pid_p = 0;
EXPORT_SYMBOL(pid_p);
#endif


unsigned long int temperature_module[10] = {	0,0,0,0,0,
						0,0,0,0,0};
EXPORT_SYMBOL(temperature_module);

//inline ?

//--- variables for pid and signal passing ---//

struct siginfo signal_info;
EXPORT_SYMBOL(signal_info);
struct task_struct *signal_task_struct;
EXPORT_SYMBOL(signal_task_struct);


inline int reliability_update_stats(unsigned int pid_sched, int cpu) {

	unsigned long int cycles, instructions;
	//int i;
	struct reliability_stats_data * tmp;


	#ifdef DEBUG_EXEC_TIME
	unsigned long int c_after , c_diff ; 
	#endif


	



	if(  ( __get_cpu_var(reliability_stats_start) > 0  )    &&    ( __get_cpu_var(reliability_gov_ready) == 1 )   ){  

		

		asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
		asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
		asm volatile ("MCR p15, 0, %0, c9, c13, 1\t\n" :: "r"(0x00000008));   //select_event

		asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (instructions));
		

		//save data into the buffer
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].cpu = cpu;
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].j = jiffies;
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].cycles = cycles;
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].instructions = instructions;
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].pid_sched = pid_sched;
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].pid_gov = __get_cpu_var(pid_gov); //__get_cpu_var(pid_gov); 
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].t_LI = __get_cpu_var(t_LI);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].HL_flag = __get_cpu_var(HL_flag);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].V_STC = __get_cpu_var(V_STC);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].V_LI = __get_cpu_var(V_LI);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].V_LTC = __get_cpu_var(V_LTC);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].V_APP = __get_cpu_var(V_APP);
		__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].f_APP = __get_cpu_var(f_APP);

		#ifdef READ_TEMP
		if (cpu == 0){		
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temp_gov[0];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temp_gov[1];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)temp_gov[2];
		}
		else if (cpu == 1){
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temp_gov[3];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temp_gov[4];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)temp_gov[5];

		}
		else if (cpu == 2){
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temp_gov[6];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temp_gov[7];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)2;

		}
		else{
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temp_gov[8];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temp_gov[9];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)2;

		}
		#else
		if (cpu == 0){		
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temperature_module[0];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temperature_module[1];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)temperature_module[2];
		}
		else if (cpu == 1){
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temperature_module[3];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temperature_module[4];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)temperature_module[5];

		}
		else if (cpu == 2){
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temperature_module[6];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temperature_module[7];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)2;

		}
		else{
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp0 = (char)temperature_module[8];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp1 = (char)temperature_module[9];
			__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) ].temp2 = (char)2;

		}
		#endif


		#ifdef DEBUG_EXEC_TIME
		//debug
		if (__get_cpu_var(t_LI)%1000 == 0){
			asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x00000001));   //program_pmcr
			asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));   //enable_all_counter
			asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (c_after));
			c_diff = c_after - cycles ; 
			printk (KERN_ALERT "PIETRO DEBUG EXEC TIME : cycles_difference = %lu" , c_diff);
		} 
		#endif	
		

		//swap buffers when full
		if ( __get_cpu_var(reliability_stats_index)==0 ){

			//debug
			/*for_each_online_cpu(i){
			
			printk(KERN_ALERT "PIETRO ALERT : reliability_stats_reader.c LOOP : CPU %lu" , 
				per_cpu(reliability_stats_data,i)[ per_cpu(reliability_stats_index,i) +1 ].cpu);


			}*/
	
			printk(KERN_ALERT "PIETRO ALERT:reliability_stats_reader.c : CPU %u - buffer full with reliability_stats_start = %u!! and idx = %u",
							__get_cpu_var(reliability_stats_data)[ __get_cpu_var(reliability_stats_index) +1 ].cpu
								, __get_cpu_var(reliability_stats_start) ,__get_cpu_var(reliability_stats_index));

			tmp = __get_cpu_var(reliability_stats_data);
			__get_cpu_var(reliability_stats_data) = __get_cpu_var(reliability_stats_data2);
			__get_cpu_var(reliability_stats_data2) = tmp;
			__get_cpu_var(reliability_stats_index) = RELIABILITY_EXPORT_LENGTH;
			__get_cpu_var(reliability_stats_start)++;  //count +1 on the number of filled buffers
			
		}

		__get_cpu_var(reliability_stats_index)--;  //note: the index is decreased!

	}
	return 0;
}

#endif //READ_RELIABILITY_STATS


//-------------------------------------------
// functions for reading registers
/*
static int armv7_pmnc_select_counter(int idx)
{
	u32 counter;

	counter = ARMV7_IDX_TO_COUNTER(idx);
	asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r" (counter));  //Event Counter Selection Register - PMSELR
	//[4:0] SEL Counter select: b00000 = select counter 0 b00001 = select counter 1 b11111 = access the Cycle Count Filter Control Register.

	isb();

	return idx;
}
static u32 armv7pmu_read_counter(int idx)
{
	u32 value = 0;


	if (idx == ARMV7_IDX_CYCLE_COUNTER)
		asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (value));
	else if (armv7_pmnc_select_counter(idx) == idx)
		asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r" (value));

	return value;
}
*/





