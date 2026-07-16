#ifndef DDS_GPIO_H
#define DDS_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define DDS_GPIO_CHANNEL_1    0U
#define DDS_GPIO_CHANNEL_2    1U

void DDS_GPIO_Init(void);
void DDS_GPIO_WriteFsync(uint8_t channel, GPIO_PinState state);
GPIO_TypeDef *DDS_GPIO_GetFsyncPort(uint8_t channel);
uint16_t DDS_GPIO_GetFsyncPin(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
