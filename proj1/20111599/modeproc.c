#include "setting.h"
#include "devices/device.h"
#include "debug.h"

#define GPIO_RELEASE		0
#define GPIO_PRESS			1
#define MAX_MODE 			5
#define DOT_MATRIX_HEIGHT	10
#define DOT_MATRIX_WIDTH	7

typedef enum { MMAP_CONTROL, DEVICE_CONTROL }	ControlType;
typedef enum { GPIO_INPUT, FPGA_INPUT } 		InputType;
typedef enum { GPIO_OUTPUT, FPGA_OUTPUT } 		OutputType;
// enum structure for write command
typedef enum 
{ 
	FPGA_CLEAR_FND,
	FPGA_WRITE_FND,

	FPGA_CLEAR_LCD,
	FPGA_WRITE_LCD,
	FPGA_WRITE_LCD_BYTE,

	FPGA_PRINT_CHR_DOT,
	FPGA_PRINT_NUM_DOT,
	FPGA_WRITE_DOT_MATRIX,

	GPIO_CLEAR_FND,
	GPIO_WRITE_FND,

	GPIO_ON_LED,
	GPIO_OFF_LED,

	TURN_OFF_GPIO,
	TURN_OFF_FPGA,
	INIT_CLOCK_MODE,
	INIT_COUNTER_MODE,
	INIT_DRAW_BOARD_MODE,
	INIT_TEXT_EDITOR_MODE
} WriteCommandType;

typedef struct Point
{
	int x;
	int y;
} Point;

typedef struct inputMesg
{
	InputType 		type;
	struct			input_event gpio_in;
	unsigned char   fpga_in[FPGA_MAX_SWITCH];
} inputMesg;

typedef struct outputMesg
{
	OutputType type;
	WriteCommandType command;
	char toWrite;
	int loc;
	int val;
	int fnd_val[4];
	unsigned char dat;
	unsigned char drawboard[10][7];
} outputMesg;

int child_pid[2];

/**
 * Main process's function to read messages from its input process
 *
 * @param 	int **ipc_pipe	int array for pipes
 * @return	inputMesg		received message from its input process
 */
inputMesg ipc_inread(int **ipc_pipe)
{
	static inputMesg inmsg;

	read(ipc_pipe[IN2MAIN][INPUT_STREAM], &inmsg, sizeof(inputMesg));

	return inmsg;
}
/**
 * Main process's function to write messages to its output process
 *
 * @param 	int **ipc_pipe		int array for pipes
 * @param	WriteCommandType	command type
 * @param 	outputMesg out		message to send to its output process
 * @return	inputMesg			received input message
 */
