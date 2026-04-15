#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <ByteRingBuffer.h>

extern struct FlashInfo flashMemory;

#define SERIAL_RX_BUFFER_SZ (256U)
extern uint8_t serialRxBacking[SERIAL_RX_BUFFER_SZ];
extern struct ByteRingBuffer serialRx;

void initSerial();

void stepSerial();

void initDataCollection();

void stepDataCollection();

#endif // APP_H
