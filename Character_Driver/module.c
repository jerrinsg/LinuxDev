#include <linux/init.h> 				// Specifies the intialization and cleanup functions 
#include <linux/module.h> 			// THIS_MODULE, attribute
#include <linux/kernel.h> 			// printk, ... 
#include <linux/slab.h> 				// kmalloc() 

/* #include <version.h> for module version to be compatible with the kernel into which it is going to be loaded. ...required ??  */

#include <asm/uaccess.h> 			// copy_to_user() copy_from_user()
#include <linux/types.h> 			// dev_t
#include <linux/kdev_t.h> 			// MKDEV, MAJOR, MINOR
#include <linux/fs.h> 				// alloc_chrdev_region() , file_operations

#include <linux/device.h> 			// class_create()
#include <linux/cdev.h>  			// cdev_init() and cdev_add()
	
#define MAX_LENGTH 50

static char *memory_buffer;			// Stores messages written into the device driver
static short count, flag;			// flag - simple hack to avoid infinite o/p sequence
static dev_t first; 				// Global variable for the first device number  
static struct cdev c_dev; 			// Global variable for the character device structure  
static struct class *cl; 			// Global variable for the device class  

/*** functions for the desired file operations ***/

static int scull_open(struct inode *i, struct file *f)  {

  	printk(KERN_INFO "Driver: open()\n");
  	return 0;						// Driver successfully opened
}

static int scull_close(struct inode *i, struct file *f)  {
  	printk(KERN_INFO "Driver: close()\n");
  	return 0;						// Driver successfully closed
}

/* ssize_t is an signed size value; */
/* __user is a form of documentation, noting that a pointer is a user-space address that cannot be directly dereferenced. It can be used by external checking software to find misuse of user-space address */

/* APIs just to ensure that user-space buffers are safe to access, and then updated them */

static ssize_t scull_read(struct file *f, char __user *buf, size_t len, loff_t *off)  {
/* When the user requests to read data from the device, the driver must WRITE data back to the buffer */
	printk(KERN_INFO "Driver: write()\n");
	
  	if(flag==0)
  		return 0;
	  	
	if(copy_to_user(buf, memory_buffer, count) != 0)  // Copy data from kernel space to user space. 
		return -EFAULT;
  	
  	flag=0;
  	
  	printk(KERN_INFO "Driver : Written %d bytes to userspace\n", count);
  	return count;					// No of bytes read
	
}

static ssize_t scull_write(struct file *f, const char __user *buf, size_t len, loff_t *off)  {
/* When the user requests to write into the device, the driver must READ back data from the buffer  */
  	printk(KERN_INFO "Driver: write()\n");
  	memset(memory_buffer, 0, MAX_LENGTH);
  	count=0;	
  	
  	if(len > MAX_LENGTH)
  		len=MAX_LENGTH;
  	
  	while(len--)  {
  	
  		if(copy_from_user(memory_buffer+count++, buf++, 1) != 0) 
  			return -EFAULT;
  		flag=1;
  	}
  	
  	printk(KERN_INFO "Driver: Read %d bytes from userspace()\n", count);
  	return count;
}

/* Filling file operations structure with desired file operations */
static struct file_operations pugs_fops =  {
  	.owner = THIS_MODULE,
  	.open = scull_open,
  	.release = scull_close,
  	.read = scull_read,
  	.write = scull_write
  	
/* Standard C tagged structure initialization syntax. It makes driver more portable across changes in the definitions of the structures, code more compact and readable */
};

/* Initialization function should be declared static, since they are not meant to be visible outside the specific file. __init token is a hint to the kernel that the given function is used only at initialization time. The module loader drops the initialization function after the module is laoded, making its memory available for other users. */

static int __init hello_init(void) {
	int result;

  	printk(KERN_INFO "Hello world!\n"); 	// printk because kernel runs by itself, without the help of the C library  
  	flag=0;							// Hack to prevent infinite sequence (:/)
	memory_buffer = kmalloc(MAX_LENGTH, GFP_KERNEL);// Allocating memory for the buffer */
	if (!memory_buffer) 
		return -ENOMEM;

	memset(memory_buffer, 0, MAX_LENGTH);
  	
  	/* Registering the device files  */
  	/* int alloc_chrdev_region(dev_t *first, unsigned int firstminor, unsigned int count, char *name); */
  	result = alloc_chrdev_region(&first, 0, 1, "scull");		

  	if (result < 0)  {
  		printk(KERN_WARNING "Scull: can't get major|Device registration failed");
    		return result;
	}
	printk(KERN_INFO "Scull Registered <Major, Minor>: <%d, %d>\n", MAJOR(first), MINOR(first));
	
	/* Creating the device class. chardrv is the device class name   */
	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)  {
    		unregister_chrdev_region(first, 1);
    		return -1;
  	}
    	
    	/* Populating the device info under this class. scull is the device name   */
    	if (device_create(cl, NULL, first, NULL, "scull") == NULL)  {

    		class_destroy(cl);
    		unregister_chrdev_region(first, 1);
    		return -1;
  	}
	/* Multiple device files be created (in a loop) by device_create(cl, NULL, MKNOD(MAJOR(first), MINOR(first) + i), NULL, "scull%d", i);*/	
  	
  	/* Initialising the character device structure with file operations */
	cdev_init(&c_dev, &pugs_fops);
	
	/* Hand the character device structure to the VFS  */
	if (cdev_add(&c_dev, first, 1) == -1)  {
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}	
	
	return 0;
}

/* __exit indicates code executed during module unload only  */
static void __exit hello_exit(void) {
	
	cdev_del(&c_dev);
	device_destroy(cl, first);
  	class_destroy(cl);
	
	if (memory_buffer) {
		kfree(memory_buffer);
	}
	
	unregister_chrdev_region(first, 1);
  	printk(KERN_INFO "Bye, world !\n");	
}

 /* Macros */
module_init(hello_init);
module_exit(hello_exit);

/* Module Attributes  */

/* This avoids kernel taint warning [WARNING: modpost: missing MODULE_LICENSE()]. kernel gets tainted! Default license assumed is proprietery */
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("My First Device Driver");
MODULE_AUTHOR("Jerrin Shaji George");
/* Reference : http://www.linuxforu.com/tag/linux-device-drivers/page/2/ */
