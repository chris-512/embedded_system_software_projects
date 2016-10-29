#ifndef _MACROS_H_
#define _MACROS_H_

// BOOLEAN MACROS
#ifndef true
   #define	true         1
#endif
#ifndef false
   #define	false        0
#endif

// The number of main's child processes
#define CHLD_PROC_NO    2
// The number of pipes to create
#define PIPE_NO         2
// The size of key event buffer
#define BUF_SIZE		64
// input_event size
#define EV_SIZE		sizeof(struct input_event)

// BUTTON MACROS
typedef enum ButtonType {
	SW_UP        	=	KEY_VOLUMEUP,
	SW_DOWN         =	KEY_VOLUMEDOWN,
	SW_PRESS        =	KEY_POWER,
	SW1             =	KEY_MENU,
	SW2             =	KEY_HOME,
	SW3             =	KEY_BACK,
	SW4             =	KEY_SEARCH
} ButtonType;

#define EVENTLOOP	while(1)

#endif

