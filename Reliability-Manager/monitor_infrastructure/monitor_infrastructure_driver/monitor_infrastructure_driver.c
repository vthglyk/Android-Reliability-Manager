

/* -------   MONITOR MODULE FOR ANDROID/LINUX   ----------------------------
 * 
 * Author: Andrea Bartolini
 *
 * Modified by : Pietro Mercati
 * email : pimercat@eng.ucsd.edu
 * 
 * If using this code for research purposes, include 
 * references to the following publications
 * 
 * 1) P.Mercati, A. Bartolini, F. Paterna, T. Rosing and L. Benini; A Linux-governor based 
 *    Dynamic Reliability Manager for android mobile devices. DATE 2014.
 * 2) P.Mercati, A. Bartolini, F. Paterna, L. Benini and T. Rosing; An On-line Reliability 
 *    Emulation Framework. EUC 2014
 * 
	This file is part of DRM.
        DRM is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        DRM is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with DRM.  If not, see <http://www.gnu.org/licenses/>.
*/





#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/cpumask.h>

#include <linux/cdev.h>
#include <linux/device.h>  //for class_create

#define MONITOR_MAJOR 32
#define MONITOR_NAME "monitor"

#define SELECT_CPU 	1
#define READY 		2
#define S_READY 	3
#define PASS_H_DIM	4

#define EXYNOS_TMU_COUNT 5 // this has to be the same as in exynos_thermal.c and core.c
/*
*	Defines the dimension of the buffer to allocate for collecting the MONITOR statistics. 	
*/
#define MONITOR_EXPORT_PAGE 6	//Number of pages allocated for each buffer
#define MONITOR_EXPORT_LENGTH 1024	//Number of enries of kind "monitor_stats_data" inside the allocated buffer

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
};

// to be defined in the kernel source code ( e.g. in core.c )
DECLARE_PER_CPU(struct monitor_stats_data *, monitor_stats_data);  // address of the buffer #1: to be declared in the monitor_stats_module as well
DECLARE_PER_CPU(struct monitor_stats_data *, monitor_stats_data2);  // address of the buffer #2: to be declared in the monitor_stats_module as well
DECLARE_PER_CPU(int , monitor_stats_index); //index inside of the stats buffer
DECLARE_PER_CPU(int , monitor_stats_start); //incremented each time a buffer is filled. useful in conditions.

static int selected_cpu = 0;
extern int H_table_dim;
extern long unsigned int *H_table;

/* ---- Private Constants and Types -------------------------------------- */
static char monitorBanner[] __initdata = KERN_INFO "User Mode MONITOR MODULE Driver: 1.00\n";

/* ---- Private Variables ------------------------------------------------ */
static  dev_t           monitorDrvDevNum = 0;   //P: actual device number
static  struct class   *monitorDrvClass = NULL; 
static  struct  cdev    monitorDrvCDev;  //P: kernel's internal structure that represent char devices

//------------------------------------------------------------------------------------------------------


/* monitor_mod_init - MODULE INIT - Allocate the two buffer to contains the statistics (performance counters) sampled by the kernel
*		for the current cpu in execution. Set the index inside these array at the end of them and start the sampling 
*		for the current cpu.
*/
long monitor_mod_init(void){
	__get_cpu_var(monitor_stats_data) = (struct monitor_stats_data *)__get_free_pages(GFP_KERNEL,MONITOR_EXPORT_PAGE);
	__get_cpu_var(monitor_stats_data2) = (struct monitor_stats_data *)__get_free_pages(GFP_KERNEL,MONITOR_EXPORT_PAGE);
	__get_cpu_var(monitor_stats_index) = MONITOR_EXPORT_LENGTH-1;
	__get_cpu_var(monitor_stats_start) = 1;
	printk(KERN_ALERT "PIETRO ALERT MODULE monitor mod init CPU%d -start = %d - idx = %d \n", 
				smp_processor_id(),__get_cpu_var(monitor_stats_start),__get_cpu_var(monitor_stats_index));
	return 0;
}

