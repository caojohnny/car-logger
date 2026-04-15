#include <Flash.h>

#include <main.h>
#include <assert.h>

#define SPI_TIMEOUT_TICKS (1000U)
#define PAGE_SZ (256U)
#define FLASH_SZ (256U * 1024U * 1024U);

static void flashWriteEnable()
{
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

	uint8_t command[2] = {0x06};
	HAL_SPI_Transmit(&hspi1, command, 1, SPI_TIMEOUT_TICKS);

	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

bool flashInit(struct FlashInfo* info)
{
	// TODO
	info->writerAddress = 0U;

	info->flashLength = FLASH_SZ;

	return true;
}

bool flashIsIdle(struct FlashInfo* info)
{
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

	uint8_t command[2] = {0x05};
	HAL_SPI_Transmit(&hspi1, command, 1, SPI_TIMEOUT_TICKS);

	uint8_t statusRegister1;
	HAL_SPI_Receive(&hspi1, &statusRegister1, 1, SPI_TIMEOUT_TICKS);

	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);


	return (statusRegister1 & 0x01) == 0;
}

static void flashWaitUntilIdle(struct FlashInfo* info)
{
	while (!flashIsIdle(info))
	{
	}
}

bool flashClearMemory(struct FlashInfo* info)
{
	flashWriteEnable();

	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

	uint8_t command[1] = {0x60};
	HAL_SPI_Transmit(&hspi1, command, 1, SPI_TIMEOUT_TICKS);

	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

	flashWaitUntilIdle(info);

	info->writerAddress = 0;

	return true;
}

bool flashLogData(struct FlashInfo* info, uint8_t* src, uint16_t len)
{
	assert(len < PAGE_SZ);

	uint16_t bytesWritten = 0;

	uint32_t startAddress = info->writerAddress;
	uint32_t startPage = startAddress / PAGE_SZ;

	uint32_t currentAddress = startAddress;
	uint32_t currentPage = startPage;
	do
	{
		uint32_t nextPageStart = (currentPage + 1) * PAGE_SZ;
		uint32_t bytesToWrite = nextPageStart - currentAddress;
		uint32_t bytesRemaining = len - bytesWritten;
		if (bytesToWrite > bytesRemaining)
		{
			bytesToWrite = bytesRemaining;
		}

		if (currentAddress + bytesToWrite >= info->flashLength)
		{
			return false;
		}

		flashWriteEnable();

		HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

		uint8_t command[5] = {0x12, (currentAddress >> 24) & 0xFF, (currentAddress >> 16) & 0xFF, (currentAddress >> 8) & 0xFF, currentAddress & 0xFF};
		HAL_SPI_Transmit(&hspi1, command, 5, SPI_TIMEOUT_TICKS);
		HAL_SPI_Transmit(&hspi1, src + bytesWritten, bytesToWrite, SPI_TIMEOUT_TICKS);

		HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

		flashWaitUntilIdle(info);

		currentAddress += bytesToWrite;
		currentPage += 1;

		bytesWritten += bytesToWrite;
	} while (bytesWritten != len);

	info->writerAddress += len;

	return true;
}

bool flashReadData(struct FlashInfo* info, uint32_t address, uint8_t* dest, uint16_t len)
{
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

	uint8_t command[5] = {0x13, (address >> 24) & 0xFF, (address >> 16) & 0xFF, (address >> 8) & 0xFF, address & 0xFF};
	HAL_SPI_Transmit(&hspi1, command, 5, SPI_TIMEOUT_TICKS);

	HAL_SPI_Receive(&hspi1, dest, len, SPI_TIMEOUT_TICKS);

	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

	return true;
}
