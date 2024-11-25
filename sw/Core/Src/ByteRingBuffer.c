#include <ByteRingBuffer.h>

#include <string.h>

void byteRingBufferInit(struct ByteRingBuffer* buffer, uint8_t* backing, uint16_t length)
{
    buffer->backing = backing;
    buffer->length = length;

    buffer->writerIndex = 0;
    buffer->readerIndex = 0;
    buffer->size = 0;
}

bool byteRingBufferAdd(struct ByteRingBuffer* buffer, uint8_t* source, uint16_t length)
{
    uint16_t available = buffer->length - buffer->size;
    if (length > available)
    {
        return false;
    }

    buffer->size += length;

    uint16_t numToEnd = buffer->length - buffer->writerIndex;
    if (numToEnd >= length)
    {
        numToEnd = length;
        length = 0;
    }
    else if (numToEnd < length)
    {
        length -= numToEnd;
    }

    memcpy(buffer->backing + buffer->writerIndex, source, numToEnd);
    buffer->writerIndex += numToEnd;

    if (length > 0)
    {
        memcpy(buffer->backing + buffer->writerIndex, source + numToEnd, length);
        buffer->writerIndex = length;
    }

    return true;
}

bool byteRingBufferRemove(struct ByteRingBuffer* buffer, uint8_t* dest, uint16_t length)
{
    if (length > buffer->size)
    {
        return false;
    }

    buffer->size -= length;

    uint16_t numToEnd = buffer->length - buffer->readerIndex;
    if (numToEnd >= length)
    {
        numToEnd = length;
        length = 0;
    }
    else if (numToEnd < length)
    {
        length -= numToEnd;
    }

    memcpy(dest, buffer->backing + buffer->readerIndex, numToEnd);
    buffer->readerIndex += numToEnd;

    if (length > 0)
    {
        memcpy(dest + numToEnd, buffer->backing + buffer->readerIndex, length);
        buffer->readerIndex = length;
    }

    return true;
}
