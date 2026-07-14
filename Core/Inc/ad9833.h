#ifndef AD9833_H
#define AD9833_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum
{
    AD9833_WAVE_SINE = 0,
    AD9833_WAVE_TRIANGLE
} AD9833_Waveform;

typedef enum
{
    AD9833_FREQ_REG_0 = 0,
    AD9833_FREQ_REG_1
} AD9833_FreqRegister;

typedef enum
{
    AD9833_PHASE_REG_0 = 0,
    AD9833_PHASE_REG_1
} AD9833_PhaseRegister;

typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *fsync_gpio;
    uint16_t fsync_pin;
    uint32_t mclk_hz;
    uint32_t spi_timeout_ms;
    uint16_t control_word;
} AD9833_HandleTypeDef;

HAL_StatusTypeDef AD9833_Init(AD9833_HandleTypeDef *had9833,
                              SPI_HandleTypeDef *hspi,
                              GPIO_TypeDef *fsync_gpio,
                              uint16_t fsync_pin,
                              uint32_t mclk_hz);

HAL_StatusTypeDef AD9833_Reset(AD9833_HandleTypeDef *had9833);
HAL_StatusTypeDef AD9833_Start(AD9833_HandleTypeDef *had9833);
HAL_StatusTypeDef AD9833_SetWaveform(AD9833_HandleTypeDef *had9833, AD9833_Waveform waveform);
HAL_StatusTypeDef AD9833_SetFrequency(AD9833_HandleTypeDef *had9833,
                                      AD9833_FreqRegister reg,
                                      double frequency_hz);
HAL_StatusTypeDef AD9833_SelectFrequencyRegister(AD9833_HandleTypeDef *had9833, AD9833_FreqRegister reg);
HAL_StatusTypeDef AD9833_SetPhase(AD9833_HandleTypeDef *had9833,
                                  AD9833_PhaseRegister reg,
                                  double phase_deg);
HAL_StatusTypeDef AD9833_SelectPhaseRegister(AD9833_HandleTypeDef *had9833, AD9833_PhaseRegister reg);
HAL_StatusTypeDef AD9833_Sleep(AD9833_HandleTypeDef *had9833, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif
