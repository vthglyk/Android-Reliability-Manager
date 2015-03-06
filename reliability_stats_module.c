

/* -------   relibility stats module ----------------------------

this module 



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

//#include <linux/HL_table.h> //for H_table

#define REL_MAJOR 32
#define REL_NAME "rel"

/*
*Used in the iocontrol case statement
*/
#define PASS_H_DIM	9
#define SELECT_CPU	4
#define READY		1
#define S_READY		5
#define UPDATE_V_LTC	6
#define GET_DELTA_LI	7
#define GET_V_LI	8


/*
*	Defines the dimention of the buffer to allocate for collecting the REL statistics. 	
*/
#define RELIABILITY_EXPORT_PAGE 6	//Number of pages allocated for each buffer
#define RELIABILITY_EXPORT_LENGTH 1024	//Number of enries of kind "reliability_stats_data" inside the allocated buffer

DECLARE_PER_CPU(struct reliability_stats_data *, reliability_stats_data);  // address of the buffer #1: to be declared in the reliability_stats_module as well
DECLARE_PER_CPU(struct reliability_stats_data *, reliability_stats_data2);  // address of the buffer #2: to be declared in the reliability_stats_module as well
DECLARE_PER_CPU(int , reliability_stats_index); //index inside of the stats buffer
DECLARE_PER_CPU(int , reliability_stats_start); //incremented each time a buffer is filled. useful in conditions.


DECLARE_PER_CPU(unsigned long int, V_LTC);
DECLARE_PER_CPU(unsigned long int, delta_LI);
DECLARE_PER_CPU(unsigned long int, V_LI);



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

static int selected_cpu = 0;

extern int H_table_dim;
extern long unsigned int H_table[1];

/* ---- Private Constants and Types -------------------------------------- */
static char     relBanner[] __initdata = KERN_INFO "User Mode RELIABILITY STATS MODULE Driver: 1.00\n";

/* ---- Private Variables ------------------------------------------------ */
static  dev_t           relDrvDevNum = 0;   //P: actual device number
static  struct class   *relDrvClass = NULL; 
static  struct  cdev    relDrvCDev;  //P: kernel's internal structure that represent char devices

//------------------------------------------------------------------------------------------------------


/* rel_mod_init - MODULE INIT - Allocate the two buffer to contains the statistics (performance counters) sampled by the kernel
*		for the current cpu in execution. Set the index inside these array at the end of them and start the sampling 
*		for the current cpu.
*/
long rel_mod_init(void){
	__get_cpu_var(reliability_stats_data) = (struct reliability_stats_data *)__get_free_pages(GFP_KERNEL,RELIABILITY_EXPORT_PAGE);
	__get_cpu_var(reliability_stats_data2) = (struct reliability_stats_data *)__get_free_pages(GFP_KERNEL,RELIABILITY_EXPORT_PAGE);
	__get_cpu_var(reliability_stats_index) = RELIABILITY_EXPORT_LENGTH-1;
	__get_cpu_var(reliability_stats_start) = 1;
	printk(KERN_ALERT "PIETRO ALERT MODULE reliability mod init CPU%d -start = %d - idx = %d \n", 
				smp_processor_id(),__get_cpu_var(reliability_stats_start),__get_cpu_var(reliability_stats_index));
	return 0;
}

/* rel_mod_exit - MODULE EXIT - Deallocate the bufffer and stop the kernel performance counters sampling
*/
long rel_mod_exit(void){
	printk(KERN_ALERT "PIETRO ALERT MODULE reliability mod exit CPU%d index %d \n", smp_processor_id() , __get_cpu_var(reliability_stats_index));
	__get_cpu_var(reliability_stats_index) = 0;
	__get_cpu_var(reliability_stats_start) = 0;
	__free_pages((struct page *)__get_cpu_var(reliability_stats_data), RELIABILITY_EXPORT_PAGE);
	__free_pages((struct page *)__get_cpu_var(reliability_stats_data2), RELIABILITY_EXPORT_PAGE);

	return 0;
}

