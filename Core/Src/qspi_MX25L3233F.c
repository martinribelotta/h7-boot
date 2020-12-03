#include <qspi.h>

/* Reset Operations */
#define RESET_ENABLE_CMD 0x66
#define RESET_MEMORY_CMD 0x99

/* Register Operations */
#define READ_STATUS_REG_CMD  0x05
#define WRITE_STATUS_REG_CMD 0x01

/* Status Register */
#define STATUS_WIP  ((uint8_t)0x01) /*!< Write in progress */
#define STATUS_WREN ((uint8_t)0x02) /*!< Write enable latch */
#define STATUS_QEN  ((uint8_t)0x40) /*!< Quad enable */

/* Write Operations */
#define WRITE_ENABLE_CMD  0x06
#define WRITE_DISABLE_CMD 0x04

/* Register Operations */
#define READ_STATUS_REG_CMD  0x05
#define WRITE_STATUS_REG_CMD 0x01

/* Read Operations */
#define READ_CMD                 0x03
#define FAST_READ_CMD            0x0B
#define DUAL_OUT_FAST_READ_CMD   0x3B
#define DUAL_INOUT_FAST_READ_CMD 0xBB
#define QUAD_OUT_FAST_READ_CMD   0x6B
#define QUAD_INOUT_FAST_READ_CMD 0xEB

#define QUAD_INOUT_FAST_READ_DUMMY_CYCLES 6

#define ERASE_CMD     0x20
#define ERASE_ALL_CMD 0x60
#define WRITE_CMD     0x02

/**
 * This function can send or send then receive QSPI data.
 */
uint8_t qspi_send_then_recv(QSPI_HandleTypeDef *hqspi, void *send_buf, size_t send_length, void *recv_buf,
                            size_t recv_length)
{
    QSPI_CommandTypeDef Cmdhandler;
    unsigned char *ptr = (unsigned char *)send_buf;
    size_t count = 0;
    uint8_t result = QSPI_OK;

    /* get instruction */
    Cmdhandler.Instruction = ptr[0];
    Cmdhandler.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    count++;

    /* get address */
    if (send_length > 1) {
        if (send_length >= 4) {
            /* address size is 3 Byte */
            Cmdhandler.Address = (ptr[1] << 16) | (ptr[2] << 8) | (ptr[3]);
            Cmdhandler.AddressSize = QSPI_ADDRESS_24_BITS;
            count += 3;
            Cmdhandler.AddressMode = QSPI_ADDRESS_1_LINE;
        } else {
            if (send_length < 3) {
                return QSPI_ERROR;
            }
            Cmdhandler.Address = 0;
            Cmdhandler.AddressMode = QSPI_ADDRESS_NONE;
            Cmdhandler.AddressSize = 0;
        }
    } else {
        /* no address stage */
        Cmdhandler.Address = 0;
        Cmdhandler.AddressMode = QSPI_ADDRESS_NONE;
        Cmdhandler.AddressSize = 0;
    }

    Cmdhandler.AlternateBytes = 0;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.AlternateBytesSize = 0;

    Cmdhandler.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.DdrMode = QSPI_DDR_MODE_DISABLE;
    Cmdhandler.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;

    if (send_buf && recv_buf) {
        /* recv data */
        /* set dummy cycles */
        if (count != send_length) {
            Cmdhandler.DummyCycles = (send_length - count) * 8;
        } else {
            Cmdhandler.DummyCycles = 0;
        }

        /* set recv size */
        Cmdhandler.DataMode = QSPI_DATA_1_LINE;
        Cmdhandler.NbData = recv_length;
        HAL_QSPI_Command(hqspi, &Cmdhandler, 5000);

        if (recv_length != 0) {
            if (HAL_QSPI_Receive(hqspi, recv_buf, 5000) != HAL_OK) {
                // sfud_log_info("qspi recv data failed(%d)!",
                // hqspi->ErrorCode);
                hqspi->State = HAL_QSPI_STATE_READY;
                result = QSPI_ERROR;
            }
        }

        return result;
    } else {
        /* send data */
        /* set dummy cycles */
        Cmdhandler.DummyCycles = 0;

        /* determine if there is data to send */
        if (send_length - count > 0) {
            Cmdhandler.DataMode = QSPI_DATA_1_LINE;
        } else {
            Cmdhandler.DataMode = QSPI_DATA_NONE;
        }

        /* set send buf and send size */
        Cmdhandler.NbData = send_length - count;
        HAL_QSPI_Command(hqspi, &Cmdhandler, 5000);

        if (send_length - count > 0) {
            if (HAL_QSPI_Transmit(hqspi, (uint8_t *)(ptr + count), 5000) != HAL_OK) {
                // sfud_log_info("qspi send data failed(%d)!", hqspi.ErrorCode);
                hqspi->State = HAL_QSPI_STATE_READY;
                result = QSPI_ERROR;
            }
        }

        return result;
    }
}

