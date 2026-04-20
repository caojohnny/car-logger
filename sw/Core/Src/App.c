#include <App.h>

#include <main.h>
#include <Can.h>
#include <Flash.h>
#include <Obd2.h>
#include <IsoTp.h>
#include <Sleep.h>

#include <stdio.h>
#include <string.h>
#include <usbd_cdc_if.h>

struct FlashInfo flashMemory;

uint8_t serialRxBacking[SERIAL_RX_BUFFER_SZ];
struct ByteRingBuffer serialRx;

#define MIN_FREE_BLOCK_SZ (11U)
struct __attribute__((__packed__)) CollectedData
{
	uint32_t time;
	uint16_t engineSpeed;
	uint8_t vehicleSpeed;
	uint8_t throttlePosition;
	uint8_t tankLevel;
	uint16_t fuelRate;
};

#define START_DELIMITER ("START>")
#define END_DELIMITER ("<END")
#define SERIAL_TX_SZ (128U)

#define PIDS_TO_REQUEST_SZ (5U)
static enum Obd2Pid PIDS_TO_REQUEST[PIDS_TO_REQUEST_SZ] = {
	OBD2PID_ENGINE_SPEED,
	OBD2PID_VEHICLE_SPEED,
	OBD2PID_THROTTLE_POSITION,
	OBD2PID_FUEL_TANK_LEVEL,
	OBD2PID_ENGINE_FUEL_RATE
};
static uint16_t pidIndex = 0;

struct CollectedData collectedData;

bool loggingEnabled = true;

void initSerial()
{
	byteRingBufferInit(&serialRx, serialRxBacking, SERIAL_RX_BUFFER_SZ);
}

static bool isCommand(char* commandString, char* expectedCommand, int len)
{
	return len > strlen(expectedCommand) && strncmp(commandString, expectedCommand, len - 1) == 0;
}

static void transmitBuffer(void* ptr, int len)
{
	while (CDC_Transmit_FS(ptr, len))
	{
		sleepMs(1);
	}
}

void stepSerial()
{
	if (serialRx.size > 0)
	{
		int commandLen = 0;
		for (int i = serialRx.readerIndex; i < serialRx.readerIndex + serialRx.size; ++i)
		{
			int rbIndex = i;
			if (rbIndex >= serialRx.length)
			{
				rbIndex -= serialRx.length;
			}

			if (serialRx.backing[rbIndex] == (uint8_t) '\r')
			{
				commandLen = i - serialRx.readerIndex + 1;
				break;
			}
		}

		if (commandLen != 0)
		{
			char commandString[SERIAL_RX_BUFFER_SZ];
			byteRingBufferRemove(&serialRx, (uint8_t*) commandString, commandLen);

			if (isCommand(commandString, "clear", commandLen))
			{
				// Prevents us from writing cleared memory
				loggingEnabled = false;

				printf("Clearing memory, please wait.\r\n");
				uint32_t start = HAL_GetTick();
				flashClearMemory(&flashMemory);
				uint32_t end = HAL_GetTick();

				printf("Done clearing memory (elapsed time = %ld ms)\r\n", (end - start));

				loggingEnabled = true;
			}
			else if (isCommand(commandString, "download", commandLen))
			{
				// Prevents us from reading forever
				loggingEnabled = false;

				transmitBuffer(START_DELIMITER, strlen(START_DELIMITER));
				uint32_t maxAvailableData = flashMemory.writerAddress;
				for (int i = 0; i < maxAvailableData; i += SERIAL_TX_SZ)
				{
					uint16_t readLength = SERIAL_TX_SZ;
					uint32_t maxRemaining = maxAvailableData - i;
					if (maxRemaining < readLength)
					{
						readLength = (uint16_t) maxRemaining;
					}

					uint8_t tempBuffer[SERIAL_TX_SZ];
					flashReadData(&flashMemory, i, tempBuffer, readLength);

					transmitBuffer(tempBuffer, readLength);
				}
				transmitBuffer(END_DELIMITER, strlen(END_DELIMITER));

				loggingEnabled = true;
			}
			else
			{
				printf("Unknown command: %s\n", commandString);
			}
		}
	}
}

void initDataCollection()
{
	assert(MIN_FREE_BLOCK_SZ == sizeof(struct CollectedData));

	uint8_t flashInitWorkingMemory[MIN_FREE_BLOCK_SZ];
	bool rc = flashInit(&flashMemory, flashInitWorkingMemory, MIN_FREE_BLOCK_SZ);
	if (!rc)
	{
		Error_Handler();
	}

	printf("Finished flash initialization (start = %ld)\r\n", flashMemory.writerAddress);

	HAL_StatusTypeDef initResult = canInit(&hcan);
	if (initResult != HAL_OK)
	{
		Error_Handler();
	}
}

void stepDataCollection()
{
	bool hasRxError = canRxHasError();
	if (hasRxError)
	{
		Error_Handler();
	}

	// TX data requests
	struct Obd2Data obd2Data = makeObd2Data(OBD2MODE_CURRENT_DATA, PIDS_TO_REQUEST[pidIndex]);

	uint8_t packed[ISO_TP_SF_SIZE];
	packObd2Data(packed, &obd2Data, ISO_TP_SF_SIZE);

	HAL_StatusTypeDef txResult = canTransmit(&hcan, CAN_ID_FUNC_ADDR, packed, ISO_TP_SF_SIZE);
	bool hasCanConnection = txResult == HAL_OK;

	// RX data requests
	uint8_t readData[ISO_TP_SF_SIZE];
	while (canReceive(readData, ISO_TP_SF_SIZE))
	{
		struct Obd2Data obd2Data;
		parseObd2Data(&obd2Data, readData, ISO_TP_SF_SIZE);
		if (obd2Data.mode == 0x40 + OBD2MODE_CURRENT_DATA)
		{
			if (obd2Data.pid == OBD2PID_ENGINE_SPEED)
			{
				collectedData.engineSpeed = (((uint16_t) obd2Data.abcd[0]) << 8) | (obd2Data.abcd[1]);
			}
			else if (obd2Data.pid == OBD2PID_VEHICLE_SPEED)
			{
				collectedData.vehicleSpeed = obd2Data.abcd[0];
			}
			else if (obd2Data.pid == OBD2PID_THROTTLE_POSITION)
			{
				collectedData.throttlePosition = obd2Data.abcd[0];
			}
			else if (obd2Data.pid == OBD2PID_FUEL_TANK_LEVEL)
			{
				collectedData.tankLevel = obd2Data.abcd[0];
			}
			else if (obd2Data.pid == OBD2PID_ENGINE_FUEL_RATE)
			{
				collectedData.fuelRate = (((uint16_t) obd2Data.abcd[0]) << 8) | (obd2Data.abcd[1]);
			}
			else
			{
				Error_Handler();
			}
		}
	}

	pidIndex++;
	bool hasLoggedAllPids = (pidIndex == PIDS_TO_REQUEST_SZ);
	if (hasLoggedAllPids)
	{
		pidIndex = 0;
	}

	// Data logging
	if (loggingEnabled && hasCanConnection && hasLoggedAllPids)
	{
		collectedData.time = HAL_GetTick();
		flashLogData(&flashMemory, (uint8_t*) &collectedData, sizeof(collectedData));
	}
}