/* rel_int_ready - Copy to user space the index inside the statistic export buffer
*	@_ext_buf : is the pointer to the userspace variable where to save the current entry processed in the export buffer
*/
long rel_int_ready(void* _ext_buf){
	int * ext_buf = _ext_buf;
	if (copy_to_user(ext_buf,&__get_cpu_var(reliability_stats_index), sizeof(int)))
		return -EFAULT;
	return 0;
}

/* open function - called when the "file" /dev/rel is opened in userspace
*	execute the rel_mod_init for each cpu on-line in the systems
*/
static int rel_open (struct inode *inode, struct file *file) {
	unsigned int i;
	printk(KERN_ALERT "rel_open\n");
	// Init the REL kernel process on a per cpu bases!
	for_each_online_cpu(i) {
		work_on_cpu(i, (void*)rel_mod_init, NULL);
	}
	
	return 0;
}

/* close function - called when the "file" /dev/rel is closed in userspace  
*	execute the rel_mod_exit for each cpu on-line in the systems
*/
static int rel_release (struct inode *inode, struct file *file) {
	int i = 0;
	printk(KERN_ALERT "rel_release\n");
	for_each_online_cpu(i){
		work_on_cpu(i, (void*)rel_mod_exit, NULL);
	}
	return 0;
}

//copy full buffer to userspace
static ssize_t rel_stats_read (struct file *file, char *buf,
		size_t count, loff_t *ppos) {
	int err;
        err = copy_to_user((char *)buf,(char *)(per_cpu(reliability_stats_data2, selected_cpu)),(sizeof(struct reliability_stats_data)*RELIABILITY_EXPORT_LENGTH));


	if (err != 0){
		printk(KERN_ALERT "rel_read - error");
		return -EFAULT;
	}
	return 1;
}

//copy the table of pid of H applications from userspace
static ssize_t rel_H_table_write (struct file *file, const char *buf,
		size_t count, loff_t *ppos) {
	int err, k;
	
        err = copy_from_user( H_table  , buf , sizeof(long unsigned int)*H_table_dim  ) ;

	for (k = 0 ; k< H_table_dim ; k++){
		printk(KERN_ALERT "PIETRO DEBUG : H_table element = %lu\n", H_table[k]);
	}
	printk(KERN_ALERT "PIETRO DEBUG : H_table_dim = %u\n", H_table_dim);
	
	if (err != 0){
		printk(KERN_ALERT "rel_read - error");
		return -EFAULT;
	}
	return 1;
}


