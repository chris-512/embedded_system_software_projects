#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>

#define FND_MAJOR       241
#define FND_NAME        "fnd_driver"

#define DEV_MAJOR       245
#define DEV_NAME        "stopwatch"

// GPIO FND
#define FND_GPL2CON     0x11000100 // Pin configuration
#define FND_GPL2DAT     0x11000104 // Pin Data
#define FND_GPE3CON     0x11400140 // Pin configuration
#define FND_GPE3DAT     0x11400144 // Pin Data

const unsigned char fnd_loc[] = { 0x02, 0x04, 0x10, 0x80 };
const unsigned char fnd_val[] = 
{ 
    0x03, // 0
    0x9F, // 1
    0x25, // 2
    0x0D, // 3
    0x99, // 4
    0x49, // 5
    0x41, // 6
    0x1F, // 7
    0x01, // 8
    0x09  // 9
};

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);
int     inter_open(struct inode *, struct file *);
int     inter_release(struct inode *, struct file *);
ssize_t inter_write(struct file *, const unsigned long *, size_t, loff_t *);

irqreturn_t inter_handler(int irq, void* dev_id, struct pt_regs* reg);

static int inter_usage  = 0;
static int stopWatchEnabled;
struct timer_list timer1, timer2, exit_timer, get_value_timer;
int timerCount;
int isStopWatchPaused;

static unsigned char *fnd_data;
static unsigned char *fnd_data2;
static unsigned char *fnd_ctrl;
static unsigned char *fnd_ctrl2;

static struct file_operations inter_fops =
{
	.open      = inter_open,
	.write     = inter_write,
	.release   = inter_release,
};

int fnd_dat[4];

int arg;
int delay;
int flag;
void stopwatch_handler(unsigned long arg)
{
    int min, sec;

    if(!isStopWatchPaused && flag) 
    {
        timerCount++;
        flag = 0;
    }

    min        = timerCount / 60;
    sec        = timerCount % 60;

    fnd_dat[0] = (min / 10) % 10;
    fnd_dat[1] =        min % 10;

    fnd_dat[2] = (sec / 10) % 10;
    fnd_dat[3] =        sec % 10;

    outb(fnd_val[fnd_dat[arg]], (unsigned int) fnd_data);
    outb(fnd_loc[arg], (unsigned int) fnd_data2);
    arg = (arg + 1) % 4;

    delay++;
    // 1 second
    timer1.expires = get_jiffies_64() + (1 * HZ / 10000);
    timer1.data    = arg;
    add_timer(&timer1); 
}
void stopwatch_handler2(unsigned long arg)
{
    flag = 1;

    timer2.expires = get_jiffies_64() + (1 * HZ);
    add_timer(&timer2);
}

/*
 * function name : sw2_intrhandler
 * description   : When pressing the sw2 gpio button,
 *                 change fnd information per second.
 */
irqreturn_t sw2_intrhandler(int irq, void* dev_id, struct pt_regs* reg)
{
    isStopWatchPaused = 0;
    flag = 0;

    del_timer(&timer1);
    del_timer(&timer2);

        // register timer1
        init_timer(&timer1);
        timer1.expires = get_jiffies_64() + (1 * HZ / 10000);
        timer1.data = 0;
        timer1.function = stopwatch_handler;
        add_timer(&timer1);

        // register timer2
        init_timer(&timer2);
        timer2.expires = get_jiffies_64() + (1 * HZ);
        timer2.data = 0;
        timer2.function = stopwatch_handler2;
        add_timer(&timer2);

	return IRQ_HANDLED;
}
/*
 * function name : sw3_intrhandler
 * description   : When pressing the sw3 gpio button,
 *                 pause the stopwatch.
 */
irqreturn_t sw3_intrhandler(int irq, void* dev_id, struct pt_regs* reg)
{
    isStopWatchPaused = 1;

	return IRQ_HANDLED;
}   
/*
 * function name : sw4_intrhandler
 * description   : Go the the beginning -> fnd: 0000
 *
 */
irqreturn_t sw4_intrhandler(int irq, void* dev_id, struct pt_regs* reg)
{
    timerCount = 0;
    isStopWatchPaused = 1;

	return IRQ_HANDLED;
}

int isThreeSecMotion;
int exitFlag;

void exit_motion_check_handler(unsigned long arg)
{
    exitFlag = 0;
}
void exit_get_value_handler(unsigned long arg)
{
    if(exitFlag == 0 && isThreeSecMotion)
    {
        wake_up_interruptible(&wq_write);
        printk("wake_up\n");

        del_timer(&exit_timer);
        del_timer(&get_value_timer);
        return;
    }

    if(gpio_get_value(S5PV310_GPX2(4)))
    {
        isThreeSecMotion = 0;

        del_timer(&exit_timer);
        del_timer(&get_value_timer);
        return;
    }

    get_value_timer.expires = get_jiffies_64() + (1 * HZ / 10000);
    add_timer(&get_value_timer);
}

/*
 * function name : sw6_intrhandler
 * description   : Terminate the application and turn off fnd.
 *
 */
