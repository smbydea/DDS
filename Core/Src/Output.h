#ifndef OUTPUT_H
#define OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/*
 * Output 模块是比赛应用层封装：
 * - 底层仍然使用 ad9833.c/.h 负责寄存器和 SPI 通信。
 * - 本模块绑定当前板子的 SPI1、AD9833_CS 引脚和 25 MHz MCLK。
 * - main.c 只需要调用 Output_Init() 和 Output_SetWave() 即可输出波形。
 */

typedef enum
{
    OUTPUT_WAVE_SINE = 0,      /* 正弦波输出 */
    OUTPUT_WAVE_TRIANGLE       /* 三角波输出 */
} Output_Waveform;

HAL_StatusTypeDef Output_Init(void);
HAL_StatusTypeDef Output_SetWave(Output_Waveform waveform, double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_Sine(double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_Triangle(double frequency_hz, double phase_deg);
HAL_StatusTypeDef Output_SetPhase(double phase_deg);
HAL_StatusTypeDef Output_Stop(void);

#ifdef __cplusplus
}
#endif

#endif