void ipc_outwrite(int **ipc_pipe, WriteCommandType command, outputMesg out)
{
	out.type = command;
	write(ipc_pipe[MAIN2OUT][OUTPUT_STREAM], &out, sizeof(outputMesg));
}
// GPIO Procedures
/**
 * MODE 1: Clock
 *
 * @param 	int **ipc_pipe	int array for pipes
 * @return	ModeType		what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
ModeType clockProc(int **ipc_pipe) 
{
	inputMesg 	in;
	outputMesg 	out;

	// Turn off FPGA
	ipc_outwrite(ipc_pipe, TURN_OFF_FPGA, out);
	/*
	   INIT_CLOCK_MODE

	   1) Write an initial board time to FND
	   2) Turn on the No.1 LED
	     - Use the time signal to clock the fnd time.
	*/
	ipc_outwrite(ipc_pipe, INIT_CLOCK_MODE, out);

	int i, ev;

	// 0: non change mode, 1: change mode
	int ClockChangeMode = 0;
	
	time_t t;
	struct tm *l;
	time_t prev, cur;
	int saveFlag = 0;
	int LEDToggle = 0;
	int time_val[4];
	int saved_time[4];
	time_t saved_t;

	// GET THE CURRENT TIME
	prev = time(NULL);
	
	// add 60 * 60 * 9 to plus 9 hours
	t = prev + 60 * 60 * 9;
	l = localtime(&t);
	// place the local time at time array
	time_val[0] = l->tm_hour / 10;
	time_val[1] = l->tm_hour % 10;
	time_val[2] = l->tm_min / 10;
	time_val[3] = l->tm_min % 10;

	EVENTLOOP {
		in = ipc_inread(ipc_pipe);
		// Only see the GPIO_INPUT
		if(in.type == GPIO_INPUT) 
		{
			ModeType gpioButtonType = in.gpio_in.code;
			if(in.gpio_in.value == GPIO_RELEASE)
				continue;
			switch(gpioButtonType)
			{
			case SW1: // Clock Change Mode
				ClockChangeMode = !ClockChangeMode;
				if(ClockChangeMode)
				{
					// In change mode, save the current time ( to stop the clock )
					// so that when I modify the time using SW2, SW3, SW4 key, we can use saved_t variable.
					saved_t = time(NULL) + 60 * 60 * 9;
					l = localtime(&saved_t);
					saved_time[0] = l->tm_hour / 10;
					saved_time[1] = l->tm_hour % 10;
					saved_time[2] = l->tm_min / 10;
					saved_time[3] = l->tm_min % 10;

					// Turn On LED 3 and 4 at the same time
					out.dat = 0x30;
					ipc_outwrite(ipc_pipe, GPIO_ON_LED, out);
					usleep(5000);

					saveFlag = 0;
				} 
				else 
				{
					// Turn On LED 1 when it is not in clock change mode.
					out.dat = 0xE0;
					ipc_outwrite(ipc_pipe, GPIO_ON_LED, out);

					saveFlag = 1;
				}
				break;
			// MODE CHANGE:	FORWARD
			case SW_UP:	return MODE_NEXT;
			// MODE CHANGE: BACKWARD
			case SW_DOWN:	return MODE_PREV;
			// MODE TERMINATED
			case SW_PRESS:	return MODE_RETURN;
			case SW2:
			case SW3:
			case SW4:
				// SW2, SW3, SW3 only works in clock change mode
				if(ClockChangeMode)
				{
					switch(gpioButtonType)
					{
					case SW2: // Reset the displayed time to current board time
						saved_t = time(NULL) + 60 * 60 * 9;
						break;
					case SW3: // Increase 1 Hour
						saved_t += 60 * 60;
						break;
					case SW4: // Increase 1 Minute
						saved_t += 60;
						break;
					}
					
					// Reflect the changes made
					l = localtime(&saved_t);
					saved_time[0] = l->tm_hour / 10;
					saved_time[1] = l->tm_hour % 10;
					saved_time[2] = l->tm_min / 10;
					saved_time[3] = l->tm_min % 10;
				}
				break;
			default:	break;
			}
		}
		cur = time(NULL);
		// Below happens per second
		if(!saveFlag && cur - prev >= 1)  // cur - prev = diff >= 1
		{
			t = time(NULL) + 60 * 60 * 9;
			l = localtime(&t);

			time_val[0] = l->tm_hour / 10;
			time_val[1] = l->tm_hour % 10;
			time_val[2] = l->tm_min / 10;
			time_val[3] = l->tm_min % 10;

			// In clock change mode, the LED 3 and 4 turns on interchangably
			if(ClockChangeMode)  
			{
				static unsigned char led[2] = {0xB0, 0x70};

				out.dat = led[LEDToggle];
				ipc_outwrite(ipc_pipe, GPIO_ON_LED, out);
				LEDToggle = !LEDToggle;
				// LED Toggle 0 -> 1 -> 0 -> 1 -> 0 -> ...
			}

			prev = cur;
		}
		// If the user reflects the clock change, then savedFlag = 1.
		if(saveFlag && cur - prev >= 1) 
		{
			// For each second, saved_t++
			t = saved_t++;
			l = localtime(&t);

			saved_time[0] = l->tm_hour / 10;
			saved_time[1] = l->tm_hour % 10;
			saved_time[2] = l->tm_min / 10;
			saved_time[3] = l->tm_min % 10;

			if(ClockChangeMode) 
			{
				static unsigned char led[2] = {0xB0, 0x70};

				out.dat = led[LEDToggle];
				ipc_outwrite(ipc_pipe, GPIO_ON_LED, out);
				LEDToggle = !LEDToggle;
			}

			prev = cur;
		}

		// No save action performed and not in change mode
		if(!saveFlag && !ClockChangeMode) {
			// Write the current time to FND
			for(i = 0; i < 4; i++)
				out.fnd_val[i] = time_val[i];
			ipc_outwrite(ipc_pipe, GPIO_WRITE_FND, out);	
		} 
		// (Save action performed at least once before and not in change mode)
		// or (in change mode)
		else if((saveFlag && !ClockChangeMode) || ClockChangeMode) {
			// Write the saved time to FND
			for(i = 0; i < 4; i++)
				out.fnd_val[i] = saved_time[i];
			ipc_outwrite(ipc_pipe, GPIO_WRITE_FND, out);
		}
	}
}
/**
 * MODE 2: Counter
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return ModeType       	what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
ModeType counterProc(int **ipc_pipe)
{
	inputMesg in;
	outputMesg out;

	int count = 0;
	int count_ary[4] = {0, };
	int led[4] = {0xD0, 0xB0, 0x70, 0xE0};
	int radix_mode[4] = {10, 8, 16, 2};
	int mode = 0, i, digit;
	int temp, temp1, temp2, temp3;
	
	// turn off the fpga
	ipc_outwrite(ipc_pipe, TURN_OFF_FPGA, out);
	// initialize the counter mode
	ipc_outwrite(ipc_pipe, INIT_COUNTER_MODE, out);

	EVENTLOOP {
		in = ipc_inread(ipc_pipe);
		// Only see the GPIO_INPUT
		if(in.type == GPIO_INPUT)
		{
			ModeType gpioButtonType = in.gpio_in.code;
			if(in.gpio_in.value == GPIO_RELEASE)
				continue;
			switch(gpioButtonType)
			{
			case SW1:
				// change the radix mode
				mode = (mode + 1) % 4;
				out.dat = led[mode];
				ipc_outwrite(ipc_pipe, GPIO_ON_LED, out);

				if(mode == 0 && count >= 1000)
					count %= 1000;

				// Change a number temp according to changed radix
				// and store the changed number to count_ary
				temp = count;
				for(i = 3; i >= 0; i--) {
					if(temp)
						count_ary[i] = temp % radix_mode[mode];
					else
						count_ary[i] = 0;
					temp /= radix_mode[mode];
				}
				break;
			// MODE CHANGE:	FORWARD
			case SW_UP:		return MODE_NEXT;
			// MODE CHANGE: BACKWARD
			case SW_DOWN:	return MODE_PREV;
			// MODE TERMINATED
			case SW_PRESS:	return MODE_RETURN;
			case SW2: // 100
			case SW3: // 10
			case SW4: // 1
				// increase the particular digit by 1
				switch(gpioButtonType)
				{
				case SW2: digit = 2; // increase 3rd digit by 1
					break;
				case SW3: digit = 1; // increase 2nd digit by 1
					break;
				case SW4: digit = 0; // increase 1st digit by 1
					break;
				}
				

				temp3 = pow(radix_mode[mode], digit);
				temp1 = count / temp3;
				temp2 = count % temp3;
				temp1 += 1;

				// reassemble
				count = temp1 * temp3 + temp2;
				if(mode == 0 && count >= 1000)
					count %= 1000;

				// Change a number temp according to changed radix
				// and store the changed number to count_ary
				temp = count;
				for(i = 3; i >= 0; i--) {
					if(temp)
						count_ary[i] = temp % radix_mode[mode];
					else
						count_ary[i] = 0;
					temp /= radix_mode[mode];
				}

				break;
			default:	break;
			}
		}
		
		for(i = 0; i < 4; i++)
			out.fnd_val[i] = count_ary[i];
		ipc_outwrite(ipc_pipe, GPIO_WRITE_FND, out);
	}
}
/**
 * MODE 3: Text Editor
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return ModeType       	what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
// FPGA Procedures
ModeType textEditorProc(int **ipc_pipe) 
{
	inputMesg 	in;
	outputMesg 	out;
	unsigned char   fpga_sw[FPGA_MAX_SWITCH];

	int fnd_number = 0;
	char prev = -1, cur;
	int loc = 0, next = 0;
	int pressedCnt = 0;
	// 0 : Character, 1 : Number
	int modeToggle = 0;
	int OneKeyFlag = 0;
	int TwoKeyFlag = 0;
	char inp;

	// turn off the gpio
	ipc_outwrite(ipc_pipe, TURN_OFF_GPIO, out);
	// initialize the text editor mode
	ipc_outwrite(ipc_pipe, INIT_TEXT_EDITOR_MODE, out);

	int i;
	EVENTLOOP {
		in = ipc_inread(ipc_pipe);
		if(in.type == GPIO_INPUT) 
		{
			if(in.gpio_in.value == GPIO_RELEASE)
				continue;
			switch( in.gpio_in.code ) 
			{
			// MODE CHANGE: FORWARD
			case SW_UP:	return MODE_NEXT;
			// MODE CHANGE: BACKWARD
			case SW_DOWN:	return MODE_PREV;
			// MODE TERMINATED
			case SW_PRESS:  return MODE_RETURN;
			default:	break;
			}
		} 
		else if(in.type == FPGA_INPUT)
		{
			memcpy(fpga_sw, in.fpga_in, FPGA_MAX_SWITCH);

			if(TwoKeyPressed(fpga_sw) && !TwoKeyFlag) {
				TwoKeyFlag = 1;
				// Press 2 and 3: text lcd clear
				if(fpga_sw[1] && fpga_sw[2]) {
					ipc_outwrite(ipc_pipe, FPGA_CLEAR_LCD, out);
					loc = 0;
				}
				// Press 5 and 6: switch between number mode and character mode
				else if(fpga_sw[4] && fpga_sw[5]) {
					modeToggle = !modeToggle;
					if(modeToggle)
						ipc_outwrite(ipc_pipe, FPGA_PRINT_NUM_DOT, out);
					else
						ipc_outwrite(ipc_pipe, FPGA_PRINT_CHR_DOT, out);
				} 
				// Press 8 and 9: print the space onto the lcd
				else if(fpga_sw[7] && fpga_sw[8]) {
					out.loc = loc;
					out.toWrite = ' ';
					ipc_outwrite(ipc_pipe, FPGA_WRITE_LCD, out);
					if(0 <= loc && loc < 8)
						loc++;
				}

				// pressed button twice, so increase by 2
				fnd_number = (fnd_number + 2) % 10000;

				out.fnd_number = fnd_number;
				ipc_outwrite(ipc_pipe, FPGA_WRITE_FND, out);
			}
			else if(OneKeyPressed(fpga_sw) && !OneKeyFlag) {
				OneKeyFlag = 1;

				// only allow 1 button press
				if((inp = getOne(fpga_sw)) < 0)
					continue;

				if(modeToggle) { // number
					out.loc = loc;
					out.toWrite = '1' + inp;
					ipc_outwrite(ipc_pipe, FPGA_WRITE_LCD, out);
					if(loc < 8) loc++;
				} else { // character
					cur = inp;
					// if you press the button again
					if( prev != -1 && prev == cur ) {
						next = (next + 1) % 3;
						out.loc = loc-1;
						out.toWrite = switch_alphabet[inp][next];
						ipc_outwrite(ipc_pipe, FPGA_WRITE_LCD, out);
					} else {
						out.loc = loc;
						out.toWrite = switch_alphabet[inp][0];
						ipc_outwrite(ipc_pipe, FPGA_WRITE_LCD, out);
						if(loc < 8) loc++;
						next = 0;
					}
					prev = cur;
				}

				// pressed button once, so increase by 1
				fnd_number = (fnd_number + 1) % 10000;
				out.fnd_number = fnd_number;
				ipc_outwrite(ipc_pipe, FPGA_WRITE_FND, out);
			}
			else if(KeyReleased(fpga_sw) && (OneKeyFlag || TwoKeyFlag))
			{
				OneKeyFlag = TwoKeyFlag = 0;
			}
		}
	}

}

// clear the board info
void clearDrawBoard(char drawboard[][DOT_MATRIX_WIDTH])
{
	int i, j;
	for(i = 0; i < DOT_MATRIX_HEIGHT; i++)
		for(j = 0; j < DOT_MATRIX_WIDTH; j++)
			drawboard[i][j] = 0;
}

/**
 * MODE 4: Drawboard
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return ModeType       	what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
ModeType drawBoardProc(int **ipc_pipe)
{
	inputMesg 	in;
	outputMesg 	out;
	unsigned char   fpga_sw[FPGA_MAX_SWITCH];

	int fnd_number = 0;

	int OneKeyFlag = 0;
	char inp;
	int i, j;
	int y, x;
	int DotShowToggle = 1;
	int preserveFlag = 0;
	int cursorToggle = 0;
	time_t prev, cur;

	y = x = 0;

	// initialize
	clearDrawBoard(out.drawboard);

	// turn off gpio
	ipc_outwrite(ipc_pipe, TURN_OFF_GPIO, out);
	// initialize the draw board mode
	ipc_outwrite(ipc_pipe, INIT_DRAW_BOARD_MODE, out);

	prev = time(NULL);

	EVENTLOOP {
		in = ipc_inread(ipc_pipe);
		if(in.type == GPIO_INPUT)
		{
			if(in.gpio_in.value == GPIO_RELEASE)
				continue;
			switch( in.gpio_in.code ) 
			{
			// MODE CHANGE: FORWARD
			case SW_UP:	return MODE_NEXT;
			// MODE CHANGE: BACKWARD
			case SW_DOWN:	return MODE_PREV;
			// MODE TERMINATED
			case SW_PRESS:  return MODE_RETURN;
			default:	break;
			}
		} 
		else if(in.type == FPGA_INPUT) 
		{
			memcpy(fpga_sw, in.fpga_in, FPGA_MAX_SWITCH);

			// If I pressed once
			if(OneKeyPressed(fpga_sw) && !OneKeyFlag) 
			{
				OneKeyFlag = 1;

				if((inp = getOne(fpga_sw)) < 0)
					continue;

				switch(inp)
				{
				case 0:

					clearDrawBoard(out.drawboard);
					ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);

					y = x = 0;
					DotShowToggle = 1;
					break;
				case 1: // UP
					if(0 < y) y--;
					break;
				case 2: // SHOW/HIDE CURSOR
					DotShowToggle = !DotShowToggle;
					break;
				case 3: // LEFT
					if(0 < x) x--;
					break;
				case 4: // DRAW
					out.drawboard[y][x] = 1;
					break;
				case 5: // RIGHT
					if(x < DOT_MATRIX_WIDTH-1) x++;
					break;
				case 6: // CLEAR PICTURE
					clearDrawBoard(out.drawboard);
					ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);
					break;
				case 7: // DOWN
					if(y < DOT_MATRIX_HEIGHT-1) y++;
					break;
				case 8: // REVERSE
					for(i = 0; i < DOT_MATRIX_HEIGHT; i++)
						for(j = 0; j < DOT_MATRIX_WIDTH; j++)
						{
							if(out.drawboard[i][j])
								out.drawboard[i][j] = 0;
							else
								out.drawboard[i][j] = 1;
						}
					ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);
					break;
				}

				// Since I pressed the button once, increase the fnd number by 1
				fnd_number = (fnd_number + 1) % 10000;
				out.fnd_number = fnd_number;
				ipc_outwrite(ipc_pipe, FPGA_WRITE_FND, out);
			}
			else if(KeyReleased(fpga_sw) && OneKeyFlag)
			{
				OneKeyFlag = 0;
			}
		}
		cur = time(NULL);
		if(cur - prev >= 1) {
			// if some pixel already existed at (y, x)
			preserveFlag = out.drawboard[y][x];
			
			if(DotShowToggle)
				out.drawboard[y][x] = cursorToggle;

			ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);

			// preserve the original value
			out.drawboard[y][x] = preserveFlag;

			cursorToggle = !cursorToggle;
			prev = cur;
		}
	}

}



/* For dijkstra game */
Point a[100];
Point sol[100];

