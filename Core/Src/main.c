#include "main.h"
#include "fatfs.h"
#include "quadspi.h"
#include "sdmmc.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"
#include "qspi.h"
#include "microrl.h"
#include "shell.h"

#include <time.h>
#include <string.h>

void SystemClock_Config(void);
void MX_USB_HOST_Process(void);

void __premain(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Pos;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_QUADSPI_Init();
    MX_SDMMC1_SD_Init();
    MX_FATFS_Init();
    // MX_USB_HOST_Init();
}

microrl_t mrl;

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

static void microrl_print(const char *s)
{
    while (*s)
        stdio_putchar(*s++);
}

static void print_sysinfo(void)
{
    static const char *const fake_argv[] = { "info" };
    shell_execute(1, fake_argv);
}

int main(void)
{
    print_sysinfo();
    microrl_init(&mrl, microrl_print);
    microrl_set_execute_callback(&mrl, shell_execute);
    while (1) {
        // MX_USB_HOST_Process();
        if (LL_USART_IsActiveFlag_RXNE(USART1)) {
            int c = LL_USART_ReceiveData8(USART1);
            microrl_insert_char(&mrl, c);
        } else {
            __WFI();
        }
    }
}

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

void Error_Handler(void)
{
    __BKPT(0);
    while (1) {
    }
}
