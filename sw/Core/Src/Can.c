#include <Can.h>
#include <ByteRingBuffer.h>

/// The FIFO used for CAN RX
#define CAN_SELECTED_FIFO (CAN_RX_FIFO0)

/// Size of data received from the CAN HAL
#define CAN_RX_SIZE (8U)

/// The size of the CAN RX backing buffer
#define CAN_RX_BUFFER_SIZE (1024U)
/// Backing data for the ring buffer
static uint8_t rxBufferBacking[CAN_RX_BUFFER_SIZE];
/// The CAN RX buffer containing all read data
static struct ByteRingBuffer rxBuffer;
/// Error flag for RX data
static bool rxError;

bool canRxHasError()
{
	return rxError;
}

bool canReceive(uint8_t* buffer, uint16_t bufferLength)
{
	return byteRingBufferRemove(&rxBuffer, buffer, bufferLength);
}

HAL_StatusTypeDef canTransmit(CAN_HandleTypeDef* canHandle, enum CanId target, const uint8_t* data, uint16_t dataLength)
{
	CAN_TxHeaderTypeDef header;
	header.StdId = (uint32_t) target;
	header.IDE = CAN_ID_STD;
	header.RTR = CAN_RTR_DATA;
	header.DLC = dataLength;
	header.TransmitGlobalTime = DISABLE;

	uint32_t mb;
	HAL_StatusTypeDef rc = HAL_CAN_AddTxMessage(canHandle, &header, data, &mb);

	return rc != HAL_OK;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *canHandle)
{

	// Avoid masking an error
	if (rxError)
	{
		return;
	}

	CAN_RxHeaderTypeDef header;
	uint8_t data[CAN_RX_SIZE];
	HAL_CAN_GetRxMessage(canHandle, CAN_SELECTED_FIFO, &header, data);

	rxError = !byteRingBufferAdd(&rxBuffer, data, CAN_RX_SIZE);
}

HAL_StatusTypeDef canInit(CAN_HandleTypeDef* canHandle)
{
	byteRingBufferInit(&rxBuffer, rxBufferBacking, CAN_RX_BUFFER_SIZE);
	rxError = false;

	CAN_FilterTypeDef sf;
	sf.FilterIdHigh = 0x0000;
	sf.FilterIdLow = 0x0000;
    sf.FilterMaskIdHigh = 0x0000;
    sf.FilterMaskIdLow = 0x0000;
    sf.FilterFIFOAssignment = CAN_SELECTED_FIFO;
    sf.FilterBank = 0;
    sf.FilterMode = CAN_FILTERMODE_IDMASK;
    sf.FilterScale = CAN_FILTERSCALE_32BIT;
    sf.FilterActivation = CAN_FILTER_ENABLE;
    sf.SlaveStartFilterBank = 14;

    HAL_StatusTypeDef status = HAL_CAN_ConfigFilter(canHandle, &sf);
    if (status != HAL_OK) {
    	return status;
    }

	status = HAL_CAN_Start(canHandle);
    if (status != HAL_OK) {
    	return status;
    }

    status = HAL_CAN_ActivateNotification(canHandle, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK) {
    	return status;
    }

    return HAL_OK;
}
