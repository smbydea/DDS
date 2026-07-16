#include "DDS.h"
#include "DDS_GPIO.h"
#include "DDS_SPI.h"

#define DDS_AD9833_MCLK_HZ       25000000UL
#define DDS_SPI_TIMEOUT_MS       10U

#define AD9833_CTRL_B28          0x2000U
#define AD9833_CTRL_HLB          0x1000U
#define AD9833_CTRL_FSELECT      0x0800U
#define AD9833_CTRL_PSELECT      0x0400U
#define AD9833_CTRL_RESET        0x0100U
#define AD9833_CTRL_SLEEP1       0x0080U
#define AD9833_CTRL_SLEEP12      0x0040U
#define AD9833_CTRL_OPBITEN      0x0020U
#define AD9833_CTRL_MODE         0x0002U

#define AD9833_REG_FREQ0         0x4000U
#define AD9833_REG_FREQ1         0x8000U
#define AD9833_REG_PHASE0        0xC000U
#define AD9833_REG_PHASE1        0xE000U

#define AD9833_FREQ_WORD_MASK    0x0FFFFFFFUL
#define AD9833_PHASE_MASK        0x0FFFU

static DDS_HandleTypeDef hdss[DDS_CHANNEL_COUNT];
static uint8_t dds_bus_is_initialized = 0U;
static uint8_t dds_channel_is_initialized[DDS_CHANNEL_COUNT] = {0U, 0U};

static uint8_t DDS_IsValidChannel(DDS_Channel channel)
{
    return (channel < DDS_CHANNEL_COUNT) ? 1U : 0U;
}

static HAL_StatusTypeDef DDS_EnsureBusReady(void)
{
    HAL_StatusTypeDef status;

    if (dds_bus_is_initialized != 0U) {
        return HAL_OK;
    }

    DDS_GPIO_Init();

    status = DDS_SPI_Init();
    if (status != HAL_OK) {
        return status;
    }

    dds_bus_is_initialized = 1U;
    return HAL_OK;
}

