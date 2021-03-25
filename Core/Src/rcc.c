#include <rcc.h>

void rcc_initialize(const rcc_init_t *config)
{
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

    if (HAL_RCC_OscConfig(&config->RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_RCC_ClockConfig(&config->RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_RCCEx_PeriphCLKConfig(&config->PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }

    HAL_PWREx_EnableUSBVoltageDetector();
}
