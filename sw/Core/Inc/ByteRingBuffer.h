#ifndef BYTE_RING_BUFFER_H
#define BYTE_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

struct ByteRingBuffer
{
    uint8_t* backing;
    uint16_t length;

    uint16_t writerIndex;
    uint16_t readerIndex;
    uint16_t size;
};

void byteRingBufferInit(struct ByteRingBuffer* buffer, uint8_t* backing, uint16_t length);

bool byteRingBufferAdd(struct ByteRingBuffer* buffer, uint8_t* source, uint16_t length);

bool byteRingBufferRemove(struct ByteRingBuffer* buffer, uint8_t* dest, uint16_t length);

#endif // RING_BUFFER_H