int c[10][7];
int n = 0;
int soln = 0;
int endy, endx;

int dy[] = {0, -1, 0, 1};
int dx[] = {-1, 0, 1, 0};

// return whether a is closer than b
int closer(Point a, Point b)
{
	// distance measure using manhatten distance
	int dist1 = abs(a.x-endx) + abs(a.y-endy);
	int dist2 = abs(b.x-endx) + abs(b.y-endy);

	return (dist1 < dist2);
}
// store points for solution path
void solpush(Point p)
{
	sol[soln++] = p;
}
// check if the heap is empty
int empty()
{
	return (n == 0);
}
// print the heap
void printheap()
{
	int i;

	// To see the current heap structure
	printf("Heap: ");
	for(i = 1; i <= n; i++)
		printf("[%d]", abs(endx-a[i].x)+abs(endy-a[i].y));
	printf("\n");
}
// get the heap top
Point top()
{
	return a[1];
}
// heap pop operation
Point pop()
{
	Point temp = a[1];
	a[1] = a[n--];
	Point root = a[1];
	int child = 2;
	// root(a[1]) goes down
	while(child <= n) {
		if(child < n && closer(a[child+1], a[child]))
				child++;
		if(closer(root, a[child])) break;
		else {
			a[child / 2] = a[child];
			child *= 2;
		}
	}
	a[child/2] = root;
	return temp;
}
// heap push operation
void push(Point item)
{
	a[++n] = item;
	int i = n;
	while((i != 1) && (closer(item, a[i/2]))) {
		a[i] = a[i/2];
		i /= 2;
	}
	a[i] = item;
}
// delay for t iterations
void delay(int t)
{
	while(t--);
}
// display string 'str' on text lcd. (max size: 32)
void displayLCD(int **ipc_pipe, const char *str)
{
	outputMesg 		out;

	if(strlen(str) > 32) return;

	int i;
	for(i = 0; i < 32; i++) 
	{
		out.loc = i;
		if(i < strlen(str))
			out.toWrite = str[i];
		else
			out.toWrite = 0;
		ipc_outwrite(ipc_pipe, FPGA_WRITE_LCD_BYTE, out);
	}
}
/**
 * MODE 5: Dijkstra
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return ModeType       	what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
ModeType dijkstraGhost(int **ipc_pipe)
{
	inputMesg 		in;
	outputMesg 		out;
	unsigned char   fpga_sw[FPGA_MAX_SWITCH];

	char inp;
	int i, j;

	int starty, startx;
	int y, x;

	// 0 : insert mode 1: change start position 2: change end position
	int mode 				= 0;

	int fnd_number 			= 0;
	int OneKeyFlag 			= 0;
	int preserveFlag 		= 0;
	int cursorToggle		= 0;
	time_t prev, cur;

	// Dijkstra
	int solved				= 0; // flag to show whether there is a solution
	Point p, now, next;

	// x and y set to 0. The point starts at (0, 0).
	y = x = 0;

	// starty, startx, endy, endx not set
	starty = startx = endy = endx = -1;
	
	for(i = 0; i < DOT_MATRIX_HEIGHT; i++)
		for(j = 0; j < DOT_MATRIX_WIDTH; j++)
			c[i][j] = 0;

	// initialize the board
	clearDrawBoard(out.drawboard);

	ipc_outwrite(ipc_pipe, TURN_OFF_GPIO, out);
	ipc_outwrite(ipc_pipe, INIT_DRAW_BOARD_MODE, out);

	prev = time(NULL);

	displayLCD(ipc_pipe, "CURRENT MODE    " "1. SET BLOCK POS");

	EVENTLOOP {
		in = ipc_inread(ipc_pipe);
		if(in.type == GPIO_INPUT)
		{
			if(in.gpio_in.value == GPIO_RELEASE)
				continue;
			switch( in.gpio_in.code ) 
			{
			// MODE CHANGE: FORWARD
			case SW_UP:	return MODE_NEXT;
			// MODE CHANGE: BACKWARD
			case SW_DOWN:	return MODE_PREV;
			// MODE TERMINATED
			case SW_PRESS:  return MODE_RETURN;
			default:	break;
			}
		} 
		else if(in.type == FPGA_INPUT) 
		{
			memcpy(fpga_sw, in.fpga_in, FPGA_MAX_SWITCH);

			if(OneKeyPressed(fpga_sw) && !OneKeyFlag) 
			{
				OneKeyFlag = 1;

				if((inp = getOne(fpga_sw)) < 0)
					continue;

				switch(inp)
				{
				case 0:
					clearDrawBoard(out.drawboard);
					ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);
					mode = y = x = 0;
					startx = starty = endx = endy = -1;
					for(i = 0; i < DOT_MATRIX_HEIGHT; i++)
						for(j = 0; j < DOT_MATRIX_WIDTH; j++)
							c[i][j] = 0;
					displayLCD(ipc_pipe, "CURRENT MODE    " "1. SET BLOCK POS");
					break;
				case 1: // POINT GO UP
					if(0 < y) y--;
					break;
				case 2: // CHANGE MODE
					mode = (mode + 1) % 3;
					if(mode == 0)
						displayLCD(ipc_pipe, "CURRENT MODE   " " 1. SET BLOCK POS");
					else if(mode == 1)
						displayLCD(ipc_pipe, "CURRENT MODE   " " 2. SET START POS");
					else if (mode == 2)
						displayLCD(ipc_pipe, "CURRENT MODE   " " 3. SET END POS  ");
					break;
				case 3: // POINT GO LEFT
					if(0 < x) x--;
					break;
				case 4: // DRAW THE POINT
					switch(mode)
					{
					case 0:	
						out.drawboard[y][x] = 1;
						break;
					case 1:
						startx = x;
						starty = y;
						printf("start: (%d %d)\n", startx, starty);
						break;
					case 2:
						endx = x;
						endy = y;
						printf("end: (%d %d)\n", endx, endy);
						break;
					}

					break;
				case 5: // RIGHT
					if(x < DOT_MATRIX_WIDTH-1) x++;
					break;
				case 6: 
					break;
				case 7: // DOWN
					if(y < DOT_MATRIX_HEIGHT-1) y++;
					break;
				case 8: // START DIJKSTRA (A* ALGORITHM)
					if(startx == -1 && starty == -1 && endx == -1 && endy == -1) {
						displayLCD(ipc_pipe, "NEED START & END");
						continue;
					}
					// Solve the dijkstra

					displayLCD(ipc_pipe, "GO DIJKSTRA....!");

					// initialize the dijkstra
					p.x = startx;
					p.y = starty;
					c[p.y][p.x] = 1;
					soln = n = 0;
					push(p);

					while(!empty()) {
						now.x = top().x;
						now.y = top().y;
						pop();

						solpush(now);

						if(now.x == endx && now.y == endy) {
							solved = 1;
							break;
						}

						for(i = 0; i < 4; i++) {
							next.x = now.x + dx[i];
							next.y = now.y + dy[i];
							if(0 <= next.x && next.x < DOT_MATRIX_WIDTH && 0 <= next.y && next.y < DOT_MATRIX_HEIGHT && !out.drawboard[next.y][next.x] && !c[next.y][next.x]) {
								push(next);
								c[next.y][next.x] = 1;
							}
						}
						printheap();
					}
					if(solved) {
						for(i = 0; i < soln; i++) {
							p.y = sol[i].y;
							p.x = sol[i].x;
							out.drawboard[p.y][p.x] = 1;
							ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);
							usleep(100000);
						}
						solved = 0;
					} else {
						// NO SOLUTION FOUND
						displayLCD(ipc_pipe, "NO PATH FOUND..!");
					}
					startx = starty = endx = endy = -1;
					break;
				}

				fnd_number		= (fnd_number + 1) % 10000;
				out.fnd_number 	= fnd_number;
				ipc_outwrite(ipc_pipe, FPGA_WRITE_FND, out);
			}
			else if(KeyReleased(fpga_sw) && OneKeyFlag)
			{
				OneKeyFlag = 0;
			}
		}
		cur = time(NULL);
		if(cur - prev >= 1) {
			// if some pixel already existed at (y, x)
			preserveFlag = out.drawboard[y][x];
			
			out.drawboard[y][x] = cursorToggle;

			ipc_outwrite(ipc_pipe, FPGA_WRITE_DOT_MATRIX, out);

			// preserve the original value
			out.drawboard[y][x] = preserveFlag;

			cursorToggle = !cursorToggle;
			prev = cur;
		}
	}

}
/**
 * Return a call function for each mode using function pointer
 *
 * @param  int mode			The variable that indexes a function pointer for each mode
 * @param  int **ipc_pipe	int array for pipes
 * @return ModeType			what action to take when SW_UP, SW_DOWN or SW_PRESS
 */