uint8_t QSPI_EnableQuad(QSPI_HandleTypeDef *hqspi, int en)
{
    uint8_t r = QSPI_WriteEnable(hqspi);
    if (r != QSPI_OK) {
        return r;
    }
    uint8_t stbuf[3] = {WRITE_STATUS_REG_CMD, 0, 0};
    r = QSPI_GetStatus(hqspi, &stbuf[1]);
    if (en) {
        stbuf[1] |= STATUS_QEN;
    } else {
        stbuf[1] &= ~STATUS_QEN;
    }
    if (qspi_send_then_recv(hqspi, stbuf, sizeof(stbuf), NULL, 0) != QSPI_OK) {
        return QSPI_ERROR;
    }
    return QSPI_AutoPollingMemReady(hqspi, 5000);
}

uint8_t QSPI_GetStatus(QSPI_HandleTypeDef *hqspi, uint8_t *status)
{
    uint8_t cmd = READ_STATUS_REG_CMD;
    uint8_t stbuf[2];
    uint8_t r = qspi_send_then_recv(hqspi, &cmd, 1, stbuf, 2);
    *status = stbuf[0];
    return r;
}

/**
 * @brief  Reads an amount of data from the QSPI memory.
 * @param  pData: Pointer to data to be read
 * @param  ReadAddr: Read start address
 * @param  Size: Size of data to read
 * @retval QSPI memory status
 */
