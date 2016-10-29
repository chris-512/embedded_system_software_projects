app:
	arm-none-linux-gnueabi-gcc -static app_write.c -o app_write
	arm-none-linux-gnueabi-gcc -static app_ioctl.c -o app_ioctl
#	adb push app_write .
#	adb push app_ioctl .
clean:
	rm app_write app_ioctl
