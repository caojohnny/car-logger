#include <Serial.h>

#include <usb_device.h>
#include <stdio.h>
#include <usbd_cdc_if.h>
#include <Sleep.h>

// https://github.com/alexeykosinov/Redirect-printf-to-USB-VCP-on-STM32H7-MCU
int _write(int file, char *ptr, int len) {
	if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
		return 0;
	}

    while (CDC_Transmit_FS((uint8_t*) ptr, len))
    {
    	sleepMs(1);
    }

    return len;
}
