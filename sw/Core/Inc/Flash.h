#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>

struct flash_info
{
};

struct flash_data_value
{
};

bool flash_init(struct flash_info* info);

bool flash_log_data(struct flash_info* info, const struct flash_data_value* data);

#endif // FLASH_H
