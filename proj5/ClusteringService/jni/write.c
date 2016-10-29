#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "android/log.h"

#define LOG_TAG     "FND_LOG"
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#define FND_DEVICE  "/dev/fpga_fnd"

void JNICALL Java_com_example_clusteringservice_ClusteringService_writeFPGAFND(JNIEnv *env, jclass this, jint num)
{
    unsigned char data[4];

    int len = 4;
    while(len) {
        data[len-1] = '0' + num % 10;
        len--;
        num /= 10;
    }

    int dev = open(FND_DEVICE, O_RDWR);
    if (dev < 0) {
        LOGV("Device open error : %s\n",FND_DEVICE);
        return;
    }

    int retval = write(dev, &data, 4);  
    if(retval < 0) {
        LOGV("Write Error!\n");
        return;
    }   

    memset(data, 0, sizeof(data));

    sleep(1);

    retval = read(dev, &data, 4);
    if( retval < 0 ) {
        LOGV("Read Error!\n");
        return;
    }   

    close(dev);
}