static HAL_StatusTypeDef DDS_WriteWord(DDS_HandleTypeDef *handle, uint16_t word)
{
    uint8_t tx[2];
    HAL_StatusTypeDef status;

    if (handle == NULL || handle->hspi == NULL || DDS_IsValidChannel(handle->channel) == 0U) {
        return HAL_ERROR;
    }

    tx[0] = (uint8_t)(word >> 8);
    tx[1] = (uint8_t)(word & 0xFFU);

    DDS_GPIO_WriteFsync((uint8_t)handle->channel, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(handle->hspi, tx, sizeof(tx), handle->spi_timeout_ms);
    DDS_GPIO_WriteFsync((uint8_t)handle->channel, GPIO_PIN_SET);

    return status;
}

static HAL_StatusTypeDef DDS_WriteControl(DDS_HandleTypeDef *handle)
{
    if (handle == NULL) {
        return HAL_ERROR;
    }

    return DDS_WriteWord(handle, handle->control_word);
}

static HAL_StatusTypeDef DDS_EnsureChannelReady(DDS_Channel channel)
{
    if (DDS_IsValidChannel(channel) == 0U) {
        return HAL_ERROR;
    }

    if (dds_channel_is_initialized[channel] != 0U) {
        return HAL_OK;
    }

    return DDS_InitChannel(channel);
}

HAL_StatusTypeDef DDS_InitChannel(DDS_Channel channel)
{
    HAL_StatusTypeDef status;

    if (DDS_IsValidChannel(channel) == 0U) {
        return HAL_ERROR;
    }

    status = DDS_EnsureBusReady();
    if (status != HAL_OK) {
        return status;
    }

    hdss[channel].hspi = DDS_SPI_GetHandle();
    hdss[channel].channel = channel;
    hdss[channel].mclk_hz = DDS_AD9833_MCLK_HZ;
    hdss[channel].spi_timeout_ms = DDS_SPI_TIMEOUT_MS;
    hdss[channel].control_word = AD9833_CTRL_B28 | AD9833_CTRL_RESET;

    DDS_GPIO_WriteFsync((uint8_t)channel, GPIO_PIN_SET);

    status = DDS_WriteControl(&hdss[channel]);
    if (status == HAL_OK) {
        dds_channel_is_initialized[channel] = 1U;
    }

    return status;
}

HAL_StatusTypeDef DDS_Init(void)
{
    HAL_StatusTypeDef status;
    uint8_t index;

    status = DDS_EnsureBusReady();
    if (status != HAL_OK) {
        return status;
    }

    for (index = 0U; index < (uint8_t)DDS_CHANNEL_COUNT; index++) {
        status = DDS_InitChannel((DDS_Channel)index);
        if (status != HAL_OK) {
            return status;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef DDS_ResetChannel(DDS_Channel channel)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    hdss[channel].control_word |= AD9833_CTRL_RESET;
    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_StartChannel(DDS_Channel channel)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    hdss[channel].control_word &= (uint16_t)~AD9833_CTRL_RESET;
    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_SetWaveformChannel(DDS_Channel channel, DDS_Waveform waveform)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    hdss[channel].control_word &= (uint16_t)~(AD9833_CTRL_OPBITEN | AD9833_CTRL_MODE);

    switch (waveform) {
    case DDS_WAVE_SINE:
        break;
    case DDS_WAVE_TRIANGLE:
        hdss[channel].control_word |= AD9833_CTRL_MODE;
        break;
    default:
        return HAL_ERROR;
    }

    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_SetFrequencyChannel(DDS_Channel channel, DDS_FreqRegister reg, double frequency_hz)
{
    uint16_t reg_base;
    uint32_t freq_word;
    HAL_StatusTypeDef status;
    DDS_HandleTypeDef *handle;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    handle = &hdss[channel];

    if (handle->mclk_hz == 0U || frequency_hz < 0.0) {
        return HAL_ERROR;
    }

    if (frequency_hz > ((double)handle->mclk_hz / 2.0)) {
        return HAL_ERROR;
    }

    switch (reg) {
    case DDS_FREQ_REG_0:
        reg_base = AD9833_REG_FREQ0;
        break;
    case DDS_FREQ_REG_1:
        reg_base = AD9833_REG_FREQ1;
        break;
    default:
        return HAL_ERROR;
    }

    freq_word = (uint32_t)(((frequency_hz * 268435456.0) / (double)handle->mclk_hz) + 0.5);
    freq_word &= AD9833_FREQ_WORD_MASK;

    handle->control_word |= AD9833_CTRL_B28;
    handle->control_word &= (uint16_t)~AD9833_CTRL_HLB;

    status = DDS_WriteControl(handle);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_WriteWord(handle, (uint16_t)(reg_base | (freq_word & 0x3FFFU)));
    if (status != HAL_OK) {
        return status;
    }

    return DDS_WriteWord(handle, (uint16_t)(reg_base | ((freq_word >> 14) & 0x3FFFU)));
}

HAL_StatusTypeDef DDS_SelectFrequencyRegisterChannel(DDS_Channel channel, DDS_FreqRegister reg)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    switch (reg) {
    case DDS_FREQ_REG_0:
        hdss[channel].control_word &= (uint16_t)~AD9833_CTRL_FSELECT;
        break;
    case DDS_FREQ_REG_1:
        hdss[channel].control_word |= AD9833_CTRL_FSELECT;
        break;
    default:
        return HAL_ERROR;
    }

    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_SetPhaseChannel(DDS_Channel channel, DDS_PhaseRegister reg, double phase_deg)
{
    uint16_t reg_base;
    uint16_t phase_word;
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    if (phase_deg < 0.0) {
        return HAL_ERROR;
    }

    while (phase_deg >= 360.0) {
        phase_deg -= 360.0;
    }

    switch (reg) {
    case DDS_PHASE_REG_0:
        reg_base = AD9833_REG_PHASE0;
        break;
    case DDS_PHASE_REG_1:
        reg_base = AD9833_REG_PHASE1;
        break;
    default:
        return HAL_ERROR;
    }

    phase_word = (uint16_t)(((phase_deg * 4096.0) / 360.0) + 0.5) & AD9833_PHASE_MASK;

    return DDS_WriteWord(&hdss[channel], (uint16_t)(reg_base | phase_word));
}

HAL_StatusTypeDef DDS_SelectPhaseRegisterChannel(DDS_Channel channel, DDS_PhaseRegister reg)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    switch (reg) {
    case DDS_PHASE_REG_0:
        hdss[channel].control_word &= (uint16_t)~AD9833_CTRL_PSELECT;
        break;
    case DDS_PHASE_REG_1:
        hdss[channel].control_word |= AD9833_CTRL_PSELECT;
        break;
    default:
        return HAL_ERROR;
    }

    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_SleepChannel(DDS_Channel channel, uint8_t enable)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    if (enable != 0U) {
        hdss[channel].control_word |= AD9833_CTRL_SLEEP1 | AD9833_CTRL_SLEEP12;
    } else {
        hdss[channel].control_word &= (uint16_t)~(AD9833_CTRL_SLEEP1 | AD9833_CTRL_SLEEP12);
    }

    return DDS_WriteControl(&hdss[channel]);
}

HAL_StatusTypeDef DDS_SetWaveChannel(DDS_Channel channel, DDS_Waveform waveform, double frequency_hz, double phase_deg)
{
    HAL_StatusTypeDef status;

    status = DDS_EnsureChannelReady(channel);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_SetFrequencyChannel(channel, DDS_FREQ_REG_0, frequency_hz);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_SetPhaseChannel(channel, DDS_PHASE_REG_0, phase_deg);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_SelectFrequencyRegisterChannel(channel, DDS_FREQ_REG_0);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_SelectPhaseRegisterChannel(channel, DDS_PHASE_REG_0);
    if (status != HAL_OK) {
        return status;
    }

    status = DDS_SetWaveformChannel(channel, waveform);
    if (status != HAL_OK) {
        return status;
    }

    return DDS_StartChannel(channel);
}

HAL_StatusTypeDef DDS_SineChannel(DDS_Channel channel, double frequency_hz, double phase_deg)
{
    return DDS_SetWaveChannel(channel, DDS_WAVE_SINE, frequency_hz, phase_deg);
}

HAL_StatusTypeDef DDS_TriangleChannel(DDS_Channel channel, double frequency_hz, double phase_deg)
{
    return DDS_SetWaveChannel(channel, DDS_WAVE_TRIANGLE, frequency_hz, phase_deg);
}

HAL_StatusTypeDef DDS_StopChannel(DDS_Channel channel)
{
    if (DDS_IsValidChannel(channel) == 0U) {
        return HAL_ERROR;
    }

    if (dds_channel_is_initialized[channel] == 0U) {
        return HAL_OK;
    }

    return DDS_ResetChannel(channel);
}

HAL_StatusTypeDef DDS_Reset(void)
{
    return DDS_ResetChannel(DDS_CHANNEL_1);
}

HAL_StatusTypeDef DDS_Start(void)
{
    return DDS_StartChannel(DDS_CHANNEL_1);
}

HAL_StatusTypeDef DDS_SetWaveform(DDS_Waveform waveform)
{
    return DDS_SetWaveformChannel(DDS_CHANNEL_1, waveform);
}

HAL_StatusTypeDef DDS_SetFrequency(DDS_FreqRegister reg, double frequency_hz)
{
    return DDS_SetFrequencyChannel(DDS_CHANNEL_1, reg, frequency_hz);
}

HAL_StatusTypeDef DDS_SelectFrequencyRegister(DDS_FreqRegister reg)
{
    return DDS_SelectFrequencyRegisterChannel(DDS_CHANNEL_1, reg);
}

HAL_StatusTypeDef DDS_SetPhase(DDS_PhaseRegister reg, double phase_deg)
{
    return DDS_SetPhaseChannel(DDS_CHANNEL_1, reg, phase_deg);
}

HAL_StatusTypeDef DDS_SelectPhaseRegister(DDS_PhaseRegister reg)
{
    return DDS_SelectPhaseRegisterChannel(DDS_CHANNEL_1, reg);
}

HAL_StatusTypeDef DDS_Sleep(uint8_t enable)
{
    return DDS_SleepChannel(DDS_CHANNEL_1, enable);
}

HAL_StatusTypeDef DDS_SetWave(DDS_Waveform waveform, double frequency_hz, double phase_deg)
{
    return DDS_SetWaveChannel(DDS_CHANNEL_1, waveform, frequency_hz, phase_deg);
}

HAL_StatusTypeDef DDS_Sine(double frequency_hz, double phase_deg)
{
    return DDS_SineChannel(DDS_CHANNEL_1, frequency_hz, phase_deg);
}

HAL_StatusTypeDef DDS_Triangle(double frequency_hz, double phase_deg)
{
    return DDS_TriangleChannel(DDS_CHANNEL_1, frequency_hz, phase_deg);
}

HAL_StatusTypeDef DDS_Stop(void)
{
    return DDS_StopChannel(DDS_CHANNEL_1);
}
