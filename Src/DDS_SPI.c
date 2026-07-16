#include "DDS_SPI.h"

/*
 * AD9833 SPI configuration.
 * Change this block when you want to use another SPIx or other SCK/MOSI pins.
 */
#define DDS_SPI_INSTANCE          SPI1
#define DDS_SPI_CLK_ENABLE()      __HAL_RCC_SPI1_CLK_ENABLE()
#define DDS_SPI_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
#define DDS_SPI_GPIO_PORT         GPIOA
#define DDS_SPI_SCK_PIN           GPIO_PIN_5
#define DDS_SPI_MOSI_PIN          GPIO_PIN_7
#define DDS_SPI_GPIO_AF           GPIO_AF5_SPI1

static SPI_HandleTypeDef hdss_spi;

static void DDS_SPI_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    DDS_SPI_CLK_ENABLE();
    DDS_SPI_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = DDS_SPI_SCK_PIN | DDS_SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = DDS_SPI_GPIO_AF;
    HAL_GPIO_Init(DDS_SPI_GPIO_PORT, &GPIO_InitStruct);
}

HAL_StatusTypeDef DDS_SPI_Init(void)
{
    DDS_SPI_GPIO_Init();

    hdss_spi.State = HAL_SPI_STATE_READY;
    hdss_spi.Instance = DDS_SPI_INSTANCE;
    hdss_spi.Init.Mode = SPI_MODE_MASTER;
    hdss_spi.Init.Direction = SPI_DIRECTION_2LINES;
    hdss_spi.Init.DataSize = SPI_DATASIZE_8BIT;
    hdss_spi.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hdss_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hdss_spi.Init.NSS = SPI_NSS_SOFT;
    hdss_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hdss_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hdss_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    hdss_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hdss_spi.Init.CRCPolynomial = 10;

    return HAL_SPI_Init(&hdss_spi);
}

SPI_HandleTypeDef *DDS_SPI_GetHandle(void)
{
    return &hdss_spi;
}