static long rel_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg) {
	long retval = 0;

	switch ( cmd ) {
		case SELECT_CPU:
			if ( copy_from_user( &selected_cpu, (int *)arg, sizeof(selected_cpu) )) {
							
				return -EFAULT;
			}
			break;
		case PASS_H_DIM:
			//printk (KERN_ALERT "PIETRO DEBUG : ioctl PASS - ENTER");
			if ( copy_from_user( &H_table_dim, (int *)arg, sizeof(int) )) {
				printk (KERN_ALERT "PIETRO DEBUG : ioctl PASS - Pass table error")  ;			
				return -EFAULT;
			}

			printk(KERN_ALERT "PIETRO DEBUG : ioctl - H_table_dim = %d", H_table_dim);
			break;
		case READY:		
			if (copy_to_user((int *)arg, &__get_cpu_var(reliability_stats_start), sizeof(int)))
				return -EFAULT;
			break;
	
		case S_READY:		
			if (copy_to_user((int *)arg, &per_cpu(reliability_stats_start,selected_cpu), sizeof(int))){	 
				return -EFAULT;
			printk (KERN_ALERT "PIETRO DEBUG : &per_cpu(reliability_stats_start,selected_cpu) = %u" , per_cpu(reliability_stats_start,selected_cpu));
			}
			break;
			
		case UPDATE_V_LTC:
			if ( copy_from_user( &per_cpu(V_LTC,selected_cpu), (int *)arg, sizeof(H_table_dim) )) {
				printk (KERN_ALERT "PIETRO DEBUG : ioctl UPDATE_V_LTC - Pass error")  ;			
				return -EFAULT;
			}			
			break;

		case GET_DELTA_LI:		
			if (copy_to_user((long unsigned int *)arg, &per_cpu(delta_LI,selected_cpu), sizeof(long unsigned int))){
				printk (KERN_ALERT "PIETRO DEBUG : ioctl GET_DELTA_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		case GET_V_LI:		
			if (copy_to_user((long unsigned int *)arg, &per_cpu(V_LI,selected_cpu), sizeof(long unsigned int))){
				printk (KERN_ALERT "PIETRO DEBUG : ioctl GET_V_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;


		default:
			printk(KERN_ALERT "DEBUG: rel_ioctrl - You should not be here!!!!\n");
			retval = -EINVAL;
	}

	return retval;

}




/*
READY : this one has to be used as a condition by the userspace program
in particular, when the value of reliability_stats_start changes, it means
that 

*/

// define which file operations are supported
struct file_operations rel_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	rel_stats_read,
	.write		=	rel_H_table_write,
	.readdir	=	NULL,
	.poll		=	NULL,
	.unlocked_ioctl	=	rel_ioctl,
	.mmap		=	NULL,
	.open		=	rel_open,
	.flush		=	NULL,
	.release	=	rel_release,
	.fsync		=	NULL,
	.fasync		=	NULL,
	.lock		=	NULL,
	//.readv		=	NULL,
	//.writev		=	NULL,
};

/*
// initialize module 
static int __init rel_init_module (void) {
	int i;
	printk(KERN_ALERT "initializing module\n");
	i = register_chrdev (REL_MAJOR, REL_NAME, &rel_fops);
	if (i != 0){
		return - EIO;
		printk(KERN_ALERT "PIETRO ALERT error mounting reliability module")	;
	}
	return 0;
}

// close and cleanup module
static void __exit rel_cleanup_module (void) {
	printk(KERN_ALERT "cleaning up module\n");
	unregister_chrdev (REL_MAJOR, REL_NAME);
}

*/




//module initialization
static int __init rel_init_module (void) {
    int     rc = 0;

    printk( relBanner );

    if ( MAJOR( relDrvDevNum ) == 0 )
    {
        /* Allocate a major number dynamically */
        if (( rc = alloc_chrdev_region( &relDrvDevNum, 0, 1, REL_NAME )) < 0 )
        {
            printk( KERN_WARNING "%s: alloc_chrdev_region failed; err: %d\n", __func__, rc );
            return rc;
        }
    }
    else
    {
        /* Use the statically assigned major number */
        if (( rc = register_chrdev_region( relDrvDevNum, 1, REL_NAME )) < 0 )  //P: returns 0 if ok, negative if error
        {
           printk( KERN_WARNING "%s: register_chrdev failed; err: %d\n", __func__, rc );
           return rc;
        }
    }

    cdev_init( &relDrvCDev, &rel_fops );
    relDrvCDev.owner = THIS_MODULE;

    if (( rc = cdev_add( &relDrvCDev, relDrvDevNum, 1 )) != 0 )
    {
        printk( KERN_WARNING "%s: cdev_add failed: %d\n", __func__, rc );
        goto out_unregister;
    }

    /* Now that we've added the device, create a class, so that udev will make the /dev entry */

    relDrvClass = class_create( THIS_MODULE, REL_NAME );
    if ( IS_ERR( relDrvClass ))
    {
        printk( KERN_WARNING "%s: Unable to create class\n", __func__ );
        rc = -1;
        goto out_cdev_del;
    }

    device_create( relDrvClass, NULL, relDrvDevNum, NULL, REL_NAME );

    goto done;

out_cdev_del:
    cdev_del( &relDrvCDev );

out_unregister:
    unregister_chrdev_region( relDrvDevNum, 1 );

done:
    return rc;
}

//module exit function
static void __exit rel_cleanup_module (void) {
        printk(KERN_ALERT "cleaning up module\n");
	
	device_destroy( relDrvClass, relDrvDevNum );
	class_destroy( relDrvClass );
	cdev_del( &relDrvCDev );

	unregister_chrdev_region( relDrvDevNum, 1 );

}




module_init(rel_init_module);
module_exit(rel_cleanup_module);
MODULE_AUTHOR(" Pietro Mercati and Andrea Bartolini" );
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read the internal Snapdragon statistics for reliability management");









