#include "device.h"

void initOutputDevice(OutputDevice *outdev)
{
	initGPIO(outdev);
	initFPGA(outdev);
}
void offOutputDevice()
{

}
