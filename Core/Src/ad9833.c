#include "ad9833.h"

#define AD9833_CTRL_B28       0x2000U
#define AD9833_CTRL_HLB       0x1000U
#define AD9833_CTRL_FSELECT   0x0800U
#define AD9833_CTRL_PSELECT   0x0400U
#define AD9833_CTRL_RESET     0x0100U
#define AD9833_CTRL_SLEEP1    0x0080U
#define AD9833_CTRL_SLEEP12   0x0040U
#define AD9833_CTRL_OPBITEN   0x0020U
#define AD9833_CTRL_DIV2      0x0008U
#define AD9833_CTRL_MODE      0x0002U

#define AD9833_REG_FREQ0      0x4000U
#define AD9833_REG_FREQ1      0x8000U
#define AD9833_REG_PHASE0     0xC000U
#define AD9833_REG_PHASE1     0xE000U

#define AD9833_FREQ_WORD_MASK 0x0FFFFFFFUL
#define AD9833_PHASE_MASK     0x0FFFU

static HAL_StatusTypeDef AD9833_WriteWord(AD9833_HandleTypeDef *had9833, uint16_t word)
{
    uint8_t tx[2];
    HAL_StatusTypeDef status;

    if (had9833 == NULL || had9833->hspi == NULL || had9833->fsync_gpio == NULL) {
        return HAL_ERROR;
    }

    tx[0] = (uint8_t)(word >> 8);
    tx[1] = (uint8_t)(word & 0xFFU);

    HAL_GPIO_WritePin(had9833->fsync_gpio, had9833->fsync_pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(had9833->hspi, tx, sizeof(tx), had9833->spi_timeout_ms);
    HAL_GPIO_WritePin(had9833->fsync_gpio, had9833->fsync_pin, GPIO_PIN_SET);

    return status;
}

static HAL_StatusTypeDef AD9833_WriteControl(AD9833_HandleTypeDef *had9833)
{
    return AD9833_WriteWord(had9833, had9833->control_word);
}

HAL_StatusTypeDef AD9833_Init(AD9833_HandleTypeDef *had9833,
                              SPI_HandleTypeDef *hspi,
                              GPIO_TypeDef *fsync_gpio,
                              uint16_t fsync_pin,
                              uint32_t mclk_hz)
{
    if (had9833 == NULL || hspi == NULL || fsync_gpio == NULL || mclk_hz == 0U) {
        return HAL_ERROR;
    }

    had9833->hspi = hspi;
    had9833->fsync_gpio = fsync_gpio;
    had9833->fsync_pin = fsync_pin;
    had9833->mclk_hz = mclk_hz;
    had9833->spi_timeout_ms = 10U;
    had9833->control_word = AD9833_CTRL_B28 | AD9833_CTRL_RESET;

    HAL_GPIO_WritePin(had9833->fsync_gpio, had9833->fsync_pin, GPIO_PIN_SET);

    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_Reset(AD9833_HandleTypeDef *had9833)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    had9833->control_word |= AD9833_CTRL_RESET;
    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_Start(AD9833_HandleTypeDef *had9833)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    had9833->control_word &= (uint16_t)~AD9833_CTRL_RESET;
    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_SetWaveform(AD9833_HandleTypeDef *had9833, AD9833_Waveform waveform)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    had9833->control_word &= (uint16_t)~(AD9833_CTRL_OPBITEN | AD9833_CTRL_MODE);

    switch (waveform) {
    case AD9833_WAVE_SINE:
        break;
    case AD9833_WAVE_TRIANGLE:
        had9833->control_word |= AD9833_CTRL_MODE;
        break;
    default:
        return HAL_ERROR;
    }

    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_SetFrequency(AD9833_HandleTypeDef *had9833,
                                      AD9833_FreqRegister reg,
                                      double frequency_hz)
{
    uint16_t reg_base;
    uint32_t freq_word;
    HAL_StatusTypeDef status;

    if (had9833 == NULL || had9833->mclk_hz == 0U || frequency_hz < 0.0) {
        return HAL_ERROR;
    }

    if (frequency_hz > ((double)had9833->mclk_hz / 2.0)) {
        return HAL_ERROR;
    }

    switch (reg) {
    case AD9833_FREQ_REG_0:
        reg_base = AD9833_REG_FREQ0;
        break;
    case AD9833_FREQ_REG_1:
        reg_base = AD9833_REG_FREQ1;
        break;
    default:
        return HAL_ERROR;
    }

    freq_word = (uint32_t)(((frequency_hz * 268435456.0) / (double)had9833->mclk_hz) + 0.5);
    freq_word &= AD9833_FREQ_WORD_MASK;

    had9833->control_word |= AD9833_CTRL_B28;
    had9833->control_word &= (uint16_t)~AD9833_CTRL_HLB;

    status = AD9833_WriteControl(had9833);
    if (status != HAL_OK) {
        return status;
    }

    status = AD9833_WriteWord(had9833, (uint16_t)(reg_base | (freq_word & 0x3FFFU)));
    if (status != HAL_OK) {
        return status;
    }

    return AD9833_WriteWord(had9833, (uint16_t)(reg_base | ((freq_word >> 14) & 0x3FFFU)));
}

HAL_StatusTypeDef AD9833_SelectFrequencyRegister(AD9833_HandleTypeDef *had9833, AD9833_FreqRegister reg)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    switch (reg) {
    case AD9833_FREQ_REG_0:
        had9833->control_word &= (uint16_t)~AD9833_CTRL_FSELECT;
        break;
    case AD9833_FREQ_REG_1:
        had9833->control_word |= AD9833_CTRL_FSELECT;
        break;
    default:
        return HAL_ERROR;
    }

    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_SetPhase(AD9833_HandleTypeDef *had9833,
                                  AD9833_PhaseRegister reg,
                                  double phase_deg)
{
    uint16_t reg_base;
    uint16_t phase_word;

    if (had9833 == NULL || phase_deg < 0.0) {
        return HAL_ERROR;
    }

    while (phase_deg >= 360.0) {
        phase_deg -= 360.0;
    }

    switch (reg) {
    case AD9833_PHASE_REG_0:
        reg_base = AD9833_REG_PHASE0;
        break;
    case AD9833_PHASE_REG_1:
        reg_base = AD9833_REG_PHASE1;
        break;
    default:
        return HAL_ERROR;
    }

    phase_word = (uint16_t)(((phase_deg * 4096.0) / 360.0) + 0.5) & AD9833_PHASE_MASK;

    return AD9833_WriteWord(had9833, (uint16_t)(reg_base | phase_word));
}

HAL_StatusTypeDef AD9833_SelectPhaseRegister(AD9833_HandleTypeDef *had9833, AD9833_PhaseRegister reg)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    switch (reg) {
    case AD9833_PHASE_REG_0:
        had9833->control_word &= (uint16_t)~AD9833_CTRL_PSELECT;
        break;
    case AD9833_PHASE_REG_1:
        had9833->control_word |= AD9833_CTRL_PSELECT;
        break;
    default:
        return HAL_ERROR;
    }

    return AD9833_WriteControl(had9833);
}

HAL_StatusTypeDef AD9833_Sleep(AD9833_HandleTypeDef *had9833, uint8_t enable)
{
    if (had9833 == NULL) {
        return HAL_ERROR;
    }

    if (enable != 0U) {
        had9833->control_word |= AD9833_CTRL_SLEEP1 | AD9833_CTRL_SLEEP12;
    } else {
        had9833->control_word &= (uint16_t)~(AD9833_CTRL_SLEEP1 | AD9833_CTRL_SLEEP12);
    }

    return AD9833_WriteControl(had9833);
}
