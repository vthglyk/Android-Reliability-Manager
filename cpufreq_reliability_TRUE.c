/*
 *  drivers/cpufreq/cpufreq_reliability.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
Pietro's changes:  (ctrl+F: "P")
- in dbs_check_cpu
- in dbs_freq_increase

*/



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>


//#include "cpufreq_reliability_test.h"   // contains test vectors 

// PIETRO
#include <linux/reliability.h>
//#include <linux/HL_table.h>


#ifdef READ_TEMP
#include <linux/msm_tsens.h>
#endif

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define PIETRO_CUT_AWAY				0

#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(10)
#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define DEF_SAMPLING_DOWN_FACTOR		(1)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL	(3)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(11)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)
#define MIN_FREQUENCY_DOWN_DIFFERENTIAL		(1)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

#define POWERSAVE_BIAS_MAXLEVEL			(1000)
#define POWERSAVE_BIAS_MINLEVEL			(-1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_RELIABILITY
//static
#endif

/*----pietro's variables----*/
//int index_pietro=0;



#ifdef MONITOR_ON
//DECLARE_PER_CPU(int , reliability_stats_start);
#endif


#ifdef CONFIG_RELIABILITY

#define NUM_CLUSTERS 2
#define CPUS_PER_CLUSTER 4

static unsigned int search_in_Vf_table(unsigned int V_STC);
static long unsigned int search_in_H_table(int pid); 

// Variables for the reliability management algorithm

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

//debug

DECLARE_PER_CPU(int , monitor_stats_start);


unsigned long int Vf_table[NUM_CLUSTERS][2][15] = 
{
{
{950,975,1000,1025,1075,1100,1125,1175,1200,1225,1237,1250,1250,1250,1250},
//{950000,975000,1000000,1025000,1075000,1100000,1125000,1175000,1200000,1225000,1237500,1250000,1250000,1250000,1250000},
{1000000,1028571,1057142,1085714,1114285,1142857,1171428,1200000,1228571,1257142,1285714,1314285,1342857,1371428,1400000} 
},
{
{950,975,1000,1025,1075,1100,1125,1175,1200,1225,1237,1250,1250,1250,1250},
//{950000,975000,1000000,1025000,1075000,1100000,1125000,1175000,1200000,1225000,1237500,1250000,1250000,1250000,1250000},
{384000,486000,594000,702000,810000,918000,1026000,1134000,1242000,1350000,1458000,1512000,1566000,1620000,1674000}
}
};

unsigned long int V_MAX = 1250;
unsigned long int V_MIN = 950;
//unsigned long int V_MAX = 1250000;  //PIETRO: if you use the "long" version, there are problems of overflow when multiplying 
//unsigned long int V_MIN = 950000;
unsigned long int f_MAX[NUM_CLUSTERS] = {1400000, 2000000};
unsigned long int f_MIN[NUM_CLUSTERS] = {1000000, 1200000};

//BILL
extern int H_table_dim;
extern long unsigned int *H_table;

DECLARE_PER_CPU(unsigned int , activate_safe_mode);

DEFINE_PER_CPU(long unsigned int , jiffies_prec) = 0;
DEFINE_PER_CPU(unsigned int , enter_signal) = 0;

#ifdef READ_TEMP
extern long unsigned int temp_gov[10];
extern int32_t tsens_get_temp(struct tsens_device *dev, unsigned long *temp);
#endif

#ifdef LTC_SIGNAL
extern pid_t pid_p;
#endif

//--------------- stuff for signals ----------------------

/*
#define SIG_PIETRO 32
extern struct siginfo signal_info;
extern struct task_struct *signal_task_struct;
*/

#endif //CONFIG_RELIABILITY

struct cpufreq_governor cpufreq_gov_reliability = {
       .name                   = "reliability",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	unsigned int rate_mult;
	int cpu;
	unsigned int sample_type:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info);
static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct workqueue_struct *input_wq;

static DEFINE_PER_CPU(struct work_struct, dbs_refresh_work);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int down_differential;
	unsigned int ignore_nice;
	unsigned int sampling_down_factor;
	int          powersave_bias;
	unsigned int io_is_busy;
	#ifdef PIETRO_SYSFS
	unsigned int sysfs_V_LTC;
	#endif
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
	.down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
	.ignore_nice = 0,
	.powersave_bias = 0,
	#ifdef PIETRO_SYSFS
	.sysfs_V_LTC = 1200,
	#endif
};

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}