static ModeType runProc(int mode, int **ipc_pipe)
{
        static wrapperFunc callProc[] = { clockProc, counterProc, textEditorProc, drawBoardProc, dijkstraGhost};
        return callProc[mode](ipc_pipe);
}
/**
 * Main process
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return void
 */
void mainProcLoop(int **ipc_pipe)
{	
	int loop = 1;
	int exit = 0;

	close(ipc_pipe[IN2MAIN][OUTPUT_STREAM]);
	close(ipc_pipe[MAIN2OUT][INPUT_STREAM]);

	DEBUG("Main Process pid: %d", getpid());

	while(exit) {
		printf("Mode: %d\n", mode);
		// mode change according to the input from SW_UP, SW_DOWN, and SW_PRESS
		ModeType type = runProc(mode, ipc_pipe);
		switch(type)
		{
		case MODE_NEXT:
			mode = (mode + 1) % MAX_MODE;
			break;
		case MODE_PREV:
			mode = (mode == 0)? MAX_MODE - 1 : mode - 1;
			break;
		case MODE_RETURN:
			loop = 0;
			break;
		}
	}

	// To prevent child processes from being zombies.
	// Kill the input process
	kill(child_pid[INPUT_PROCESS], SIGKILL);
	// Kill the output process
	kill(child_pid[OUTPUT_PROCESS], SIGKILL);
	// Wait until child processes die.
	wait(NULL);
}
/**
 * Input process
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return void
 */
