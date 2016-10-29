#ifndef _DEVICE_HEADER_
#define _DEVICE_HEADER_

// MMAP device
#define MEM_DEVICE     	 	"/dev/mem"

// MMAP - FND mmap version (for GPIO)
#define FND_GPL_ADDR		0x11000000
#define FND_GPL_CON		0x0100
#define FND_GPL_DAT		0x0104

#define FND_GPE_ADDR		0x11400000
#define FND_GPE_CON		0x00140
#define FND_GPE_DAT		0x00144

// MMAP - TEXT LCD mmap version
#define FPGA_LCD_ADDR 		0x04000000
#define FPGA_LCD_OFFSET 	0x100

// FPGA - TEXT LCD device version
#define FPGA_LCD_DEVICE		"/dev/fpga_text_lcd"
// FPGA - DOT MATRIX
#define FPGA_DOT_DEVICE 	"/dev/fpga_dot"
// FPGA - FND device version (for FPGA)
#define FPGA_FND_DEVICE		"/dev/fpga_fnd"
// FPGA - SWITCH
#define FPGA_SWITCH_DEVICE	"/dev/fpga_push_switch"

#define FPGA_MAX_SWITCH 	9
#define FPGA_MAX_LCD		32

// GPIO_LED
#define GPIO_LED_DEVICE		"/dev/led_driver"
// GPIO_FND
#define GPIO_FND_DEVICE		"/dev/fnd_driver"
// GPIO INPUT
#define GPIO_INPUT_DEVICE    	"/dev/input/event1"


typedef struct InputDevice
{

} InputDevice;

typedef struct OutputDevice
{
	struct gpio {
		int led_fd;
		int fnd_fd;
		void *gpladdr;
		void *gpe_addr;
		unsigned long *gpe_con;
		unsigned long *gpe_dat;
		unsigned long *gpl_con;
		unsigned long *gpl_dat;
	} gpio;
	struct fpga {
		int lcd_fd;
		unsigned char *lcd_addr;

		int fnd_fd;
		int dot_fd;
	} fpga;
} OutputDevice;

static char switch_alphabet[9][3] = 
{
	{'.', 'Q', 'Z'},
	{'A', 'B', 'C'},
	{'D', 'E', 'F'},
	{'G', 'H', 'I'}, 
	{'J', 'K', 'L'},
	{'M', 'N', 'O'}, 
	{'P', 'R', 'S'},
	{'T', 'U', 'V'}, 
	{'W', 'X', 'Y'}
};

static unsigned char fnd_loc[] = {
	0x96,
	0x02,
	0x04,
	0x10,
	0x80,
	0x02|0x04|0x10|0x80,
};
static unsigned char fnd_val[] = {
	0x03, // 0
	0x9F, // 1
	0x25, // 2
	0x0D, // 3
	0x99, // 4
	0x49, // 5
	0x41, // 6
	0x1F, // 7
	0x01, // 8
	0x09, // 9
	0x11, // A 0001 0001
	0xC1, // b 1100 0001
	0x63, // C 0110 0011
	0x85, // d 1000 0101
	0x61, // E 0110 0001
	0x71, // F 0111 0001
};


extern void initOutputDevice(OutputDevice *);

extern void initFPGA(OutputDevice *);
extern void turnOffFPGA(OutputDevice *);
extern void initGPIO(OutputDevice *);
extern void turnOffGPIO(OutputDevice *);

extern void initDeviceClock(OutputDevice *);
extern void initDeviceCounter(OutputDevice *);
extern void initDeviceTextEditor(OutputDevice *);
extern void initDeviceDrawBoard(OutputDevice *);


extern void clearGPIOFND(OutputDevice*);
extern void writeGPIOFND(OutputDevice*, int, int);
extern void offGPIOLED(int);
extern void onGPIOLED(int, unsigned char);

extern void clearDotMatrix(int);
extern void writeFPGADotMatrix(int fd, char board[][7]);
extern void writeFPGACharDotMatrix(int);
extern void writeFPGANumDotMatrix(int);

extern void clearFPGAFND(int);
extern void writeFPGAFND(int, int);

extern void clearFPGALCD(unsigned char *);
extern void writeFPGALCD(unsigned char *, int, char);

extern void writeb(unsigned char *, int, char);
extern int  OneKeyPressed(unsigned char *);
extern int  TwoKeyPressed(unsigned char *);
extern int  KeyReleased(unsigned char *);
extern int  getOne(unsigned char *);

#endif