/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
/*static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
					  unsigned int freq_next,
					  unsigned int relation)
{
	unsigned int freq_req, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	int freq_reduc;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
						   policy->cpu);

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	// Find freq bounds for freq_avg in freq_table 
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	//Find out how long we have to be in hi and lo freqs 
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}*/

static int ondemand_powersave_bias_setspeed(struct cpufreq_policy *policy,
					    struct cpufreq_policy *altpolicy,
					    int level)
{
	if (level == POWERSAVE_BIAS_MAXLEVEL) {
		#if 0
		/* maximum powersave; set to lowest frequency */
		/*__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->min : policy->min,
			CPUFREQ_RELATION_L);*/

		//P: 
		//printk(KERN_ALERT "PIETRO ALERT: I'm in ondemand_powersave_bias_setspeed with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
		__cpufreq_driver_target(policy, freq_test[index_pietro], 
				(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
		index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}
		#endif
		return 1;
	
	} else if (level == POWERSAVE_BIAS_MINLEVEL) {
		#if 0
		/* minimum powersave; set to highest frequency */
		/*__cpufreq_driver_target(policy,
			(altpolicy) ? altpolicy->max : policy->max,
			CPUFREQ_RELATION_H);*/

		//P: 
		//printk(KERN_ALERT "PIETRO ALERT: I'm in ondemand_powersave_bias_setspeed with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
		__cpufreq_driver_target(policy, freq_test[index_pietro], 
				(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
		index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}

		return 1;
		#endif
	}
	return 0;
}

static void ondemand_powersave_bias_init_cpu(int cpu)
{
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
	printk (KERN_ALERT "BILL DEBUG : In ondemand_powersave_bias_init_cpu for cpu = %u\n" , cpu);
}

static void ondemand_powersave_bias_init(void)
{
	int i;
	printk (KERN_ALERT "BILL DEBUG : In ondemand_powersave_bias_init\n" );
	for_each_online_cpu(i) {
		ondemand_powersave_bias_init_cpu(i);
	}
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_ondemand Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(up_threshold, up_threshold);
show_one(down_differential, down_differential);
show_one(sampling_down_factor, sampling_down_factor);
show_one(ignore_nice_load, ignore_nice);


#ifdef PIETRO_SYSFS
show_one(sysfs_V_LTC, sysfs_V_LTC);

static ssize_t store_sysfs_V_LTC(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.sysfs_V_LTC = input;
	return count;
}

#endif


static ssize_t show_powersave_bias
(struct kobject *kobj, struct attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dbs_tuners_ins.powersave_bias);
}

/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updating
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from ondemand
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
	int cpu;
	
	printk (KERN_ALERT "BILL DEBUG : In update_sampling_rate for cpu \n");
	dbs_tuners_ins.sampling_rate = new_rate
				     = max(new_rate, min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(od_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);

		if (!delayed_work_pending(&dbs_info->work)) {
			mutex_unlock(&dbs_info->timer_mutex);
			continue;
		}

		next_sampling  = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->work.timer.expires;


		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->timer_mutex);
			cancel_delayed_work_sync(&dbs_info->work);
			mutex_lock(&dbs_info->timer_mutex);

			schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work,
						 usecs_to_jiffies(new_rate));

		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	update_sampling_rate(input);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_down_differential(struct kobject *a, struct attribute *b,
		const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input >= dbs_tuners_ins.up_threshold ||
			input < MIN_FREQUENCY_DOWN_DIFFERENTIAL) {
		return -EINVAL;
	}

	dbs_tuners_ins.down_differential = input;

	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input  = 0;
	int bypass = 0;
	int ret, cpu, reenable_timer, j;
	struct cpu_dbs_info_s *dbs_info;

	struct cpumask cpus_timer_done;
	cpumask_clear(&cpus_timer_done);

	ret = sscanf(buf, "%d", &input);

	if (ret != 1)
		return -EINVAL;

	if (input >= POWERSAVE_BIAS_MAXLEVEL) {
		input  = POWERSAVE_BIAS_MAXLEVEL;
		bypass = 1;
	} else if (input <= POWERSAVE_BIAS_MINLEVEL) {
		input  = POWERSAVE_BIAS_MINLEVEL;
		bypass = 1;
	}

	if (input == dbs_tuners_ins.powersave_bias) {
		/* no change */
		return count;
	}

	reenable_timer = ((dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MAXLEVEL) ||
				(dbs_tuners_ins.powersave_bias ==
				POWERSAVE_BIAS_MINLEVEL));

	dbs_tuners_ins.powersave_bias = input; 						// PIETRO : here's it is assigned the value to dbs_tuners_ins.powersave_bias
	if (!bypass) {
		if (reenable_timer) {
			/* reinstate dbs timer */
			for_each_online_cpu(cpu) {
				if (lock_policy_rwsem_write(cpu) < 0)
					continue;

				dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

				for_each_cpu(j, &cpus_timer_done) {
					if (!dbs_info->cur_policy) {
						pr_err("Dbs policy is NULL\n");
						goto skip_this_cpu;
					}
					if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
						goto skip_this_cpu;
				}

				cpumask_set_cpu(cpu, &cpus_timer_done);
				if (dbs_info->cur_policy) {
					/* restart dbs timer */
					dbs_timer_init(dbs_info);
				}
skip_this_cpu:
				unlock_policy_rwsem_write(cpu);
			}
		}
		ondemand_powersave_bias_init();
	} else {
		/* running at maximum or minimum frequencies; cancel
		   dbs timer as periodic load sampling is not necessary */
		for_each_online_cpu(cpu) {
			if (lock_policy_rwsem_write(cpu) < 0)
				continue;

			dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

			for_each_cpu(j, &cpus_timer_done) {
				if (!dbs_info->cur_policy) {
					pr_err("Dbs policy is NULL\n");
					goto skip_this_cpu_bypass;
				}
				if (cpumask_test_cpu(j, dbs_info->
							cur_policy->cpus))
					goto skip_this_cpu_bypass;
			}

			cpumask_set_cpu(cpu, &cpus_timer_done);

			if (dbs_info->cur_policy) {
				/* cpu using ondemand, cancel dbs timer */
				mutex_lock(&dbs_info->timer_mutex);
				dbs_timer_exit(dbs_info);

				ondemand_powersave_bias_setspeed(
					dbs_info->cur_policy,
					NULL,
					input);

				mutex_unlock(&dbs_info->timer_mutex);
			}
skip_this_cpu_bypass:
			unlock_policy_rwsem_write(cpu);
		}
	}

	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(down_differential);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(powersave_bias);
#ifdef PIETRO_SYSFS
define_one_global_rw(sysfs_V_LTC);
#endif

static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&down_differential.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&powersave_bias.attr,
	&io_is_busy.attr,
	#ifdef PIETRO_SYSFS
	&sysfs_V_LTC.attr,
	#endif
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "reliability",
};