/* monitor_mod_exit - MODULE EXIT - Deallocate the bufffer and stop the kernel performance counters sampling
*/
long monitor_mod_exit(void){
	printk(KERN_ALERT "PIETRO ALERT MODULE monitor mod exit CPU%d index %d \n", smp_processor_id() , __get_cpu_var(monitor_stats_index));
	__get_cpu_var(monitor_stats_index) = 0;
	__get_cpu_var(monitor_stats_start) = 0;
	__free_pages((struct page *)__get_cpu_var(monitor_stats_data), MONITOR_EXPORT_PAGE);
	__free_pages((struct page *)__get_cpu_var(monitor_stats_data2), MONITOR_EXPORT_PAGE);

	return 0;
}

/* monitor_int_ready - Copy to user space the index inside the statistic export buffer
*	@_ext_buf : is the pointer to the userspace variable where to save the current entry processed in the export buffer
*/
long monitor_int_ready(void* _ext_buf){
	int * ext_buf = _ext_buf;
	if (copy_to_user(ext_buf,&__get_cpu_var(monitor_stats_index), sizeof(int)))
		return -EFAULT;
	return 0;
}

/* open function - called when the "file" /dev/monitor is opened in userspace
*	execute the monitor_mod_init for each cpu on-line in the systems
*/
static int monitor_open (struct inode *inode, struct file *file) {
	unsigned int i;
	printk(KERN_ALERT "monitor_open\n");
	// Init the MONITOR kernel process on a per cpu bases!
	for_each_online_cpu(i) {
		work_on_cpu(i, (void*)monitor_mod_init, NULL);
	}
	
	return 0;
}

/* close function - called when the "file" /dev/monitor is closed in userspace  
*	execute the monitor_mod_exit for each cpu on-line in the systems
*/
static int monitor_release (struct inode *inode, struct file *file) {
	int i = 0;
	printk(KERN_ALERT "monitor_release\n");
	for_each_online_cpu(i){
		work_on_cpu(i, (void*)monitor_mod_exit, NULL);
	}
	return 0;
}

//copy full buffer to userspace
static ssize_t monitor_stats_read (struct file *file, char *buf,
		size_t count, loff_t *ppos) {
	int err;
        err = copy_to_user((char *)buf,(char *)(per_cpu(monitor_stats_data2, selected_cpu)),(sizeof(struct monitor_stats_data)*MONITOR_EXPORT_LENGTH));


	if (err != 0){
		printk(KERN_ALERT "monitor_read - error");
		return -EFAULT;
	}
	return 1;
}


//copy the table of pid of H applications from userspace
static ssize_t rel_H_table_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos) {
	int err, k;

        printk(KERN_ALERT "Userspace Module : H_table_dim1 = %d\n", H_table_dim);

	//BILL: free and then allocate space for H_table
	kfree(H_table);
	H_table = kmalloc(sizeof(long unsigned int)*H_table_dim, GFP_KERNEL);
	if (!H_table) {
                printk(KERN_ALERT "monitor_write - kmalloc error");
                return -EFAULT;
	}

        err = copy_from_user( H_table  , buf , sizeof(long unsigned int)*H_table_dim  ) ;

	for (k = 0 ; k < H_table_dim ; k++){
		printk(KERN_ALERT "Userspace Module : H_table element = %lu, k = %d\n", H_table[k], k);
	}

	if (err != 0){
		printk(KERN_ALERT "monitor_write - error");
		return -EFAULT;
	}
	return 1;
}