void inputProcLoop(int **ipc_pipe)
{
	inputMesg	inmsg;
	static int	gpiofd, fpgafd, rd;

	close(ipc_pipe[IN2MAIN][INPUT_STREAM]);
	close(ipc_pipe[MAIN2OUT][INPUT_STREAM]);
	close(ipc_pipe[MAIN2OUT][OUTPUT_STREAM]);

	if((gpiofd = open(GPIO_INPUT_DEVICE, O_RDONLY|O_NONBLOCK)) == -1) {
		printf("%s is not a valid device.\n", GPIO_INPUT_DEVICE);
	}
	if((fpgafd = open(FPGA_SWITCH_DEVICE, O_RDONLY|O_NONBLOCK)) == -1) {
		printf("%s is not a valid device.\n", FPGA_SWITCH_DEVICE);
	}

	child_pid[INPUT_PROCESS] = getpid();

	DEBUG("Input Process: %d\n", getpid());

	EVENTLOOP {
		usleep(100);
		// GPIO input
		rd = read(gpiofd, &inmsg.gpio_in, EV_SIZE);
		if(rd == EV_SIZE) {
			inmsg.type = GPIO_INPUT;
			write(ipc_pipe[IN2MAIN][OUTPUT_STREAM], &inmsg, sizeof(inputMesg));
		}
		// FPGA input
		rd = read(fpgafd, &inmsg.fpga_in, FPGA_MAX_SWITCH);
		if(rd == FPGA_MAX_SWITCH) {
			inmsg.type = FPGA_INPUT;
			write(ipc_pipe[IN2MAIN][OUTPUT_STREAM], &inmsg, sizeof(inputMesg));
		}

	}	
}
/**
 * Output process
 *
 * @param  int **ipc_pipe	int array for pipes
 * @return void
 */
