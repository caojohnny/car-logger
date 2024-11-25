#ifndef CAN_H
#define CAN_H

#include <stm32f0xx_hal.h>
#include <stdbool.h>

enum CanId
{
    /// Functional addressing. Addresses all ECUs.
    CAN_ID_FUNC_ADDR = 0x7DF
};

bool canRxHasError();

bool canReceive(uint8_t* buffer, uint16_t bufferLength);

HAL_StatusTypeDef canTransmit(CAN_HandleTypeDef* canHandle, enum CanId target, const uint8_t* data, uint16_t dataLength);

HAL_StatusTypeDef canInit(CAN_HandleTypeDef* canHandle);

#endif // CAN_H