/************************** sysfs end ************************/

#if 0
static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
	/*if (dbs_tuners_ins.powersave_bias)
		freq = powersave_bias_target(p, freq, CPUFREQ_RELATION_H);
	else if (p->cur == p->max)
		return;

	__cpufreq_driver_target(p, freq, dbs_tuners_ins.powersave_bias ?
			CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);*/

	//P: 
	//printk(KERN_ALERT "PIETRO ALERT: I'm in dbs_freq_increase with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
	__cpufreq_driver_target(p, freq_test[index_pietro], 
				(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
	index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}
}
#endif



#if 0
static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	/* Extrapolated load of this CPU */
	unsigned int load_at_max_freq = 0;
	unsigned int max_load_freq;
	/* Current load across this CPU */
	unsigned int cur_load = 0;

	struct cpufreq_policy *policy;
	unsigned int j;

	this_dbs_info->freq_lo = 0;
	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int)
			(cur_wall_time - j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int)
			(cur_idle_time - j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int)
			(cur_iowait_time - j_dbs_info->prev_cpu_iowait);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			u64 cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
					 j_dbs_info->prev_cpu_nice;
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		/*
		 * For the purpose of ondemand, waiting for disk IO is an
		 * indication that you're performance critical, and not that
		 * the system is actually idle. So subtract the iowait time
		 * from the cpu idle time.
		 */

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))     // PM ????????  what is unlikely??
			continue;

		cur_load = 100 * (wall_time - idle_time) / wall_time;

		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = cur_load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;

		/*printk(KERN_ALERT "PIETRO ALERT : wall_time= %u idle_time= %u cur_load= %u freq_avg = %u load_freq = %u",
											wall_time, idle_time, cur_load, freq_avg, load_freq);*/

	}
	/* calculate the scaled load across CPU */
	load_at_max_freq = (cur_load * policy->cur)/policy->cpuinfo.max_freq;

	cpufreq_notify_utilization(policy, load_at_max_freq);


	/* Check for frequency increase */						// PIETRO : when the load is above the threshold, increase the frequency
	if (max_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
		/* If switching to max speed, apply sampling_down_factor */
		if (policy->cur < policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;
		//dbs_freq_increase(policy, policy->max);
		return;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		return;

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		unsigned int freq_next;
		freq_next = max_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		/* No longer fully busy, reset rate_mult */
		this_dbs_info->rate_mult = 1;

		if (freq_next < policy->min)
			freq_next = policy->min;

		if (!dbs_tuners_ins.powersave_bias) {
			/*__cpufreq_driver_target(policy, freq_next,
					CPUFREQ_RELATION_L);*/
			//P:
			//printk(KERN_ALERT "PIETRO ALERT: I'm in dbs_check_cpu 1 with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
			/*__cpufreq_driver_target(policy, freq_test[index_pietro],
					(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);//freq_test[index_pietro]
			index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}*/
		} else {
			/*int freq = powersave_bias_target(policy, freq_next,
					CPUFREQ_RELATION_L);
			__cpufreq_driver_target(policy, freq,
				CPUFREQ_RELATION_L);*/
			//P:
			//printk(KERN_ALERT "PIETRO ALERT: I'm in dbs_check_cpu 2 with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
			/*__cpufreq_driver_target(policy, freq_test[index_pietro],
				(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
			index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}*/
		}
	}
}
#endif

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int sample_type = dbs_info->sample_type;
	int delay ;
	int bill; //BILL
	
	#ifdef LTC_SIGNAL
	int ret;
	struct siginfo info;
	struct task_struct *t; 
	#endif

	long unsigned int c_before_change_freq = 0, c_after_change_freq = 0 , c_duration_change_freq = 0;

	#ifdef READ_TEMP
	struct tsens_device tsens ;
	int32_t out;
	long unsigned int temp_func[10];
	int k;
	#endif





	//PIETRO
	#ifdef CONFIG_SCHED_GOV_SYNCH
	int i;
	#endif

	mutex_lock(&dbs_info->timer_mutex);


	
	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (!dbs_tuners_ins.powersave_bias ||
	    sample_type == DBS_NORMAL_SAMPLE) {

		//dbs_check_cpu(dbs_info);    <------ DANGEROUS !!
	
		#ifdef CONFIG_RELIABILITY

		// RELIABILITY MANAGEMENT ALGORITHM - start	


		if ( per_cpu(t_LI,cpu) < per_cpu(delta_LI,cpu) )
			per_cpu(V_STC,cpu) = (  per_cpu(V_LTC,cpu) * per_cpu(delta_LI,cpu) 
								- per_cpu(V_LI,cpu) * per_cpu(t_LI,cpu)  ) 
									/ (per_cpu(delta_LI,cpu) - per_cpu(t_LI,cpu) ) ;
		else 
			per_cpu(V_STC,cpu) = per_cpu(V_STC,cpu);
		


		if ( per_cpu(V_STC,cpu) < V_MIN )
			per_cpu(V_STC,cpu) = V_MIN ; 
		if ( per_cpu(V_STC,cpu) > V_MAX)
			per_cpu(V_STC,cpu) = V_MAX ;

		//BILL DEBUG
		/*printk (KERN_ALERT "BILL DEBUG : H_table_dim = %d\n"
                                                                ,H_table_dim);
		for(bill = 0; bill < H_table_dim; bill++) {
			 printk (KERN_ALERT "BILL DEBUG : H_table[%d] = %lu\n"
                                                                , bill, H_table[bill]);

		}*/
		
		//checking the HL_flag
		if (per_cpu(activate_safe_mode,cpu)!=1){
			if (per_cpu(pid_gov,cpu) == 0) {  //IDLE task
					per_cpu(Vf_APP_index,cpu) = 0;
					per_cpu(HL_flag,cpu) = 0;
			}
			else{

				per_cpu(HL_flag,cpu) = search_in_H_table(per_cpu(pid_gov,cpu));

				//decide based on the table of voltage and frequencies and the HL flag
				if ( per_cpu(HL_flag,cpu) == 1 )
					per_cpu(Vf_APP_index,cpu) = 14; //the last one is 14
				else if ( per_cpu(HL_flag,cpu) == 0 )	
					per_cpu(Vf_APP_index,cpu) = search_in_Vf_table( per_cpu(V_STC,cpu) );
			}
		}
		else{
			per_cpu(HL_flag,cpu) = search_in_H_table(per_cpu(pid_gov,cpu));
			per_cpu(Vf_APP_index,cpu) = 0;
		}
	

		per_cpu(V_APP,cpu) = Vf_table[0][ per_cpu(Vf_APP_index,cpu) ];
		per_cpu(f_APP,cpu) = Vf_table[1][ per_cpu(Vf_APP_index,cpu) ];
	
		// fake performance governor
		//per_cpu(V_APP,cpu) = Vf_table[0][ 14 ];
		//per_cpu(f_APP,cpu) = Vf_table[1][ 14 ];

		//printk(KERN_ALERT "CPU = %u, f_APP = %lu " , cpu , __get_cpu_var(f_APP) );


		// update quantities
	
		if  ( ( per_cpu(t_LI,cpu) + per_cpu(delta_SI,cpu)  ) == 0  )
			printk (KERN_ALERT "PIETRO ALERT : ( per_cpu(t_LI) + per_cpu(delta_SI) == 0 , %lu , %lu" 
								, per_cpu(t_LI,cpu), per_cpu(delta_SI,cpu));
	
		if ( per_cpu(t_LI,cpu) < per_cpu(delta_LI,cpu) )
			per_cpu(V_LI,cpu) = ( per_cpu(V_LI,cpu) * per_cpu(t_LI,cpu) 
							+ per_cpu(V_APP,cpu) * per_cpu(delta_SI,cpu) ) 
								/ ( per_cpu(t_LI,cpu) + per_cpu(delta_SI,cpu) ) ;


		if (per_cpu(f_APP,cpu) > f_MAX)
			per_cpu(f_APP,cpu) = f_MAX ; 
		if (per_cpu(f_APP,cpu) < f_MIN)
			per_cpu(f_APP,cpu) = f_MIN ; 
		
		



		

		//set the new frequency
		__cpufreq_driver_target(dbs_info->cur_policy, per_cpu(f_APP,cpu),
					CPUFREQ_RELATION_L);

		
		
	
		c_duration_change_freq = c_after_change_freq - c_before_change_freq ; 

		if( per_cpu(t_LI,cpu)%100 == 0){
			printk (KERN_ALERT "PIETRO DEBUG EXEC TIME CHANGE FREQUENCY : c_duration_change_freq = %lu in %u\n" , c_duration_change_freq, cpu);
		}


		//per_cpu(t_LI,cpu) = per_cpu(t_LI,cpu) + 1; 


		//(maybe I can remove this)
		//this might solve the problem of "Division by zero in kernel"
		/*if  ( per_cpu(t_LI,cpu)  == 0 ) 
			{
				per_cpu(V_LTC,cpu) = 1199;//000;    //
				per_cpu(V_STC,cpu) = 1200;//000;
				per_cpu(V_LI,cpu) = 1200;//000;

				per_cpu(delta_LI,cpu) = 1000; // jiffies?
				per_cpu(t_LI,cpu) = 1 ;  // ??? 
				per_cpu(delta_SI,cpu) = 1;

			}
		*/
		#endif //CONFIG_RELIABILITY

		//--------------
		


		#ifdef READ_TEMP
		if (per_cpu(t_LI,cpu)%100 == 0){

			
		


			if (cpu == 0){
				for (k = 0; k < 3; k++){
					tsens.sensor_num = k;
					out = tsens_get_temp(&tsens,&temp_func[k]);   // FIX ME!!
					temp_gov[k] = temp_func[k];
				}
			
			}
			else if(cpu == 1){
				for (k = 3 ; k < 6 ; k++){
					tsens.sensor_num = k;
					out = tsens_get_temp(&tsens,&temp_func[k]);   // FIX ME!!
					temp_gov[k] = temp_func[k];
				}
			
			}
			else if(cpu == 2){
				for (k=6;k<8;k++){
					tsens.sensor_num = k;
					out = tsens_get_temp(&tsens,&temp_func[k]);   // FIX ME!!
					temp_gov[k] = temp_func[k];
				}
			
			}
			else{
				for (k=8;k<10;k++){
					tsens.sensor_num = k;
					out = tsens_get_temp(&tsens,&temp_func[k]);   // FIX ME!!
					temp_gov[k] = temp_func[k];
				}
			}
			

		
		}


		#endif




		#ifdef LTC_SIGNAL
		//this part is there to see if I'm able to send a signal from the governor to the LTC	.c program
		if ( ( per_cpu(t_LI,cpu) >= per_cpu(delta_LI,cpu) ) && (per_cpu(enter_signal,cpu) == 0) ){
			per_cpu(enter_signal,cpu) = 1;
			//printk(KERN_ALERT "PIETRO ALERT : I'm inside the signal sending procedure!!!");
			/* send the signal */
			memset(&info, 0, sizeof(struct siginfo));
			info.si_signo = SIGUSR1;
			info.si_code = SI_QUEUE;	// this is bit of a trickery: SI_QUEUE is normally used by sigqueue from user space,
							// and kernel space should use SI_KERNEL. But if SI_KERNEL is used the real_time data 
							// is not delivered to the user space signal handler function. 
			info.si_int = 1234;  		//real time signals may have 32 bits of data.
			rcu_read_lock();
			t = pid_task(find_vpid(pid_p) , PIDTYPE_PID);  //find the task_struct associated with this pid
			if(t == NULL){
				printk("no such pid\n");
				rcu_read_unlock();
				//return -ENODEV;
			}
			rcu_read_unlock();
			ret = send_sig_info(SIGUSR1, &info, t);    //send the signal
			if (ret < 0) {
				printk("error sending signal\n");
				//return ret;
			}		

		}
		if (  per_cpu(t_LI,cpu) < 100 ){
			per_cpu(enter_signal,cpu) = 0;
		}
		

		#endif


		// HOW TO HANDLE THE DELAY???


		if (dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			dbs_info->sample_type = DBS_SUB_SAMPLE;
			delay = dbs_info->freq_hi_jiffies;
		} else {
			/* We want all CPUs to do sampling nearly on
			 * same jiffy
			 */
			delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				* dbs_info->rate_mult);

			if (num_online_cpus() > 1)
				delay -= jiffies % delay;
		}
	} else {
		/*__cpufreq_driver_target(dbs_info->cur_policy,
			dbs_info->freq_lo, CPUFREQ_RELATION_H);*/
		delay = dbs_info->freq_lo_jiffies;
		//printk(KERN_ALERT "PIETRO ALERT: I'm in do_dbs_timer ELSE with delay = %u", delay);
		#if 0
		//P:
		printk(KERN_ALERT "PIETRO ALERT: I'm in do_dbs_timer with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
		__cpufreq_driver_target(dbs_info->cur_policy, freq_test[index_pietro],
						CPUFREQ_RELATION_L);
		index_pietro=index_pietro+1;
		if (index_pietro==10){
			index_pietro=0;
		}
		#endif
	}



	//per_cpu(t_LI,cpu) = per_cpu(t_LI,cpu) + 1; 
	if ( per_cpu(jiffies_prec , cpu) == 0){
		per_cpu(jiffies_prec , cpu) = jiffies - 1 ;
	}


	per_cpu(t_LI,cpu) = per_cpu(t_LI,cpu) + (  jiffies - per_cpu(jiffies_prec , cpu) );
	per_cpu(jiffies_prec , cpu) = jiffies;



	

	schedule_delayed_work_on(cpu, &dbs_info->work, delay); 
	mutex_unlock(&dbs_info->timer_mutex);
}








