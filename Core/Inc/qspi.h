#ifndef __QSPI_H__
#define __QSPI_H__

#include <stm32h7xx.h>

/* QSPI Error codes */
#define QSPI_OK            ((uint8_t)0x00)
#define QSPI_ERROR         ((uint8_t)0x01)
#define QSPI_BUSY          ((uint8_t)0x02)
#define QSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define QSPI_SUSPENDED     ((uint8_t)0x08)

extern uint8_t QSPI_ResetMemory(QSPI_HandleTypeDef *hqspi);
extern uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout);
extern uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi);
extern uint8_t QSPI_GetStatus(QSPI_HandleTypeDef *hqspi, uint8_t *status);
extern uint8_t QSPI_EnableQuad(QSPI_HandleTypeDef *hqspi, int en);
extern uint8_t QSPI_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
extern uint8_t QSPI_Write(QSPI_HandleTypeDef *hqspi, const uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
extern uint8_t QSPI_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t addr);
extern uint8_t QSPI_EraseAll(QSPI_HandleTypeDef *hqspi);
extern uint8_t QSPI_SendRecv(QSPI_HandleTypeDef *hqspi, void *send_buf, size_t send_length, void *recv_buf,
                             size_t recv_length);
extern uint8_t QSPI_MMAP_On(QSPI_HandleTypeDef *hqspi);
extern uint8_t QSPI_MMAP_Off(QSPI_HandleTypeDef *hqspi);

extern size_t QSPI_EraseSize(void);
extern size_t QSPI_ProgramSize(void);

#endif /* __QSPI_H__ */
