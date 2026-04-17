#include <Serial.h>

#include <stdio.h>
#include <usbd_cdc_if.h>
#include <Sleep.h>

// https://github.com/alexeykosinov/Redirect-printf-to-USB-VCP-on-STM32H7-MCU
int _write(int file, char *ptr, int len) {
    while (CDC_Transmit_FS((uint8_t*) ptr, len))
    {
    	sleepMs(1);
    }

    return len;
}
