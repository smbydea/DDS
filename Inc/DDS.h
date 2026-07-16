#ifndef DDS_H
#define DDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum
{
    DDS_CHANNEL_1 = 0,
    DDS_CHANNEL_2,
    DDS_CHANNEL_COUNT
} DDS_Channel;

typedef enum
{
    DDS_WAVE_SINE = 0,
    DDS_WAVE_TRIANGLE
} DDS_Waveform;

typedef enum
{
    DDS_FREQ_REG_0 = 0,
    DDS_FREQ_REG_1
} DDS_FreqRegister;

typedef enum
{
    DDS_PHASE_REG_0 = 0,
    DDS_PHASE_REG_1
} DDS_PhaseRegister;

typedef struct
{
    SPI_HandleTypeDef *hspi;
    DDS_Channel channel;
    uint32_t mclk_hz;
    uint32_t spi_timeout_ms;
    uint16_t control_word;
} DDS_HandleTypeDef;

HAL_StatusTypeDef DDS_Init(void);
HAL_StatusTypeDef DDS_InitChannel(DDS_Channel channel);

HAL_StatusTypeDef DDS_ResetChannel(DDS_Channel channel);
HAL_StatusTypeDef DDS_StartChannel(DDS_Channel channel);
HAL_StatusTypeDef DDS_SetWaveformChannel(DDS_Channel channel, DDS_Waveform waveform);
HAL_StatusTypeDef DDS_SetFrequencyChannel(DDS_Channel channel, DDS_FreqRegister reg, double frequency_hz);
HAL_StatusTypeDef DDS_SelectFrequencyRegisterChannel(DDS_Channel channel, DDS_FreqRegister reg);
HAL_StatusTypeDef DDS_SetPhaseChannel(DDS_Channel channel, DDS_PhaseRegister reg, double phase_deg);
HAL_StatusTypeDef DDS_SelectPhaseRegisterChannel(DDS_Channel channel, DDS_PhaseRegister reg);
HAL_StatusTypeDef DDS_SleepChannel(DDS_Channel channel, uint8_t enable);
HAL_StatusTypeDef DDS_SetWaveChannel(DDS_Channel channel, DDS_Waveform waveform, double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_SineChannel(DDS_Channel channel, double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_TriangleChannel(DDS_Channel channel, double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_StopChannel(DDS_Channel channel);

HAL_StatusTypeDef DDS_Reset(void);
HAL_StatusTypeDef DDS_Start(void);
HAL_StatusTypeDef DDS_SetWaveform(DDS_Waveform waveform);
HAL_StatusTypeDef DDS_SetFrequency(DDS_FreqRegister reg, double frequency_hz);
HAL_StatusTypeDef DDS_SelectFrequencyRegister(DDS_FreqRegister reg);
HAL_StatusTypeDef DDS_SetPhase(DDS_PhaseRegister reg, double phase_deg);
HAL_StatusTypeDef DDS_SelectPhaseRegister(DDS_PhaseRegister reg);
HAL_StatusTypeDef DDS_Sleep(uint8_t enable);
HAL_StatusTypeDef DDS_SetWave(DDS_Waveform waveform, double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_Sine(double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_Triangle(double frequency_hz, double phase_deg);
HAL_StatusTypeDef DDS_Stop(void);

#ifdef __cplusplus
}
#endif

#endif
