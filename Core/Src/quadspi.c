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

int MX_QUADSPI_EraseAll(void)
{
    if (QSPI_WriteEnable(&hqspi) != QSPI_OK) {
        return 0;
    }
    if (QSPI_EraseAll(&hqspi) != QSPI_OK) {
        return 0;
    }
    return 1;
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
        buf += chunk;
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
