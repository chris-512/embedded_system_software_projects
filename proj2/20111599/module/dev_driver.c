/* 
	DEV Ioremap Control

	FILE 	: 	dev_driver.c

	AUTH 	:	Sae Young KIM
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#define FND_MAJOR					241
#define FND_NAME					"fnd_driver"

// DEVICE DRIVER INFO
#define DEV_MAJOR   				242          	// dev device minor number
#define DEV_MINOR   				0         		// dev device minor number
#define DEV_NAME    				"dev_driver"    // dev device name

#define UON							0x00			// IOM
#define UOFF						0x01			// IOM

// (1) GPIO FND
#define FND_GPL2CON 				0x11000100  // Pin Configuration
#define FND_GPL2DAT 				0x11000104  // Pin Data
#define FND_GPE3CON 				0x11400140  // Pin Configuration
#define FND_GPE3DAT 				0x11400144  // Pin Data

// (2) GPIO LED
#define LED_GPBCON					0x11400040
#define LED_GPBDAT					0x11400044

// (1) FPGA LED
#define IOM_LED_ADDRESS				0x04000016	// physical address

// (2) FPGA FND
#define IOM_FND_ADDRESS				0x04000004  // physical address

// (3) FPGA DOT MATRIX
#define IOM_FPGA_DOT_ADDRESS		0x04000210  // physical address

// (4) FPGA TEXT LCD
#define IOM_FPGA_TEXT_LCD_ADDRESS	0x04000100  // physical address

// FPGA DEMO
#define IOM_FPGA_DEMO_ADDRESS		0x04000300  // physical address

// device open, close, write functions
int 	dev_open(struct inode *, struct file *);
int 	dev_release(struct inode *, struct file *);
ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
int 	dev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long gdata);

typedef enum { GPIO_FND_ON = 3, GPIO_LED_ON, FPGA_LED_ON, FPGA_DOT_MATRIX_ON, FPGA_FND_ON, FPGA_TEXT_LCD_ON} ioctlCmdType;

// Global variable
static int dev_usage = 0;

// 주어진 주소들을 통해 GPIO 및 FPGA 장치 제어 가능
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;
static unsigned char *iom_fpga_dot_addr;
static unsigned char *iom_fpga_demo_addr;

static unsigned char *led_data;
static unsigned char *led_ctrl;

static unsigned char *fnd_data;
static unsigned char *fnd_data2;
static unsigned char *fnd_ctrl;
static unsigned char *fnd_ctrl2;

struct timer_list timer;
unsigned char fnd_idx,  fnd_val;
unsigned char t_intval, t_count;

const unsigned char fnd_loc[] = { 0x02, 0x04, 0x10, 0x80 };
const unsigned char fnd_sym[] = { 0x73, 0x8f, 0x71, 0x8d, 0x61, 0x0d, 0x21, 0x05 };
const unsigned char gpio_led[] = { 0xE0, 0xD0, 0xB0, 0x70 };
const unsigned char fpga_led[] = 
{ 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
const unsigned char fpga_dot_sym[8][10] = {
	{0x7f, 0x7f, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60},
	{0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f},
	{0x7f, 0x7f, 0x60, 0x60, 0x7f, 0x7f, 0x60, 0x60, 0x60, 0x60},
	{0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7f},
	{0x7f, 0x7f, 0x60, 0x60, 0x7f, 0x7f, 0x60, 0x60, 0x7f, 0x7f},
	{0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7f},
	{0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x7f, 0x60, 0x60, 0x7f, 0x7f},
	{0x7f, 0x7f, 0x03, 0x03, 0x7f, 0x7f, 0x63, 0x63, 0x7f, 0x7f}
};
unsigned char student_no[] 		= "20111599";
unsigned char student_name[] 	= "KIM SAE YOUNG";
int dir  = -1;
int flag = 1;
int first_row_idx, second_row_idx;

void clear_device(void)
{
		int i;
		/* clear gpio fnd */
		outb(0xFF, (unsigned int) fnd_data);
		outb(0xFF, (unsigned int) fnd_data2);

		/* clear gpio led */
		outb(0xF0, (unsigned int) led_data);

		/* clear fpga led */
		outb(0, (unsigned int) iom_fpga_led_addr);

		/* clear fpga fnd */
		for(i = 0; i < 4; i++)
			outb(0, (unsigned int) iom_fpga_fnd_addr+i);

		/* clear fpga dot matrix */
		for(i = 0; i < 10; i++)	
			outb(0, (unsigned int) iom_fpga_dot_addr+i);

		/* clear fpga text lcd */
		for(i = 0; i < 32; i++)
			outb(0, (unsigned int) iom_fpga_text_lcd_addr+i);
}

