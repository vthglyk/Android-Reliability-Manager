

/* -------   LTC interface module ----------------------------

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

#include <asm/siginfo.h>	//siginfo
#include <linux/kernel.h>
#include <linux/rcupdate.h>	//rcu_read_lock
#include <linux/sched.h>	//find_task_by_pid_type
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/jiffies.h>
//#include <linux/HL_table.h> //for H_table

#define SELECT_CPU	1
#define SET_V_LTC	9
#define GET_DELTA_LI	3
#define GET_V_LI	4
#define SET_V_LI	5
#define GET_t_LI	6
#define SET_t_LI	7
#define SET_V_STC	8
#define GET_JIFFIES	10
#define PASS_PID	11
#define ACTIVATE_SAFE_MODE	12
#define GET_T_LI	13

#define LTC_MAJOR 33
#define LTC_NAME "ltc"


DECLARE_PER_CPU(unsigned long int, t_LI);
DECLARE_PER_CPU(unsigned long int, V_LI);
DECLARE_PER_CPU(unsigned long int, delta_LI);
DECLARE_PER_CPU(unsigned long int, V_LTC);
DECLARE_PER_CPU(unsigned long int, V_STC);


extern pid_t pid_p;

extern unsigned long int T_LI ; 

DECLARE_PER_CPU(unsigned int , activate_safe_mode);

//extern struct task_struct *find_task_by_vpid(pid_t pid);


static int selected_cpu = 0;


/* ---- Private Constants and Types -------------------------------------- */
static char     ltcBanner[] __initdata = KERN_INFO "User Mode LTC INTERFACE MODULE Driver: 1.00\n";

/* ---- Private Variables ------------------------------------------------ */
static  dev_t           ltcDrvDevNum = 0;   //P: actual device number
static  struct class   *ltcDrvClass = NULL; 
static  struct  cdev    ltcDrvCDev;  //P: kernel's internal structure that represent char devices

//------------------------------------------------------------------------------------------------------


static int ltc_open (struct inode *inode, struct file *file) {
	printk(KERN_ALERT "ltc_open\n");
	
	return 0;
}


static int ltc_release (struct inode *inode, struct file *file) {
	printk(KERN_ALERT "ltc_release\n");
	
	return 0;
}


