#include <Serial.h>

#include <stdio.h>
#include <usbd_cdc_if.h>

// https://github.com/alexeykosinov/Redirect-printf-to-USB-VCP-on-STM32H7-MCU
int _write(int file, char *ptr, int len) {
    static uint8_t rc = USBD_OK;

    do {
        rc = CDC_Transmit_FS((uint8_t*) ptr, len);
    } while (USBD_BUSY == rc);

    if (USBD_FAIL == rc) {
        return 0;
    }

    return len;
}