static struct file_operations dev_fops =
{
		.open       = dev_open,
		.write      = dev_write,
		.release    = dev_release,
		.ioctl		= dev_ioctl
};
int dev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long gdata)
{
	unsigned char fnd_sel, fnd_dat;
	unsigned char value[4];
	unsigned char fpga_text_lcd[32];
   
	// 들어오는 ioctl cmd 인자에 따라 6가지의 디바이스 (GPIO + FPGA) 달리 제어
	printk("dev_ioctl : cmd %d\n", cmd);
	switch(cmd)
	{
	case GPIO_FND_ON: // GPIO FND 제어
	{
		int fnd_idx = gdata & 0xff;
		int fnd_val = gdata >> 8;

		fnd_sel = fnd_loc[fnd_idx];
		fnd_dat = fnd_sym[fnd_val];

		outb(fnd_dat, (unsigned int) fnd_data);
		outb(fnd_sel, (unsigned int) fnd_data2);
	}
    break;
	case GPIO_LED_ON: // GPIO LED 제어
	{
		int fnd_idx = gdata & 0xff;
		outb(gpio_led[fnd_idx], (unsigned int) led_data);
	}
    break;
	case FPGA_FND_ON: // FPGA FND 제어
	{
		int tmp = gdata;
		int length = 4;
		int i;
		while(length--)
		{
			value[length] = tmp % 10;
			tmp /= 10;
		}
		for(i = 0; i < 4; i++)
			outb(value[i], (unsigned int) iom_fpga_fnd_addr+i);
	}
	break;
	case FPGA_LED_ON: // FPGA LED 제어
	{
		int fnd_val = gdata >> 8;
		outb(fpga_led[fnd_val], (unsigned int) iom_fpga_led_addr);
	}
	break;
	case FPGA_TEXT_LCD_ON: // FPGA TEXT LCD 제어
	{
		int first_row_idx = gdata & 0xff;
		int second_row_idx = gdata >> 8;
		int i;

		memset(fpga_text_lcd, ' ', 32);

		for(i = 0; i < strlen(student_no); i++)
			fpga_text_lcd[first_row_idx + i] = student_no[i];
		for(i = 0; i < strlen(student_name); i++)
			fpga_text_lcd[16 + second_row_idx + i] = student_name[i];

		for(i = 0; i < 32; i++)
			outb(fpga_text_lcd[i], (unsigned int) iom_fpga_text_lcd_addr+i);
	}
	break;
	case FPGA_DOT_MATRIX_ON: // FPGA DOT MATRIX 제어
	{
		int fnd_val = gdata >> 8;
		int i;
		for(i = 0; i < 10; i++)
			outb(fpga_dot_sym[fnd_val][i], (unsigned int) iom_fpga_dot_addr+i);
	}
	break;
	}

	return 0;
}
int dev_open(struct inode *minode, struct file *mfile)
{
		if(dev_usage != 0)
				return -EBUSY;

		dev_usage = 1;

		return 0;
}
int dev_release(struct inode *minode, struct file *mfile)
{
		dev_usage = 0;

		// Clear Device : 디바이스 초기화
		clear_device();

		return 0;
}
void timer_handler(unsigned long arg)
{
		int i;

		/* gpio fnd control */
		unsigned char fnd_sel, fnd_dat;
		unsigned char value[4];
		unsigned char fpga_text_lcd[32];
		unsigned char tmp = t_count;
		int length = 4;

		fnd_sel = fnd_loc[fnd_idx];
		fnd_dat = fnd_sym[fnd_val];

		outb(fnd_dat, (unsigned int) fnd_data);
		outb(fnd_sel, (unsigned int) fnd_data2);

		/* gpio led control */
		outb(gpio_led[fnd_idx], (unsigned int) led_data);

		/* fpga led control */
		outb(fpga_led[fnd_val], (unsigned int) iom_fpga_led_addr);

		while(length--)
		{
			value[length] = tmp % 10;
			tmp /= 10;
		}

		/* fpga fnd control */
		for(i = 0; i < 4; i++)
			outb(value[i], (unsigned int) iom_fpga_fnd_addr+i);

		/* fpga dot matrix control */
		for(i = 0; i < 10; i++)	
			outb(fpga_dot_sym[fnd_val][i], (unsigned int) iom_fpga_dot_addr+i);

		// TEXT LCD를 먼저 ' '로 초기화
		memset(fpga_text_lcd, ' ', 32);

		// 그 이후에, 학번 및 이름을 올바른 위치에 복사.
		for(i = 0; i < strlen(student_no); i++)
			fpga_text_lcd[first_row_idx + i] = student_no[i];
		for(i = 0; i < strlen(student_name); i++)
			fpga_text_lcd[16 + second_row_idx + i] = student_name[i];

		// TEXT LCD 애니메이션 효과를 위한 인덱스 조절
		if((dir == 1) && (16 - (second_row_idx + strlen(student_name)) > 0))
		{
			first_row_idx++;
			second_row_idx++;
		} 
		else if((dir == -1) && (second_row_idx > 0))
		{
			first_row_idx--;
			second_row_idx--;
		} else {
			dir *= -1;
			if(dir == 1) {
				first_row_idx++;
				second_row_idx++;
			} else {
				first_row_idx--;
				second_row_idx--;
			}
		}

		/* fpga text lcd control */
		for(i = 0; i < 32; i++)
			outb(fpga_text_lcd[i], (unsigned int) iom_fpga_text_lcd_addr+i);

		// change the symbol value and location
		fnd_val = (fnd_val + 1) % 8;
		if(fnd_val == 0)
			fnd_idx = (fnd_idx + 1) % 4;

		--t_count; // 타이머 횟수가 1씩 줄어들음
		if(t_count == 0xff) // t_count가 -1이 되면 더이상 타이머 발생하지 않음.
				return;

		timer.expires = get_jiffies_64() + (t_intval * HZ / 10); // 현재 jiffifes 값 반영하여 새로 등록
		add_timer(&timer);
}
ssize_t dev_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
		long tmp = *((const long *) gdata);

		fnd_idx  = (tmp >> 24) & 0xff; // 시작 위치
		fnd_val  = (tmp >> 16) & 0xff; // 시작 값
		t_intval = (tmp >>  8) & 0xff; // 시간 간격
		t_count  = (tmp)       & 0xff; // 타이머 횟수
		
		// 디버깅용 출력
		printk("fnd index:     %d\n", fnd_idx);
		printk("fnd value:     %d\n", fnd_val);
		printk("time interval: %d\n", t_intval);
		printk("time count:    %d\n", t_count);

		fnd_val--;

		// 초기 학번 및 이름 위치 지정
		first_row_idx  = 8 - strlen(student_no)   / 2;
		second_row_idx = 8 - strlen(student_name) / 2;

		// init timer
		init_timer(&timer);
		timer.expires 	= get_jiffies_64() + (t_intval * HZ / 10); // 0.(t_intval)초의 time quantum 지정
		timer.data 		= 0;
		timer.function  = timer_handler; // time quantum이 expire하게 되면 호출되는 함수
		add_timer(&timer);

		return length;
}
int __init dev_init(void)
{
		int result;

		struct class *fnd_dev_class = NULL;
		struct device *fnd_dev = NULL;

		result = register_chrdev(DEV_MAJOR, DEV_NAME, &dev_fops);
		if(result < 0) {
				printk(KERN_WARNING"Can't get any major!\n");
				return result;
		}

		// (1) remap gpio led addr
		led_data = ioremap(LED_GPBDAT, 0x01);
		if(led_data == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}

		led_ctrl = ioremap(LED_GPBCON, 0x04);;
		if(led_ctrl == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		} 
		outb(0xF0, (unsigned int) led_data);

		// (2) remap gpio fnd addr
		fnd_data  = ioremap(FND_GPL2DAT, 0x01);
		fnd_data2 = ioremap(FND_GPE3DAT, 0x01);
		if(fnd_data == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}

		fnd_ctrl  = ioremap(FND_GPL2CON, 0x04);
		fnd_ctrl2 = ioremap(FND_GPE3CON, 0x04);
		if(fnd_ctrl == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		} 
		else 
		{
				fnd_dev = device_create(fnd_dev_class,NULL,MKDEV(FND_MAJOR,0),NULL,FND_NAME);
				if(fnd_dev != NULL)
				{
						outl(0x11111111,(unsigned int)fnd_ctrl);
						outl(0x10010110,(unsigned int)fnd_ctrl2);
				}
				else 
						printk("device_create : failed!\n");
		}

		outb(0xFF, (unsigned int)fnd_data);

		// (1) remap fpga fnd addr
		if((iom_fpga_fnd_addr		= ioremap( IOM_FND_ADDRESS,					0x4)) == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}

		// (2) remap fgpa led addr
		if((iom_fpga_led_addr 		= ioremap( IOM_LED_ADDRESS, 				0x1)) == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}

		// (3) remap fpga text lcd addr
		if((iom_fpga_text_lcd_addr 	= ioremap( IOM_FPGA_TEXT_LCD_ADDRESS, 	0x20)) == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}
				
		// (4) remap fpga dot addr
		if((iom_fpga_dot_addr		= ioremap( IOM_FPGA_DOT_ADDRESS,			0x10)) == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}

		// (5) remap fpga demo addr
		if((iom_fpga_demo_addr 	   	= ioremap( IOM_FPGA_DEMO_ADDRESS, 		0x1)) == NULL)
		{
				printk("ioremap failed!\n");
				return -1;
		}
		outb(UON, (unsigned int) iom_fpga_demo_addr);

		printk("init module, /dev/dev_driver major : %d\n", DEV_MAJOR);

		return 0;
}

void __exit dev_exit(void)
{
		// Clear Device
		clear_device();

		// (1) - unmap gpio fnd driver
		outb(0xFF, (unsigned int) fnd_data);
		iounmap(fnd_data);
		iounmap(fnd_data2);
		iounmap(fnd_ctrl);
		iounmap(fnd_ctrl2);

		// (2) - unmap gpio led driver
		outb(0xF0, (unsigned int) led_data);
		iounmap(led_data);
		iounmap(led_ctrl);

		// (1) - unmap fpga fnd driver 
		iounmap(iom_fpga_fnd_addr);

		// (2) - unmap fpga led driver
		iounmap(iom_fpga_led_addr);

		// (3) - unmap fpga text lcd driver
		iounmap(iom_fpga_text_lcd_addr);

		// (4) - unmap fpga dot matrix driver
		iounmap(iom_fpga_dot_addr);

		// (5) - unmap fpga demo driver
		iounmap(iom_fpga_demo_addr);

		unregister_chrdev(DEV_MAJOR, DEV_NAME);

		del_timer(&timer);


		printk("/dev/dev_driver module removed.\n");
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Kim Sae Young");
