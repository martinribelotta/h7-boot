/**
 ******************************************************************************
 * File Name          : QUADSPI.c
 * Description        : This file provides code for the configuration
 *                      of the QUADSPI instances.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "quadspi.h"

/* USER CODE BEGIN 0 */
#include "qspi.h"
/* USER CODE END 0 */

QSPI_HandleTypeDef hqspi;

/* QUADSPI init function */
void MX_QUADSPI_Init(void)
{

    hqspi.Instance = QUADSPI;
    hqspi.Init.ClockPrescaler = 0;
    hqspi.Init.FifoThreshold = 1;
    hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
    hqspi.Init.FlashSize = 24;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_3_CYCLE;
    hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID = QSPI_FLASH_ID_2;
    hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        Error_Handler();
    }
    if (QSPI_ResetMemory(&hqspi) != QSPI_OK) {
        Error_Handler();
    }
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *qspiHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (qspiHandle->Instance == QUADSPI) {
        /* USER CODE BEGIN QUADSPI_MspInit 0 */

        /* USER CODE END QUADSPI_MspInit 0 */
        /* QUADSPI clock enable */
        __HAL_RCC_QSPI_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**QUADSPI GPIO Configuration
        PB2     ------> QUADSPI_CLK
        PE7     ------> QUADSPI_BK2_IO0
        PE8     ------> QUADSPI_BK2_IO1
        PE9     ------> QUADSPI_BK2_IO2
        PE10     ------> QUADSPI_BK2_IO3
        PC11     ------> QUADSPI_BK2_NCS
        */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin =
            GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USER CODE BEGIN QUADSPI_MspInit 1 */

        /* USER CODE END QUADSPI_MspInit 1 */
    }
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *qspiHandle)
{

    if (qspiHandle->Instance == QUADSPI) {
        /* USER CODE BEGIN QUADSPI_MspDeInit 0 */

        /* USER CODE END QUADSPI_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_QSPI_CLK_DISABLE();

        /**QUADSPI GPIO Configuration
        PB2     ------> QUADSPI_CLK
        PE7     ------> QUADSPI_BK2_IO0
        PE8     ------> QUADSPI_BK2_IO1
        PE9     ------> QUADSPI_BK2_IO2
        PE10     ------> QUADSPI_BK2_IO3
        PC11     ------> QUADSPI_BK2_NCS
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2);

        HAL_GPIO_DeInit(GPIOE,
                        GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_11);

        /* USER CODE BEGIN QUADSPI_MspDeInit 1 */

        /* USER CODE END QUADSPI_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

size_t MX_QUADSPI_Read(void *buf, uint32_t addr, size_t size)
{
    if (QSPI_Read(&hqspi, (uint8_t *)buf, addr, size) != QSPI_OK) {
        return -1;
    }
    return size;
}

int MX_QUADSPI_Erase(uint32_t addr, size_t size)
{
    const size_t eblock = QSPI_EraseSize();
    size_t end = addr + size;
    while (addr < end) {
        if (QSPI_WriteEnable(&hqspi) != QSPI_OK)
            return -1;
        if (QSPI_EraseSector(&hqspi, addr) != QSPI_OK) {
            break;
        }
        addr += eblock;
    }
    return size - (end - addr);
}

int MX_QUADSPI_Write(const void *buf, uint32_t addr, size_t size)
{
    size_t writeCount = 0;
    const size_t psize = QSPI_ProgramSize();
    while (writeCount < size) {
        size_t remain = size - writeCount;
        size_t chunk = psize;
        if (remain < chunk) {
            chunk = remain;
        }
        if (QSPI_WriteEnable(&hqspi) != QSPI_OK)
            return -1;
        if (QSPI_Write(&hqspi, (const uint8_t *)buf, addr, chunk) != QSPI_OK) {
            break;
        }
        addr += chunk;
        writeCount += chunk;
    }
    return writeCount;
}

int MX_QUADSPI_EnterMMAP(void)
{
    return QSPI_MMAP_On(&hqspi) == QSPI_OK;
}

void MX_QUADSPI_ExitMMAP(void)
{
    QSPI_MMAP_Off(&hqspi);
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