static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	
	printk (KERN_ALERT "BILL DEBUG : dbs_timer_init for cpu %u, in total %d\n" , dbs_info->cpu, num_online_cpus());
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	INIT_DELAYED_WORK(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

/*
 * Not all CPUs want IO time to be accounted as busy; this dependson how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (androidlcom) calis this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) andl later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
	    boot_cpu_data.x86 == 6 &&
	    boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}

static void dbs_refresh_callback(struct work_struct *unused)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int cpu = smp_processor_id();

	//printk (KERN_ALERT "BILL DEBUG : In dbs_refresh_callback \n" );

	
	get_online_cpus();

	if (lock_policy_rwsem_write(cpu) < 0)
		goto bail_acq_sema_failed;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	policy = this_dbs_info->cur_policy;
	if (!policy) {
		/* CPU not using ondemand governor */
		goto bail_incorrect_governor;
	}

	if (policy->cur < policy->max) {
		policy->cur = policy->max;

		/*__cpufreq_driver_target(policy, policy->max,
					CPUFREQ_RELATION_L);*/
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&this_dbs_info->prev_cpu_wall);

		//P:
		#if 0
		//printk(KERN_ALERT "PIETRO ALERT: I'm in dbs_refresh_callback with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
		__cpufreq_driver_target(policy, freq_test[index_pietro],
			(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
		index_pietro=index_pietro+1;
		if (index_pietro==10){
			index_pietro=0;
		}
		#endif
	}

bail_incorrect_governor:
	unlock_policy_rwsem_write(cpu);

bail_acq_sema_failed:
	put_online_cpus();
	return;
}