static long monitor_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg) {
	long retval = 0;

	switch ( cmd ) {
		/* Pietro : the value of seleceted_cpu should be passed from userspace and determined the location
			of all variables which are defined per cpu */
		case SELECT_CPU:
			if ( copy_from_user( &selected_cpu, (int *)arg, sizeof(selected_cpu) )) {
							
				return -EFAULT;
			}
			break;

		case READY:		
			if (copy_to_user((int *)arg, &__get_cpu_var(monitor_stats_start), sizeof(int)))
				return -EFAULT;
			break;
	
		case S_READY:		
			if (copy_to_user((int *)arg, &per_cpu(monitor_stats_start,selected_cpu), sizeof(int))){	 
				return -EFAULT;
			printk (KERN_ALERT "PIETRO DEBUG : &per_cpu(monitor_stats_start,selected_cpu) = %u" , per_cpu(monitor_stats_start,selected_cpu));
			}
			break;
		case PASS_H_DIM:
			printk (KERN_ALERT "PIETRO DEBUG : ioctl PASS - ENTER\n");
			if ( copy_from_user( &H_table_dim, (int *)arg, sizeof(int) )) {
				printk (KERN_ALERT "PIETRO DEBUG : ioctl PASS - Pass table error")  ;			
				retval =  -EFAULT;
			}
			break;
		default:
			printk(KERN_ALERT "DEBUG: monitor_ioctrl - You should not be here!!!!\n");
			retval = -EINVAL;
	}

	return retval;

}


/*
READY : this one has to be used as a condition by the userspace program
in particular, when the value of monitor_stats_start changes, it means
that 

*/

// define which file operations are supported
struct file_operations monitor_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	monitor_stats_read,
	.write		=	rel_H_table_write,
	.readdir	=	NULL,
	.poll		=	NULL,
	.unlocked_ioctl	=	monitor_ioctl,
	.mmap		=	NULL,
	.open		=	monitor_open,
	.flush		=	NULL,
	.release	=	monitor_release,
	.fsync		=	NULL,
	.fasync		=	NULL,
	.lock		=	NULL,
	//.readv		=	NULL,
	//.writev		=	NULL,
};


//module initialization
static int __init monitor_init_module (void) {
    int     rc = 0;

    printk( monitorBanner );

    if ( MAJOR( monitorDrvDevNum ) == 0 )
    {
        /* Allocate a major number dynamically */
        if (( rc = alloc_chrdev_region( &monitorDrvDevNum, 0, 1, MONITOR_NAME )) < 0 )
        {
            printk( KERN_WARNING "%s: alloc_chrdev_region failed; err: %d\n", __func__, rc );
            return rc;
        }
    }
    else
    {
        /* Use the statically assigned major number */
        if (( rc = register_chrdev_region( monitorDrvDevNum, 1, MONITOR_NAME )) < 0 )  //P: returns 0 if ok, negative if error
        {
           printk( KERN_WARNING "%s: register_chrdev failed; err: %d\n", __func__, rc );
           return rc;
        }
    }

    cdev_init( &monitorDrvCDev, &monitor_fops );
    monitorDrvCDev.owner = THIS_MODULE;

    if (( rc = cdev_add( &monitorDrvCDev, monitorDrvDevNum, 1 )) != 0 )
    {
        printk( KERN_WARNING "%s: cdev_add failed: %d\n", __func__, rc );
        goto out_unregister;
    }

    /* Now that we've added the device, create a class, so that udev will make the /dev entry */

    monitorDrvClass = class_create( THIS_MODULE, MONITOR_NAME );
    if ( IS_ERR( monitorDrvClass ))
    {
        printk( KERN_WARNING "%s: Unable to create class\n", __func__ );
        rc = -1;
        goto out_cdev_del;
    }

    device_create( monitorDrvClass, NULL, monitorDrvDevNum, NULL, MONITOR_NAME );

    goto done;

out_cdev_del:
    cdev_del( &monitorDrvCDev );

out_unregister:
    unregister_chrdev_region( monitorDrvDevNum, 1 );

done:
    return rc;
}

//module exit function
static void __exit monitor_cleanup_module (void) {
        printk(KERN_ALERT "cleaning up module\n");
	//BILL: clean H_table
	H_table_dim = 0;
	kfree(H_table);
	device_destroy( monitorDrvClass, monitorDrvDevNum );
	class_destroy( monitorDrvClass );
	cdev_del( &monitorDrvCDev );

	unregister_chrdev_region( monitorDrvDevNum, 1 );

}




module_init(monitor_init_module);
module_exit(monitor_cleanup_module);
MODULE_AUTHOR(" Pietro Mercati and Andrea Bartolini" );
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read the internal Snapdragon statistics for monitor management and pass the high priority pids from the AppManager");









