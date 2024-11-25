#include <Sleep.h>

#include <stm32f0xx_hal.h>
#include <stdbool.h>

void sleepMs(uint32_t millis)
{
    uint32_t endMs = HAL_GetTick() + millis;
    while (true)
    {
        uint32_t currentMs = HAL_GetTick();
        if (currentMs >= endMs)
        {
            return;
        }
    }
}
