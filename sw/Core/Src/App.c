#include <App.h>

#include <main.h>
#include <Can.h>
#include <Flash.h>
#include <Obd2.h>
#include <IsoTp.h>

#include <stdio.h>
#include <string.h>
#include <usbd_cdc_if.h>

struct FlashInfo flashMemory;

uint8_t serialRxBacking[SERIAL_RX_BUFFER_SZ];
struct ByteRingBuffer serialRx;

struct __attribute__((__packed__)) CollectedData
{
	uint32_t time;
	uint16_t engineSpeed;
	uint8_t vehicleSpeed;
	uint8_t throttlePosition;
	uint8_t tankLevel;
	uint16_t fuelLevel;
};

#define SERIAL_TX_SZ (128U)

void initSerial()
{
	byteRingBufferInit(&serialRx, serialRxBacking, SERIAL_RX_BUFFER_SZ);
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

			if (strcmp(commandString, "clear\r") == 0)
			{
				printf("Clearing memory, please wait.\r\n");
				uint32_t start = HAL_GetTick();
				flashClearMemory(&flashMemory);
				uint32_t end = HAL_GetTick();

				printf("Done clearing memory (elapsed time = %ld ms)\r\n", (end - start));
			}
			else if (strcmp(commandString, "download\r\3") == 0)
			{
				printf("START>");
				for (int i = 0; i < flashMemory.flashLength; i += SERIAL_TX_SZ)
				{
					uint8_t tempBuffer[SERIAL_TX_SZ];
					flashReadData(&flashMemory, i, tempBuffer, SERIAL_TX_SZ);

					CDC_Transmit_FS(tempBuffer, SERIAL_TX_SZ);
				}
				printf("<END");
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
	bool rc = flashInit(&flashMemory);
	if (!rc)
	{
		Error_Handler();
	}

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
	struct Obd2Data obd2Data = makeObd2Data(OBD2MODE_CURRENT_DATA, OBD2PID_ENGINE_SPEED);

	uint8_t packed[ISO_TP_SF_SIZE];
	packObd2Data(packed, &obd2Data, ISO_TP_SF_SIZE);

	HAL_StatusTypeDef tx_result = canTransmit(&hcan, CAN_ID_FUNC_ADDR, packed, ISO_TP_SF_SIZE);
	if (tx_result != HAL_OK)
	{
		Error_Handler();
	}

	// RX data requests
	struct CollectedData data = {0};

	uint8_t readData[ISO_TP_SF_SIZE];
	while (canReceive(readData, ISO_TP_SF_SIZE))
	{
		parseObd2Data(&obd2Data, readData, ISO_TP_SF_SIZE);

		if (obd2Data.mode == 0x40 + OBD2MODE_CURRENT_DATA)
		{
			// TODO: Populate data
		}
	}

	// Data logging
	flashLogData(&flashMemory, (uint8_t*) &data, sizeof(data));
}
