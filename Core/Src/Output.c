#include "Output.h"
#include "ad9833.h"
#include "main.h"

/*
 * 本文件是 AD9833 的“比赛可直接使用”封装层。
 *
 * 使用步骤：
 * 1. 在 main.c 中完成 HAL_Init、SystemClock_Config、MX_GPIO_Init、MX_SPI1_Init。
 * 2. 调用 Output_Init() 初始化 AD9833。
 * 3. 调用 Output_SetWave(OUTPUT_WAVE_SINE, 20000.0, 0.0) 输出指定波形。
 *
 * 当前硬件绑定：
 * - AD9833 FSYNC/CS: PA4，也就是 main.h 中的 AD9833_CS_GPIO_Port 和 AD9833_CS_Pin。
 * - SPI: SPI1，SCK=PA5，MOSI=PA7。
 * - AD9833 MCLK: 25 MHz。若你的模块晶振不是 25 MHz，只改 OUTPUT_AD9833_MCLK_HZ。
 */
#define OUTPUT_AD9833_MCLK_HZ        25000000UL
#define OUTPUT_DEFAULT_FREQ_HZ       20000.0
#define OUTPUT_DEFAULT_PHASE_DEG     0.0

extern SPI_HandleTypeDef hspi1;

static AD9833_HandleTypeDef output_ad9833;
static uint8_t output_is_initialized = 0U;
static double output_current_phase_deg = OUTPUT_DEFAULT_PHASE_DEG;

static AD9833_Waveform Output_ToAd9833Waveform(Output_Waveform waveform)
{
    switch (waveform) {
    case OUTPUT_WAVE_SINE:
        return AD9833_WAVE_SINE;
    case OUTPUT_WAVE_TRIANGLE:
        return AD9833_WAVE_TRIANGLE;
    default:
        return AD9833_WAVE_SINE;
    }
}

/*
 * 初始化 AD9833。
 *
 * 这个函数只需要在外设初始化完成后调用一次。它会把 SPI1、PA4 片选脚和
 * 25 MHz MCLK 交给底层 AD9833 驱动，并让芯片先保持 reset 状态。
 */
HAL_StatusTypeDef Output_Init(void)
{
    HAL_StatusTypeDef status;

    status = AD9833_Init(&output_ad9833,
                         &hspi1,
                         AD9833_CS_GPIO_Port,
                         AD9833_CS_Pin,
                         OUTPUT_AD9833_MCLK_HZ);
    if (status != HAL_OK) {
        return status;
    }

    output_is_initialized = 1U;
    output_current_phase_deg = OUTPUT_DEFAULT_PHASE_DEG;

    return HAL_OK;
}

/*
 * 输出指定波形、频率和相位。
 *
 * 参数说明：
 * - waveform: OUTPUT_WAVE_SINE 为正弦波，OUTPUT_WAVE_TRIANGLE 为三角波。
 * - frequency_hz: 输出频率，单位 Hz。AD9833 理论上不能超过 MCLK/2。
 * - phase_deg: 输出相位，单位度。比如 0.0、90.0、180.0。
 *
 * 比赛现场最常用的就是这个函数，例如：
 * Output_SetWave(OUTPUT_WAVE_SINE, 1000.0, 0.0);
 */
HAL_StatusTypeDef Output_SetWave(Output_Waveform waveform, double frequency_hz, double phase_deg)
{
    HAL_StatusTypeDef status;

    if (output_is_initialized == 0U) {
        status = Output_Init();
        if (status != HAL_OK) {
            return status;
        }
    }

    if (waveform != OUTPUT_WAVE_SINE && waveform != OUTPUT_WAVE_TRIANGLE) {
        return HAL_ERROR;
    }

    /*
     * 频率写入 FREQ0。
     * 底层 AD9833_SetFrequency 会完成：freq_word = frequency_hz * 2^28 / MCLK。
     */
    status = AD9833_SetFrequency(&output_ad9833, AD9833_FREQ_REG_0, frequency_hz);
    if (status != HAL_OK) {
        return status;
    }

    /*
     * 相位写入 PHASE0。
     * 底层 AD9833_SetPhase 会完成：phase_word = phase_deg * 4096 / 360。
     */
    status = AD9833_SetPhase(&output_ad9833, AD9833_PHASE_REG_0, phase_deg);
    if (status != HAL_OK) {
        return status;
    }

    status = AD9833_SelectFrequencyRegister(&output_ad9833, AD9833_FREQ_REG_0);
    if (status != HAL_OK) {
        return status;
    }

    status = AD9833_SelectPhaseRegister(&output_ad9833, AD9833_PHASE_REG_0);
    if (status != HAL_OK) {
        return status;
    }

    status = AD9833_SetWaveform(&output_ad9833, Output_ToAd9833Waveform(waveform));
    if (status != HAL_OK) {
        return status;
    }

    output_current_phase_deg = phase_deg;

    /* 清除 AD9833 reset 位，开始从 VOUT 输出波形。 */
    return AD9833_Start(&output_ad9833);
}

HAL_StatusTypeDef Output_Sine(double frequency_hz, double phase_deg)
{
    return Output_SetWave(OUTPUT_WAVE_SINE, frequency_hz, phase_deg);
}

HAL_StatusTypeDef Output_Triangle(double frequency_hz, double phase_deg)
{
    return Output_SetWave(OUTPUT_WAVE_TRIANGLE, frequency_hz, phase_deg);
}

/*
 * 只修改相位，不改变当前频率和波形。
 * 注意：如果你还没有调用 Output_SetWave，本函数会先初始化 AD9833，
 * 但真正开始输出仍建议先调用 Output_Sine 或 Output_Triangle。
 */
HAL_StatusTypeDef Output_SetPhase(double phase_deg)
{
    HAL_StatusTypeDef status;

    if (output_is_initialized == 0U) {
        status = Output_Init();
        if (status != HAL_OK) {
            return status;
        }
    }

    status = AD9833_SetPhase(&output_ad9833, AD9833_PHASE_REG_0, phase_deg);
    if (status == HAL_OK) {
        output_current_phase_deg = phase_deg;
    }

    return status;
}

/*
 * 停止输出：让 AD9833 重新进入 reset 状态。
 * 这不会断电，也不会清空 STM32 里的句柄配置；再次 Output_SetWave 即可恢复输出。
 */
HAL_StatusTypeDef Output_Stop(void)
{
    if (output_is_initialized == 0U) {
        return HAL_OK;
    }

    return AD9833_Reset(&output_ad9833);
}
