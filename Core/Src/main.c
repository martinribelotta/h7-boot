#include "main.h"
#include "fatfs.h"
#include "quadspi.h"
#include "sdmmc.h"
#include "usart.h"
#include "gpio.h"
#include "qspi.h"
#include "microrl.h"
#include "shell.h"
#include "stdio_serial.h"

#include "tusb.h"

#include <time.h>
#include <string.h>
#include <stdio.h>

void cdc_task(void);

void SystemClock_Config(void);

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
}

microrl_t mrl;

static void microrl_print(const char *s)
{
    while (*s)
        stdio_putchar(*s++);
}

static void print_sysinfo(void)
{
    static const char *const fake_argv[] = {"info"};
    shell_execute(1, fake_argv);
}

static FATFS fat;
static FIL file;

static int copy_file_to(FIL *f, void *addr)
{
    size_t readed = 0;
    while (readed < f_size(f)) {
        UINT chunk_readed;
        if (f_read(f, addr, 4096, &chunk_readed) != FR_OK) {
            return 0;
        }
        readed += chunk_readed;
        addr += chunk_readed;
    }
    return 1;
}

static int try_load_in_ram(const char *path, void *addr)
{
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return 0;
    }
    int r = copy_file_to(&file, addr);
    f_close(&file);
    return r;
}

static int try_load_in_qspi(const char *path)
{
    if (f_open(&file, path, FA_READ) != FR_OK) {
        return 0;
    }
    int readed = 0;
    uint32_t addr = 0;
    UINT chunk = QSPI_ProgramSize();
    uint8_t *buf = malloc(chunk);
    if (!buf) {
        f_close(&file);
        return 0;
    }
    MX_QUADSPI_Erase(addr, f_size(&file));
    while (readed < f_size(&file)) {
        UINT readed_chunk;
        f_read(&file, buf, chunk, &readed_chunk);
        MX_QUADSPI_Write(buf, addr, readed_chunk);
        readed += readed_chunk;
    }
    f_close(&file);
    return 1;
}

typedef enum {
    ATTR_UNMAP = 0,
    ATTR_R = (1 << 0),
    ATTR_W = (1 << 2),
    ATTR_X = (1 << 3),
    ATTR_RW = ATTR_R | ATTR_W,
    ATTR_RX = ATTR_R | ATTR_X,
    ATTR_RWX = ATTR_R | ATTR_W | ATTR_X,
} memory_attr_t;
typedef struct {
    uint32_t addr;
    memory_attr_t attr;
    uint32_t size;
    const char *name;
} memory_entry_t;

static const memory_entry_t memory_map[] = {
    {D1_DTCMRAM_BASE, ATTR_RWX, 128 * 1024, "DTCM RAM"},
    {D1_ITCMRAM_BASE, ATTR_RWX, 64 * 1024, "ITCM RAM"},
    {D1_AXISRAM_BASE, ATTR_RWX, 512 * 1024, "D1 AXI RAM"},
    {D2_AXISRAM_BASE, ATTR_RWX, 288 * 1024, "D2 AXI RAM"},
    {D2_AHBSRAM_BASE, ATTR_RWX, 288 * 1024, "D2 AHB RAM (alias D2 AXI RAM)"},
    {D3_SRAM_BASE, ATTR_RWX, 64 * 1024, "D3 RAM"},
    {FLASH_BASE, ATTR_RX, FLASH_SIZE, "FLASH"},
    {QSPI_BASE, ATTR_RWX, 4 * 1024 * 1024, "QSPI FLASH"},
};

static int memory_range(uint32_t addr, memory_attr_t attr)
{
    for (int i = 0; i < (sizeof(memory_map) / sizeof(*memory_map)); i++) {
        const memory_entry_t *e = &memory_map[i];
        uint32_t base = e->addr;
        uint32_t upper = e->addr + e->size;
        uint32_t perms = e->attr & attr;
        if ((addr <= upper) && (addr >= base) && (perms == attr)) {
            return i;
        }
    }
    return -1;
}

static int valid_vtor(uint32_t addr)
{
    volatile const uint32_t *vtor = (const uint32_t *)addr;
    uint32_t sp = vtor[0];
    uint32_t rstvect = vtor[1];
    // Check if SP is in any RW range
    if (memory_range(sp, ATTR_RW) == -1) {
        return 0;
    }
    // Check if reset vector is in any X range
    if (memory_range(rstvect, ATTR_X) == -1) {
        return 0;
    }
    // Check if vtor address have thumb bit in 1
    if ((rstvect & 1) == 0) {
        return 0;
    }
    return 1;
}

