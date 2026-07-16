#include "DDS_GPIO.h"

/*
 * Dual AD9833 FSYNC/CS pin configuration.
 * CS1 controls channel 1, CS2 controls channel 2.
 * Change these defines when your board uses different GPIO pins.
 */
#define DDS_FSYNC1_GPIO_PORT          GPIOA
#define DDS_FSYNC1_GPIO_PIN           GPIO_PIN_4
#define DDS_FSYNC1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DDS_FSYNC2_GPIO_PORT          GPIOA
#define DDS_FSYNC2_GPIO_PIN           GPIO_PIN_3
#define DDS_FSYNC2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} DDS_FsyncPinDef;

static const DDS_FsyncPinDef dds_fsync_pins[2] = {
    {DDS_FSYNC1_GPIO_PORT, DDS_FSYNC1_GPIO_PIN},
    {DDS_FSYNC2_GPIO_PORT, DDS_FSYNC2_GPIO_PIN}
};

static uint8_t DDS_GPIO_IsValidChannel(uint8_t channel)
{
    return (channel < 2U) ? 1U : 0U;
}

void DDS_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    DDS_FSYNC1_GPIO_CLK_ENABLE();
    DDS_FSYNC2_GPIO_CLK_ENABLE();

    HAL_GPIO_WritePin(DDS_FSYNC1_GPIO_PORT, DDS_FSYNC1_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DDS_FSYNC2_GPIO_PORT, DDS_FSYNC2_GPIO_PIN, GPIO_PIN_SET);

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = DDS_FSYNC1_GPIO_PIN;
    HAL_GPIO_Init(DDS_FSYNC1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = DDS_FSYNC2_GPIO_PIN;
    HAL_GPIO_Init(DDS_FSYNC2_GPIO_PORT, &GPIO_InitStruct);
}

void DDS_GPIO_WriteFsync(uint8_t channel, GPIO_PinState state)
{
    if (DDS_GPIO_IsValidChannel(channel) == 0U) {
        return;
    }

    HAL_GPIO_WritePin(dds_fsync_pins[channel].port, dds_fsync_pins[channel].pin, state);
}

GPIO_TypeDef *DDS_GPIO_GetFsyncPort(uint8_t channel)
{
    if (DDS_GPIO_IsValidChannel(channel) == 0U) {
        return NULL;
    }

    return dds_fsync_pins[channel].port;
}

uint16_t DDS_GPIO_GetFsyncPin(uint8_t channel)
{
    if (DDS_GPIO_IsValidChannel(channel) == 0U) {
        return 0U;
    }

    return dds_fsync_pins[channel].pin;
}