static long ltc_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg) {
	long retval = 0;
	long unsigned int j = 0;
	unsigned int cpu_activate_safe_mode = 0;
	

	/*
	int ret;
	struct siginfo info;
	struct task_struct *t; 
	*/

	switch ( cmd ) {

		case SELECT_CPU:
			if ( copy_from_user( &selected_cpu, (int *)arg, sizeof(selected_cpu) )) {
				printk (KERN_ALERT "PIETRO LTC MODULE : SELECT_CPU fail..." )  ;			
				return -EFAULT;
			}
			break;
		
		case SET_V_LTC:
			if ( copy_from_user( &per_cpu(V_LTC,selected_cpu), (int *)arg, sizeof(unsigned long int) )) {
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl SET_V_LTC - Pass error")  ;			
				return -EFAULT;
			}	
			printk (KERN_ALERT "PIETRO LTC MODULE %u: ioctl SET_V_LTC = %lu" ,selected_cpu, per_cpu(V_LTC,selected_cpu))  ;		
			break;

		case GET_DELTA_LI:		
			if (copy_to_user((long unsigned int *)arg, &per_cpu(delta_LI,selected_cpu), sizeof(long unsigned int))){
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl GET_DELTA_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		case GET_V_LI:		
			if (copy_to_user((long unsigned int *)arg, &per_cpu(V_LI,selected_cpu), sizeof(unsigned long int)) ){
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl GET_V_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		case SET_V_LI:
			if ( copy_from_user( &per_cpu(V_LI,selected_cpu), (int *)arg, sizeof(unsigned long int) )) {
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl SET_V_LI - Pass error")  ;			
				return -EFAULT;
			}
			break;


		case GET_t_LI:		
			if (copy_to_user((long unsigned int *)arg, &per_cpu(t_LI,selected_cpu), sizeof(unsigned long int))){
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl GET_t_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		case SET_t_LI:
			if ( copy_from_user( &per_cpu(t_LI,selected_cpu), (int *)arg, sizeof(unsigned long int) )) {
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl SET_t_LI - Pass error")  ;			
				return -EFAULT;
			}	
			break;

		case SET_V_STC:
			if ( copy_from_user( &per_cpu(V_STC,selected_cpu), (int *)arg, sizeof(long unsigned int) )) {
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl SET_V_STC - Pass error")  ;			
				return -EFAULT;
			}	
			break;
	
		case GET_JIFFIES:
			j = jiffies;
			if (copy_to_user((long unsigned int *)arg, &j, sizeof(jiffies))){
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl GET_JIFFIES - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		case PASS_PID:
			
			if (copy_from_user(&pid_p , (pid_t *)arg , sizeof(pid_t)) ) {
				printk(KERN_ALERT "PIETRO LTC MODULE : ioctl PASS_PID - pass error");
				return -EFAULT;
			}
			else printk(KERN_ALERT "PIETRO LTC MODULE : ioctl PASS_PID successful !!");


			#if 0
			/* send the signal */
			memset(&info, 0, sizeof(struct siginfo));
			info.si_signo = SIGUSR1;
			info.si_code = SI_QUEUE;	// this is bit of a trickery: SI_QUEUE is normally used by sigqueue from user space,
							// and kernel space should use SI_KERNEL. But if SI_KERNEL is used the real_time data 
							// is not delivered to the user space signal handler function. 
			info.si_int = 1234;  		//real time signals may have 32 bits of data.
			rcu_read_lock();
			t = pid_task(find_vpid(pid) , PIDTYPE_PID);  //find the task_struct associated with this pid
			if(t == NULL){
				printk("no such pid\n");
				rcu_read_unlock();
				return -ENODEV;
			}
			rcu_read_unlock();
			ret = send_sig_info(SIGUSR1, &info, t);    //send the signal
			if (ret < 0) {
				printk("error sending signal\n");
				return ret;
			}
			#endif

			break;

		case ACTIVATE_SAFE_MODE:	
			if (copy_from_user(&cpu_activate_safe_mode , (pid_t *)arg , sizeof(activate_safe_mode)) ) {
				printk(KERN_ALERT "PIETRO LTC MODULE : ioctl ACTIVATE_SAFE_MODE - pass error");
				return -EFAULT;
			}
			else{
				per_cpu(activate_safe_mode,cpu_activate_safe_mode) = 1;
				printk(KERN_ALERT "PIETRO LTC MODULE : ioctl ACTIVATE_SAFE_MODE %u successful !!", cpu_activate_safe_mode);
			}
			break;

		case GET_T_LI:		
			if (copy_to_user((long unsigned int *)arg, &T_LI, sizeof(unsigned long int)) ){
				printk (KERN_ALERT "PIETRO LTC MODULE : ioctl GET_T_LI - Pass error")  ;				 
				return -EFAULT;
			}
			break;

		default:
			printk(KERN_ALERT "DEBUG: ltc_ioctrl - You should not be here!!!!\n");
			retval = -EINVAL;
	}

	return retval;

}




// define which file operations are supported
struct file_operations ltc_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	NULL,
	.write		=	NULL,
	.readdir	=	NULL,
	.poll		=	NULL,
	.unlocked_ioctl	=	ltc_ioctl,
	.mmap		=	NULL,
	.open		=	ltc_open,
	.flush		=	NULL,
	.release	=	ltc_release,
	.fsync		=	NULL,
	.fasync		=	NULL,
	.lock		=	NULL,
	//.readv		=	NULL,
	//.writev		=	NULL,
};






//module initialization
static int __init ltc_init_module(void) {
    int     rc = 0;

    printk( ltcBanner );

    if ( MAJOR( ltcDrvDevNum ) == 0 )
    {
        /* Allocate a major number dynamically */
        if (( rc = alloc_chrdev_region( &ltcDrvDevNum, 0, 1, LTC_NAME )) < 0 )
        {
            printk( KERN_WARNING "%s: alloc_chrdev_region failed; err: %d\n", __func__, rc );
            return rc;
        }
    }
    else
    {
        /* Use the statically assigned major number */
        if (( rc = register_chrdev_region( ltcDrvDevNum, 1, LTC_NAME )) < 0 )  //P: returns 0 if ok, negative if error
        {
           printk( KERN_WARNING "%s: register_chrdev failed; err: %d\n", __func__, rc );
           return rc;
        }
    }

    cdev_init( &ltcDrvCDev, &ltc_fops );
    ltcDrvCDev.owner = THIS_MODULE;

    if (( rc = cdev_add( &ltcDrvCDev, ltcDrvDevNum, 1 )) != 0 )
    {
        printk( KERN_WARNING "%s: cdev_add failed: %d\n", __func__, rc );
        goto out_unregister;
    }

    /* Now that we've added the device, create a class, so that udev will make the /dev entry */

    ltcDrvClass = class_create( THIS_MODULE, LTC_NAME );
    if ( IS_ERR( ltcDrvClass ))
    {
        printk( KERN_WARNING "%s: Unable to create class\n", __func__ );
        rc = -1;
        goto out_cdev_del;
    }

    device_create( ltcDrvClass, NULL, ltcDrvDevNum, NULL, LTC_NAME );

    goto done;

out_cdev_del:
    cdev_del( &ltcDrvCDev );

out_unregister:
    unregister_chrdev_region( ltcDrvDevNum, 1 );

done:
    return rc;
}

//module exit function
static void __exit ltc_cleanup_module (void) {
        printk(KERN_ALERT "cleaning up module\n");
	
	device_destroy( ltcDrvClass, ltcDrvDevNum );
	class_destroy( ltcDrvClass );
	cdev_del( &ltcDrvCDev );

	unregister_chrdev_region( ltcDrvDevNum, 1 );

}



module_init(ltc_init_module);
module_exit(ltc_cleanup_module);
MODULE_AUTHOR(" Pietro Mercati and Andrea Bartolini" );
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("something");




