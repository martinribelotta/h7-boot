/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "quadspi.h"
#include "sdmmc.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "qspi.h"

#include <time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void dumphex(const char *label, const uint8_t *ptr, size_t count)
{
    printf("%s:", label);
    for (size_t i = 0; i < count; i++) {
        if ((i % 16) == 0) {
            printf("\n  %04X ", i);
        }
        printf("%02X ", ptr[i]);
    }
    printf("\n");
}

static void fillrand(uint8_t *buf, size_t count)
{
    while (count--)
        *buf++ = rand();
}

static void testQSPI(void)
{
    static uint8_t buf[32];
    static uint8_t buf2[32];
    // Uses noinit section to RAM DFF initialization on every power cycle
    static int seed __attribute__((section(".noinit")));
    srand(seed);
    seed = rand();

    uint8_t st;
    if (QSPI_GetStatus(&hqspi, &st) == QSPI_OK) {
        printf("QSPI status: 0x%02X\n", st);
    } else {
        puts("QSPI fail on get status");
    }

    MX_QUADSPI_Erase(0, sizeof(buf));
    fillrand(buf, sizeof(buf));
    dumphex("write buffer", buf, sizeof(buf));
    MX_QUADSPI_Write(buf, 0, sizeof(buf));
    MX_QUADSPI_Read(buf2, 0, sizeof(buf2));
    dumphex("read buffer", buf2, sizeof(buf2));
    int d = memcmp(buf, buf2, sizeof(buf));
    puts(d == 0 ? "Buffer equal" : "Buffer differ");
}

static void testuSD(void)
{
    static DIR dir;
    printf("SD %s contents:\n", SDPath);
    if (f_mount(&SDFatFS, SDPath, 1) != FR_OK) {
        printf("Error mounting SD\r\n");
        return;
    }
    if (f_opendir(&dir, SDPath) == FR_OK) {
        static FILINFO inf;
        while (f_readdir(&dir, &inf) == FR_OK) {
            if (inf.fname[0] == 0)
                break;
            printf("  %s\r\n", inf.fname);
        }
    } else {
        printf("Fail to open SD\r\n");
    }
    if (f_mount(NULL, SDPath, 1) != FR_OK) {
        printf("Error unmounting SD\r\n");
    }
}

static inline void delay(long loops)
{
    do
        asm volatile("" ::: "memory");
    while (--loops);
}

static unsigned long calcbogo(unsigned long loops)
{
    unsigned long ticks = HAL_GetTick();
    delay(loops);
    ticks = HAL_GetTick() - ticks;
    if (ticks < CLOCKS_PER_SEC) {
        return 0;
    }
    return loops * 1000ULL / ticks;
}

static void testBOGOMIPS(void)
{
    for (unsigned long loops=1 << 20; loops; loops<<=1) {
        unsigned long lps = calcbogo(loops);
        if (lps) {
            unsigned long intPart = lps / 1000000;
            unsigned long fracPart = lps % 1000000;
            printf("Loops per sec: %lu - %lu.%02lu BogoMips\n",
                   lps, intPart, fracPart);
            return;
        }
    }
    puts("Cannot calculate bogoMIPS");
}


/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Pos;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    /* USER CODE END 1 */

    /* Enable
     * I-Cache---------------------------------------------------------*/
    SCB_EnableICache();

    /* Enable
     * D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_QUADSPI_Init();
    MX_SDMMC1_SD_Init();
    MX_FATFS_Init();
    // MX_USB_HOST_Init();
    /* USER CODE BEGIN 2 */
    testQSPI();
    testuSD();
    testBOGOMIPS();
    extern int core_main(void);
    puts("Running coremark");
    core_main();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */
        // MX_USB_HOST_Process();

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {
    }
    LL_PWR_ConfigSupply(LL_PWR_LDO_SUPPLY);
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
    LL_RCC_HSE_Enable();

    /* Wait till HSE is ready */
    while (LL_RCC_HSE_IsReady() != 1) {
    }
    LL_RCC_PLL_SetSource(LL_RCC_PLLSOURCE_HSE);
    LL_RCC_PLL1P_Enable();
    LL_RCC_PLL1Q_Enable();
    LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
    LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
    LL_RCC_PLL1_SetM(4);
    LL_RCC_PLL1_SetN(480);
    LL_RCC_PLL1_SetP(2);
    LL_RCC_PLL1_SetQ(20);
    LL_RCC_PLL1_SetR(2);
    LL_RCC_PLL1_Enable();

    /* Wait till PLL is ready */
    while (LL_RCC_PLL1_IsReady() != 1) {
    }

    LL_RCC_PLL2R_Enable();
    LL_RCC_PLL2_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_2_4);
    LL_RCC_PLL2_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
    LL_RCC_PLL2_SetM(4);
    LL_RCC_PLL2_SetN(110);
    LL_RCC_PLL2_SetP(2);
    LL_RCC_PLL2_SetQ(2);
    LL_RCC_PLL2_SetR(2);
    LL_RCC_PLL2_Enable();

    /* Wait till PLL is ready */
    while (LL_RCC_PLL2_IsReady() != 1) {
    }

    /* Intermediate AHB prescaler 2 when target frequency clock is higher
     * than 80 MHz */
    LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
    LL_RCC_SetSysPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_2);
    LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_2);
    LL_SetSystemCoreClock(480000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
    LL_RCC_SetQSPIClockSource(LL_RCC_QSPI_CLKSOURCE_PLL2R);
    LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC_CLKSOURCE_PLL1Q);
    LL_RCC_SetUSARTClockSource(LL_RCC_USART16_CLKSOURCE_PCLK2);
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL1Q);
}

/* USER CODE BEGIN 4 */

int stdio_getchar(void)
{
    return LL_USART_IsActiveFlag_RXNE(USART1) ? LL_USART_ReceiveData8(USART1)
                                              : -1;
}

void stdio_putchar(char c)
{
    while (!LL_USART_IsActiveFlag_TXE(USART1))
        ;
    LL_USART_TransmitData8(USART1, c);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return
     * state
     */

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, tex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF
 * FILE****/