irqreturn_t sw6_intrhandler(int irq, void* dev_id, struct pt_regs* reg)
{
    exitFlag = isThreeSecMotion = 1;

    init_timer(&exit_timer);
    exit_timer.expires = get_jiffies_64() + (3 * HZ);
    exit_timer.data = 0;
    exit_timer.function = exit_motion_check_handler;
    add_timer(&exit_timer);
   
    init_timer(&get_value_timer);
    get_value_timer.expires = get_jiffies_64() + (1 * HZ / 10000);
    get_value_timer.data = 0;
    get_value_timer.function = exit_get_value_handler;
    add_timer(&get_value_timer);

	return IRQ_HANDLED;
}
int inter_open(struct inode *minode, struct file *mfile)
{
	int ret;
	if(inter_usage != 0)
		return -EBUSY;

	inter_usage = 1;

	/*
	*       SW2 : GPX2(0)
	*       SW3 : GPX2(1)
	*       SW4 : GPX2(2)
	*       SW6 : GPX2(4)
	*
	*       VOL+    : GPX2(5)
	*       POWER   : GPX0(1)
	*       VOL-    : GPX0(1)
	*
	*		SW2,3,4,6 : PRESS-FALLING-0 / RELEASE-RISING-1
	*		VOL -, +  : PRESS-RISING-1  / RELEASE-FALLING-0
	*		POWER	  : PRESS-FALLING-0 / RELEASE-RISING-1
	*/

    // Installing an interrupt handler

    // SW2 : start -> 1 sec : update fnd information
	ret = request_irq(gpio_to_irq(S5PV310_GPX2(0)), &sw2_intrhandler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "X2.0", NULL);

    // SW3 : pause -> pause
	ret = request_irq(gpio_to_irq(S5PV310_GPX2(1)), &sw3_intrhandler, IRQF_TRIGGER_RISING, "X2.1", NULL);

    // SW4 : reset -> Initialize your stopwatch
	ret = request_irq(gpio_to_irq(S5PV310_GPX2(2)), &sw4_intrhandler, IRQF_TRIGGER_FALLING, "X2.2", NULL);

    // SW6 : exit -> If you press a button longer than 3 sec, terminate the program and turn off your fnd.
	ret = request_irq(gpio_to_irq(S5PV310_GPX2(4)), &sw6_intrhandler, IRQF_TRIGGER_FALLING, "X2.4", NULL);

	return 0;
}

int inter_release(struct inode *minode, struct file *mfile)
{
	inter_usage = 0;
    stopWatchEnabled = 0;
    timerCount = 0;

    outb(0xFF, (unsigned int) fnd_data);

    // freeing an interrupt handler
	free_irq(gpio_to_irq(S5PV310_GPX2(0)), NULL);
	free_irq(gpio_to_irq(S5PV310_GPX2(1)), NULL);
	free_irq(gpio_to_irq(S5PV310_GPX2(2)), NULL);
	free_irq(gpio_to_irq(S5PV310_GPX2(4)), NULL);

	return 0;
}
ssize_t inter_write(struct file *inode, const unsigned long *gdata, size_t length, loff_t *off_what)
{
	printk("sleep on\n");
	interruptible_sleep_on(&wq_write);
	printk("write\n");

    del_timer(&timer1);
    del_timer(&timer2);
    del_timer(&exit_timer);
    del_timer(&get_value_timer);

	return length;
}

int __init inter_init(void)
{
	int result;

    struct class *fnd_dev_class = NULL;
    struct device *fnd_dev = NULL;

	result = register_chrdev(DEV_MAJOR, DEV_NAME, &inter_fops);
	if(result < 0)
    {
		printk(KERN_WARNING "Can't get any major!\n");
		return result;
	}

    // remap gpio fnd addr
    fnd_data    = ioremap(FND_GPL2DAT, 0x01);
    fnd_data2   = ioremap(FND_GPE3DAT, 0x01);
    if(fnd_data == NULL) 
    {
        printk("ioremap failed\n");
        return -1;
    }

    fnd_ctrl    = ioremap(FND_GPL2CON, 0x04);
    fnd_ctrl2   = ioremap(FND_GPE3CON, 0x04);
    if(fnd_ctrl == NULL)
    {
        printk("ioremap failed\n");
        return -1;
    } 
    else 
    {
        fnd_dev = device_create(fnd_dev_class, NULL, MKDEV(FND_MAJOR, 0), NULL, FND_NAME);
        if(fnd_dev != NULL) 
        {
            outl(0x11111111, (unsigned int) fnd_ctrl);
            outl(0x10010110, (unsigned int) fnd_ctrl2);
        } 
        else 
        {
            printk("device_create : failed!\n");
        }
    }

    outb(0x03, (unsigned int) fnd_data);
    outb(0xFF, (unsigned int) fnd_data2);

    printk("init module, /dev/stopwatch major: %d\n", DEV_MAJOR);

	return 0;
}
void __exit inter_exit(void)
{
    outb(0xFF, (unsigned int) fnd_data);

    // unmap gpio fnd driver
    iounmap(fnd_data);
    iounmap(fnd_data2);
    iounmap(fnd_ctrl);
    iounmap(fnd_ctrl2);

    del_timer(&timer1);
    del_timer(&timer2);

	unregister_chrdev(DEV_MAJOR,DEV_NAME);
}

module_init(inter_init);
module_exit(inter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kim Sae Young");