void outputProcLoop(int **ipc_pipe)
{
	static struct 	input_event ev[BUF_SIZE];

	close(ipc_pipe[MAIN2OUT][OUTPUT_STREAM]);
	close(ipc_pipe[IN2MAIN][INPUT_STREAM]);
	close(ipc_pipe[IN2MAIN][OUTPUT_STREAM]);

	child_pid[OUTPUT_PROCESS] = getpid();

	DEBUG("Output Process: %d\n", getpid());

	OutputDevice *outdev = (OutputDevice *)malloc(sizeof(OutputDevice));
	initOutputDevice(outdev);

	outputMesg outmsg;

	int rd, FNDWrite = 0, i;

	EVENTLOOP {
		// read message sent from main process
		rd = read(ipc_pipe[MAIN2OUT][INPUT_STREAM], &outmsg, sizeof(outmsg));
		if(rd == sizeof(outmsg)) {
			switch( outmsg.type )
			{
			// Initialize Each Mode
			case TURN_OFF_GPIO:
#ifdef OUTPROC_DEBUG
				printf("TURN_OFF_GPIO\n");
#endif
				turnOffGPIO(outdev);
				break;
			case TURN_OFF_FPGA:
#ifdef OUTPROC_DEBUG
				printf("TURN_OFF_FPGA\n");
#endif
				turnOffFPGA(outdev);
				break;
			case INIT_CLOCK_MODE:
#ifdef OUTPROC_DEBUG
				printf("INIT_CLOCK_MODE\n");
#endif
				initDeviceClock(outdev);
				break;
			case INIT_COUNTER_MODE:
#ifdef OUTPROC_DEBUG
				printf("INIT_COUNTER_MODE\n");
#endif
				initDeviceCounter(outdev);
				break;
			case INIT_TEXT_EDITOR_MODE:
#ifdef OUTPROC_DEBUG
				printf("INIT_TEXT_EDITOR\n");
#endif
				initDeviceTextEditor(outdev);
				break;
			case INIT_DRAW_BOARD_MODE:
#ifdef OUTPROC_DEBUG
				printf("INIT_DRAW_BOARD_MODE\n");
#endif
				initDeviceDrawBoard(outdev);
				break;

			// GPIO Write Operation
			case GPIO_CLEAR_FND:
#ifdef OUTPROC_DEBUG
				printf("GPIO_CLEAR_FND\n");
#endif
				clearGPIOFND(outdev);
				break;
			case GPIO_WRITE_FND:
#ifdef OUTPROC_DEBUG
				printf("GPIO_WRITE_FND\n");
#endif
				FNDWrite = 1;
				//writeGPIOFND(outdev, outmsg.loc, outmsg.val);
				break;
			case GPIO_OFF_LED:
#ifdef OUTPROC_DEBUG
				printf("GPIO_OFF_LED\n");
#endif
				offGPIOLED(outdev->gpio.led_fd);
				break;
			case GPIO_ON_LED:
#ifdef OUTPROC_DEBUG
				printf("GPIO_ON_LED\n");
#endif
				onGPIOLED(outdev->gpio.led_fd, outmsg.dat);
				break;

			// FPGA Write Operation
			case FPGA_CLEAR_FND:
#ifdef OUTPROC_DEBUG
				printf("FPGA_CLEAR_FND\n");
#endif
				clearFPGAFND(outdev->fpga.fnd_fd);
				break;
			case FPGA_WRITE_FND:
#ifdef OUTPROC_DEBUG
				printf("FPGA_WRITE_FND\n");
#endif
				writeFPGAFND(outdev->fpga.fnd_fd, outmsg.fnd_number);
				break;
			case FPGA_CLEAR_LCD:
#ifdef OUTPROC_DEBUG
				printf("FPGA_CLEAR_LCD\n");
#endif
				clearFPGALCD(outdev->fpga.lcd_addr);
				break;
			case FPGA_WRITE_LCD:
#ifdef OUTPROC_DEBUG
				printf("FPGA_WRITE_LCD\n");
#endif
				writeFPGALCD(outdev->fpga.lcd_addr, outmsg.loc, outmsg.toWrite);
				break;
			case FPGA_WRITE_LCD_BYTE:
#ifdef OUTPROC_DEBUG
				printf("FPGA_WRITE_LCD_BYTE\n");
#endif
				writeb(outdev->fpga.lcd_addr, outmsg.loc, outmsg.toWrite);
				break;
			case FPGA_PRINT_CHR_DOT:
#ifdef OUTPROC_DEBUG
				printf("FPGA_PRINT_CHR_DOT\n");
#endif
				writeFPGACharDotMatrix(outdev->fpga.dot_fd);
				break;
			case FPGA_PRINT_NUM_DOT:
#ifdef OUTPROC_DEBUG
				printf("FPGA_PRINT_NUM_DOT\n");
#endif
				writeFPGANumDotMatrix(outdev->fpga.dot_fd);
				break;
			case FPGA_WRITE_DOT_MATRIX:
#ifdef OUTPROC_DEBUG
				printf("FPGA_WRITE_DOT_MATRIX\n");
#endif
				writeFPGADotMatrix(outdev->fpga.dot_fd, outmsg.drawboard);
			default:
				break;
			}
		}

		// Write FND based on fnd_val array
		if(FNDWrite) {
			// When FNDWrite = 1, start displaying it.
			for(i = 1; i <= 4; i++) {
				// Print fnd_val[i-1] digit at i position
				writeGPIOFND(outdev, i, outmsg.fnd_val[i-1]);
				delay(1000); // give a delay
			}
			FNDWrite = 0;
		}
	}
}
