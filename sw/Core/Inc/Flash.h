#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>
#include <stdint.h>

struct FlashInfo
{
	uint32_t writerAddress;
	uint32_t flashLength;
};

bool flashInit(struct FlashInfo* info, uint8_t* workingMemory, uint16_t minFreeBlockSz);

bool flashIsIdle(struct FlashInfo* info);

bool flashClearMemory(struct FlashInfo* info);

bool flashLogData(struct FlashInfo* info, uint8_t* src, uint16_t len);

bool flashReadData(struct FlashInfo* info, uint32_t address, uint8_t* dest, uint16_t len);

#endif // FLASH_H