uint8_t QSPI_Read(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = QUAD_INOUT_FAST_READ_CMD;
    s_command.AddressMode = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_4_LINES;
    s_command.DummyCycles = 6;
    s_command.NbData = Size;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    /* Set S# timing for Read command: Min 20ns for N25Q128A memory */
    MODIFY_REG(hqspi->Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_2_CYCLE);

    /* Reception of the data */
    if (HAL_QSPI_Receive(hqspi, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    /* Restore S# timing for nonRead commands */
    MODIFY_REG(hqspi->Instance->DCR, QUADSPI_DCR_CSHT, QSPI_CS_HIGH_TIME_5_CYCLE);

    return QSPI_OK;
}

uint8_t QSPI_Write(QSPI_HandleTypeDef *hqspi, const uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
    QSPI_CommandTypeDef Cmdhandler;

    Cmdhandler.Instruction = WRITE_CMD;
    Cmdhandler.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    Cmdhandler.Address = WriteAddr;
    Cmdhandler.AddressSize = QSPI_ADDRESS_24_BITS;
    Cmdhandler.AddressMode = QSPI_ADDRESS_1_LINE;
    Cmdhandler.AlternateBytes = 0;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.AlternateBytesSize = 0;
    Cmdhandler.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.DdrMode = QSPI_DDR_MODE_DISABLE;
    Cmdhandler.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    Cmdhandler.DummyCycles = 0;
    Cmdhandler.DataMode = QSPI_DATA_1_LINE;
    Cmdhandler.NbData = Size;

    HAL_QSPI_Command(hqspi, &Cmdhandler, 5000);

    if (HAL_QSPI_Transmit(hqspi, (uint8_t *)(pData), 5000) != HAL_OK) {
        hqspi->State = HAL_QSPI_STATE_READY;
        return QSPI_ERROR;
    }
    return QSPI_AutoPollingMemReady(hqspi, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

uint8_t QSPI_EraseSector(QSPI_HandleTypeDef *hqspi, uint32_t addr)
{
    uint8_t cmd[4] = {ERASE_CMD, (addr >> 16) & 0xff, (addr >> 8) & 0xff, (addr)&0xff};
    uint8_t r = qspi_send_then_recv(hqspi, cmd, sizeof(cmd), NULL, 0);
    if (r != QSPI_OK) {
        return r;
    }
    return QSPI_AutoPollingMemReady(hqspi, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

uint8_t QSPI_EraseAll(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;

    /* Initialize the reset enable command */
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = ERASE_ALL_CMD;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &cmd, 5000) != HAL_OK) {
        return QSPI_ERROR;
    }

    if (QSPI_AutoPollingMemReady(hqspi, 30000) != HAL_OK) {
        return QSPI_ERROR;
    }
    return QSPI_OK;
}

/**
 * @brief  This function send a Write Enable and wait it is effective.
 * @param  hqspi: QSPI handle
 */
uint8_t QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Enable write operations */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = WRITE_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait for write enabling */
    s_config.Match = STATUS_WREN;
    s_config.Mask = STATUS_WREN;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction = READ_STATUS_REG_CMD;
    s_command.DataMode = QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
 * @brief  This function read the SR of the memory and wait the EOP.
 * @param  hqspi: QSPI handle
 * @param  Timeout
 */
uint8_t QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi, uint32_t Timeout)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Configure automatic polling mode to wait for memory ready */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = READ_STATUS_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    s_config.Match = 0;
    s_config.Mask = STATUS_WIP;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    if (HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, Timeout) != HAL_OK) {
        return QSPI_ERROR;
    }

    return QSPI_OK;
}

/**
 * @brief  This function reset the QSPI memory.
 * @param  hqspi: QSPI handle
 */
uint8_t QSPI_ResetMemory(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reset enable command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = RESET_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    /* Send the reset memory command */
    s_command.Instruction = RESET_MEMORY_CMD;
    if (HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return QSPI_ERROR;
    }

    /* Configure automatic polling mode to wait the memory is ready */
    if (QSPI_AutoPollingMemReady(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != QSPI_OK) {
        return QSPI_ERROR;
    }

#if 0
    uint8_t st;
    if (QSPI_GetStatus(hqspi, &st) != QSPI_OK) {
        return QSPI_ERROR;
    }

    if ((st & STATUS_QEN) == 0) {
        if (QSPI_EnableQuad(hqspi, 1) != QSPI_OK) {
            return QSPI_ERROR;
        }
    }
    if (QSPI_GetStatus(hqspi, &st) != QSPI_OK) {
        return QSPI_ERROR;
    }
    if ((st & STATUS_QEN) == 0) {
        return QSPI_ERROR;
    }
#endif

    return QSPI_OK;
}

uint8_t QSPI_MMAP_On(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd;
    QSPI_MemoryMappedTypeDef cfg;

    /* Configure the command for the read instruction */
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = QUAD_INOUT_FAST_READ_CMD;
    cmd.AddressMode = QSPI_ADDRESS_4_LINES;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.DummyCycles = QUAD_INOUT_FAST_READ_DUMMY_CYCLES;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    cmd.Address = 0;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    cmd.NbData = 0;

    /* Configure the memory mapped mode */
    cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_ENABLE;
    cfg.TimeOutPeriod = 1;

    if (HAL_QSPI_MemoryMapped(hqspi, &cmd, &cfg) != HAL_OK) {
        return QSPI_ERROR;
    }

    size_t size = 1 << hqspi->Init.FlashSize;
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)QSPI_BASE, size);

    return QSPI_OK;
}

uint8_t QSPI_MMAP_Off(QSPI_HandleTypeDef *hqspi)
{
    hqspi->Instance->CR |= QUADSPI_CR_ABORT_Msk;
    while ((hqspi->Instance->SR & QUADSPI_SR_BUSY_Msk) != 0)
        ;
    hqspi->Instance->CCR &= ~QUADSPI_CCR_FMODE_Msk;
    HAL_QSPI_DeInit(hqspi);
    HAL_QSPI_Init(hqspi);
    QSPI_ResetMemory(hqspi);
    return QSPI_OK;
}

size_t QSPI_EraseSize(void)
{
    return 4096;
}

size_t QSPI_ProgramSize(void)
{
    return 256;
}