static void dbs_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	int i;
	//printk (KERN_ALERT "BILL DEBUG : In dbs_input_event \n" );

	if ((dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MAXLEVEL) ||
		(dbs_tuners_ins.powersave_bias == POWERSAVE_BIAS_MINLEVEL)) {
		/* nothing to do */
		return;
	}

	for_each_online_cpu(i) {
		queue_work_on(i, input_wq, &per_cpu(dbs_refresh_work, i));
	}
}

static int dbs_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;
	
	printk (KERN_ALERT "BILL DEBUG : In dbs_input_connect \n" );

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	printk (KERN_ALERT "BILL DEBUG : In dbs_input_disconnect \n" );

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler dbs_input_handler = {
	.event		= dbs_input_event,
	.connect	= dbs_input_connect,
	.disconnect	= dbs_input_disconnect,
	.name		= "cpufreq_rel",
	.id_table	= dbs_ids,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;
	
	//int p;		
	printk (KERN_ALERT "BILL DEBUG : In cpufreq_governor_dbs for cpu = %u \n", cpu );

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		

		#ifdef CONFIG_RELIABILITY

		// PIETRO
		//initialization of variables for reliability algorithm
                for_each_cpu(j, policy->cpus) {

			per_cpu(V_LTC,j) = 950;//000;    //
			per_cpu(V_STC,j) = 950;//000;
			per_cpu(V_LI,j) = 950;//000;
			per_cpu(delta_LI,j) = 500; //1000; // jiffies?
			per_cpu(t_LI,j) = 0 ;  // ??? 
			per_cpu(delta_SI,j) = 1;
			per_cpu(jiffies_prec,j) = 0;
			per_cpu(activate_safe_mode,j) = 0;
			#ifdef MONITOR_ON
			//activation of reliability stats reading
			per_cpu(reliability_gov_ready,j) = 1; //this tells the scheduler that the governor is in
			#endif //MONITOR_ON
		}

		//debug
		printk(KERN_ALERT "PIETRO ALERT : CPUFREQ_GOV_START ,  CPU %u ,  monitor_stats_start = %u , reliability_gov_ready = %u",
								cpu ,  per_cpu(monitor_stats_start,cpu), per_cpu(reliability_gov_ready,cpu) );



		#endif //CONFIG_RELIABILITY


		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice)
				j_dbs_info->prev_cpu_nice =
						kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		}
		this_dbs_info->cpu = cpu;
		this_dbs_info->rate_mult = 1;
		ondemand_powersave_bias_init_cpu(cpu);
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			dbs_tuners_ins.io_is_busy = should_io_be_busy();
		}
		if (!cpu)
			rc = input_register_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);


		if (!ondemand_powersave_bias_setspeed(
					this_dbs_info->cur_policy,
					NULL,
					dbs_tuners_ins.powersave_bias))
			dbs_timer_init(this_dbs_info);
		break;



	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);
		


		
		#ifdef MONITOR_ON
		for_each_cpu(j, policy->cpus) {
			//deactivation of reliability stats reading
			per_cpu(reliability_gov_ready,j) = 0; // this tells the scheduler that the governor is off
		}
		printk(KERN_ALERT "PIETRO ALERT : CPUFREQ_GOV_STOP CPU %u , H_table_dim = %u , reliability_gov_ready = %u" , cpu, H_table_dim , per_cpu(reliability_gov_ready,cpu)  );

		#endif //MONITOR_ON
		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		/* If device is being removed, policy is no longer
		 * valid. */
		this_dbs_info->cur_policy = NULL;
		if (!cpu)
			input_unregister_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			printk(KERN_ALERT "PIETRO ALERT: I'm in CPUFREQ_GOV_LIMITS 1");
			
			/*__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);*/
			//P:
			//printk(KERN_ALERT "PIETRO ALERT: I'm in CPUFREQ_GOV_LIMITS 1 with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
			/*__cpufreq_driver_target(this_dbs_info->cur_policy, freq_test[index_pietro],
									CPUFREQ_RELATION_H);
			index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}*/			
			

		else if (policy->min > this_dbs_info->cur_policy->cur)
			printk(KERN_ALERT "PIETRO ALERT: I'm in CPUFREQ_GOV_LIMITS 2");
			#if 0
			/*__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);*/
			//P:
			//printk(KERN_ALERT "PIETRO ALERT: I'm in CPUFREQ_GOV_LIMITS 2 with index_pietro = %u and freq = %u", index_pietro, freq_test[index_pietro]);
			__cpufreq_driver_target(this_dbs_info->cur_policy, freq_test[index_pietro],
				(index_pietro<5)?  CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
			index_pietro=index_pietro+1;
			if (index_pietro==10){
				index_pietro=0;
			}
			#endif

		else if (dbs_tuners_ins.powersave_bias != 0)
			ondemand_powersave_bias_setspeed(
				this_dbs_info->cur_policy,
				policy,                            // PM that's weird.....
				dbs_tuners_ins.powersave_bias);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	unsigned int i;
	int cpu = get_cpu();
	
	printk (KERN_ALERT "BILL DEBUG : In cpufreq_gov_dbs_init for cpu = %u \n", cpu );

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	
	

	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		dbs_tuners_ins.down_differential =
					MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */                 //<------------------ BE CAREFUL !!! SAMPLING RATE!!!
		min_sampling_rate =
			jiffies_to_usecs(1);
			//MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

	input_wq = create_workqueue("iewq");
	if (!input_wq) {
		printk(KERN_ERR "Failed to create iewq workqueue\n");
		return -EFAULT;
	}
	for_each_possible_cpu(i) {
		struct cpu_dbs_info_s *this_dbs_info =
			&per_cpu(od_cpu_dbs_info, i);
		mutex_init(&this_dbs_info->timer_mutex);
		INIT_WORK(&per_cpu(dbs_refresh_work, i), dbs_refresh_callback);
	}

	return cpufreq_register_governor(&cpufreq_gov_reliability);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_reliability);
	destroy_workqueue(input_wq);
}

//-----------------------------------------------------------
#ifdef CONFIG_RELIABILITY

static unsigned int search_in_Vf_table(unsigned int V_STC){
	unsigned int diff = 10000000;
	unsigned int diff_prev = 10000000;
	unsigned int i;


	for (i=0; i<15; i++){
		if (Vf_table[0][i] > V_STC)
			diff = Vf_table[0][i] - V_STC;
		else diff = V_STC - Vf_table[0][i] ;
		
		if (diff < diff_prev){
			diff_prev = diff;
			Vf_APP_index = i;
		}
		 
	}
	
return Vf_APP_index;	 
}

static long unsigned int search_in_H_table(int pid){
	long unsigned int HL_flag = 0;
	int i=0;
	for (i=0; i<H_table_dim ; i++){
		if (pid == H_table[i]){
			HL_flag = 1;
		}
	}
	return HL_flag;
}

#endif //CONFIG_RELIABILITY


//-----------------------------------------------------------


MODULE_AUTHOR("Pietro Mercati <pietromercati@gmail.com.com>");
MODULE_DESCRIPTION("'cpufreq_reliability' - A dynamic cpufreq governor for "
	"high reliability processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_RELIABILITY
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
