#include "../setting.h"
#include "device.h"
#include "fpga_dot_font.h"

void initDeviceTextEditor(OutputDevice *outdev)
{
	clearFPGALCD(outdev->fpga.lcd_addr);
	writeFPGAFND(outdev->fpga.fnd_fd, 0);
	writeFPGACharDotMatrix(outdev->fpga.dot_fd);
}
void initDeviceDrawBoard(OutputDevice *outdev)
{
	clearFPGALCD(outdev->fpga.lcd_addr);
	writeFPGAFND(outdev->fpga.fnd_fd, 0);
	clearDotMatrix(outdev->fpga.dot_fd);
}
void turnOffFPGA(OutputDevice *outdev)
{
	clearFPGALCD(outdev->fpga.lcd_addr);
	writeFPGAFND(outdev->fpga.fnd_fd, 0);
	clearDotMatrix(outdev->fpga.dot_fd);
}
void initFPGA(OutputDevice *outdev)
{
	outdev->fpga.lcd_fd = open(MEM_DEVICE, O_RDWR|O_SYNC);
	if(outdev->fpga.lcd_fd < 0) {
		fprintf(stderr, "Device open error : %s\n", MEM_DEVICE);
		exit(-1);
	}
	outdev->fpga.lcd_addr = (unsigned char *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED, outdev->fpga.lcd_fd,FPGA_LCD_ADDR);
	outdev->fpga.lcd_addr += FPGA_LCD_OFFSET;

	// FPGA FND
	outdev->fpga.fnd_fd = open("/dev/fpga_fnd", O_RDWR);
	if(outdev->fpga.fnd_fd < 0) {
		fprintf(stderr, "Device open error : %s\n", FPGA_FND_DEVICE);
		exit(-1); 
	}

	// DOT MATRIX
	outdev->fpga.dot_fd = open(FPGA_DOT_DEVICE, O_WRONLY);
	if(outdev->fpga.dot_fd < 0) {
		fprintf(stderr, "Device open error : %s\n", FPGA_DOT_DEVICE);
		exit(-1); 
	}
}

void clearDotMatrix(int fd)
{
	static unsigned char mat[10] = 
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	write(fd, mat, sizeof(mat));
}
void writeFPGADotMatrix(int fd, char board[][7])
{
	int i, j;
	unsigned char mat[10] = {0, };

	for(i = 0; i < 10; i++) {
		for(j = 0; j < 7; j++) {
			if(board[i][j])
				mat[i] |= (1 << (6-j));
		}
	}
	write(fd, mat, sizeof(mat));
}
void writeFPGACharDotMatrix(int fd)
{
	write(fd, fpga_char, sizeof(fpga_char));
}
void writeFPGANumDotMatrix(int fd)
{
	write(fd, fpga_number[1], sizeof(fpga_number[1]));
}
void clearFPGAFND(int fd)
{
	unsigned char fnd_digits[4] = {0, };

	write(fd, fnd_digits, 4);
}
void writeFPGAFND(int fd, int count)
{
	int i;
	unsigned char fnd_digits[4] = {0, };

	for(i = 3; i >= 0 && count; i--) {
		fnd_digits[i] = count % 10;
		count /= 10;
	}
	write(fd, fnd_digits, 4);
}
void writeb(unsigned char *lcd_addr, int loc, char ch)
{
	lcd_addr[loc] = ch;
}
void writeFPGALCD(unsigned char *lcd_addr, int loc, char ch)
{
	int i;
	if(0 <= loc && loc < 8) {
		writeb(lcd_addr, loc, ch);
	} else if(loc == 8) {
		for(i = 1; i < 8; i++)
			lcd_addr[i-1] = lcd_addr[i];
		lcd_addr[7] = ch;
	}
}
int OneKeyPressed(unsigned char *fpga_sw)
{
	int i;
	int cnt = 0;
	for(i = 0; i < FPGA_MAX_SWITCH; i++)
		if(fpga_sw[i])
			cnt++;
	if(cnt == 1) return 1;
	return 0;
}

int TwoKeyPressed(unsigned char *fpga_sw)
{
	int i;
	int cnt = 0;
	for(i = 0; i < FPGA_MAX_SWITCH; i++)
		if(fpga_sw[i])
			cnt++;
	if(cnt == 2) return 1;
	return 0;
}

int KeyReleased(unsigned char *fpga_sw)
{
	int i;
	for(i = 0; i < FPGA_MAX_SWITCH; i++)
		if(fpga_sw[i])
			return 0;
	return 1;
}
void clearFPGALCD(unsigned char *lcd_addr)
{
	int i;
	for(i = 0; i < FPGA_MAX_LCD; i++)
		writeb(lcd_addr, i, 0);
}
int getOne(unsigned char *fpga_sw)
{
	int i, cnt = 0, in;
	for(i = 0; i < FPGA_MAX_SWITCH; i++)
		if(fpga_sw[i]) {
			cnt++;
			in = i;
		}

	if(cnt != 1)
		return -1;

	return in;
}

