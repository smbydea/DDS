#ifndef DDS_SPI_H
#define DDS_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef DDS_SPI_Init(void);
SPI_HandleTypeDef *DDS_SPI_GetHandle(void);

#ifdef __cplusplus
}
#endif

#endif