static int print_memory_map(int argc, const char *const *argv)
{
    puts("   base        top    size      attr  name");
    for (int i = 0; i < (sizeof(memory_map) / sizeof(*memory_map)); i++) {
        const memory_entry_t *e = &memory_map[i];
        char sattr[4] = {(e->attr & ATTR_R) ? 'r' : '-', (e->attr & ATTR_W) ? 'w' : '-', (e->attr & ATTR_X) ? 'x' : '-',
                         0};
        printf("0x%08lX 0x%08lX %-10ld %s  %s\n", e->addr, e->addr + e->size, e->size, sattr, e->name);
    }
    return 0;
}
static SHELL_CMD(memory, "Show memory regions", print_memory_map);

__attribute__((noreturn)) static void boot_to(uint32_t addr)
{
    const uint32_t *vtor = (uint32_t *)addr;
    __disable_irq();
    SCB->VTOR = addr;
    __set_MSP(vtor[0]);
    __set_PSP(vtor[0]);
    __set_CONTROL(0);
    void (*entry)(void) __attribute__((noreturn)) = (void *)vtor[1];
    entry();
    for (;;)
        ;
}

static int do_boot(int argc, const char *const *argv, int check)
{
    if (argc < 2) {
        printf("usage: %s <addr>\n", argv[0]);
        return -1;
    }
    uint32_t addr;
    if (sscanf(argv[1], "%li", &addr) != 1) {
        printf("cannot parse %s\n", argv[1]);
        return -1;
    }
    if (check) {
        if (!valid_vtor(addr)) {
            puts("Invalid vtor");
            return -2;
        }
    }
    boot_to(addr);
    puts("Boot fail");
    return 0;
}
static int cmd_boot(int argc, const char *const *argv)
{
    return do_boot(argc, argv, 1);
}
static int cmd_bootf(int argc, const char *const *argv)
{
    return do_boot(argc, argv, 0);
}
static SHELL_CMD(boot, "Boot to address", cmd_boot);
static SHELL_CMD(bootf, "Forced boot to address", cmd_bootf);

static int cmd_validate_vtor(int argc, const char *const *argv)
{
    if (argc < 2) {
        puts("usage: validvtor <addr>");
        return -1;
    }
    uint32_t addr;
    if (sscanf(argv[1], "%lx", &addr) != 1) {
        printf("cannot decode %s\n", argv[1]);
        return -1;
    }
    if (valid_vtor(addr)) {
        puts("Valid VTOR");
    } else {
        puts("Invalid VTOR");
    }

    return 0;
}
static SHELL_CMD(validvtor, "Validate VTOR address", cmd_validate_vtor);

static void try_to_boot_qspi()
{
    MX_QUADSPI_EnterMMAP();
    if (valid_vtor(QSPI_BASE)) {
        boot_to(QSPI_BASE);
    }
    MX_QUADSPI_ExitMMAP();
}

static int cmd_qspi_boot(int argc, const char *const *argv)
{
    puts("Booting QSPI");
    try_to_boot_qspi();
    puts("Fail!");
    return 0;
}
static SHELL_CMD(qspiboot, "Boot from QSPI", cmd_qspi_boot);

static void boot_process()
{
    if (f_mount(&fat, SDPath, 1) == FR_OK) {
        if (try_load_in_ram("axiram.bin", (void *)D1_AXISRAM_BASE)) {
            if (valid_vtor(D1_AXISRAM_BASE)) {
                boot_to(D1_AXISRAM_BASE);
            }
        }
        try_load_in_qspi("qflash.bin");
        f_mount(NULL, SDPath, 1);
    }
    try_to_boot_qspi();
}

int main(void)
{
    print_sysinfo();
    if (LL_GPIO_IsInputPinSet(USER_SWITCH_GPIO_Port, USER_SWITCH_Pin)) {
        boot_process();
    }
    microrl_init(&mrl, microrl_print);
    microrl_set_execute_callback(&mrl, shell_execute);
    tusb_init();
    while (1) {
        tud_task();
        if (LL_USART_IsActiveFlag_RXNE(USART1)) {
            int c = LL_USART_ReceiveData8(USART1);
            microrl_insert_char(&mrl, c);
        } else {
            __WFI();
        }
        cdc_task();
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


// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void)
{
  // connected() check for DTR bit
  // Most but not all terminal client set this when making connection
  // if ( tud_cdc_connected() )
  {
    // connected and there are data available
    if ( tud_cdc_available() )
    {
      uint8_t buf[64];

      // read and echo back
      uint32_t count = tud_cdc_read(buf, sizeof(buf));

      for(uint32_t i=0; i<count; i++)
      {
        tud_cdc_write_char(buf[i]);

        if ( buf[i] == '\r' ) tud_cdc_write_char('\n');
      }

      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;

  // connected
  if ( dtr )
  {
    // print initial message when connected
    tud_cdc_write_str("\r\nTinyUSB CDC MSC device example\r\n");
    tud_cdc_write_flush();
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}
