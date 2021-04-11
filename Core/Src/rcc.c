#include <rcc.h>

int rcc_initialize(rcc_init_t *config, uint32_t version)
{
    uint32_t flashLatency = FLASH_LATENCY_4;
    if (version > 0) {
        flashLatency = config->flashLatency;
        HAL_PWREx_ConfigSupply(config->configSupply);
        __HAL_PWR_VOLTAGESCALING_CONFIG(config->voltageScale);
    } else {
        HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    }

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    __HAL_RCC_PLL_PLLSOURCE_CONFIG(config->RCC_OscInitStruct.PLL.PLLSource);

    if (HAL_RCC_OscConfig(&config->RCC_OscInitStruct) != HAL_OK) {
        return RCC_INIT_FAIL;
    }

    if (HAL_RCC_ClockConfig(&config->RCC_ClkInitStruct, flashLatency) != HAL_OK) {
        return RCC_INIT_FAIL;
    }

    if (HAL_RCCEx_PeriphCLKConfig(&config->PeriphClkInitStruct) != HAL_OK) {
        return RCC_INIT_FAIL;
    }

    HAL_PWREx_EnableUSBVoltageDetector();
    return RCC_INIT_OK;
}
