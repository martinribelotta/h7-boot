#include "shell.h"
#include "fatfs.h"

#include "quadspi.h"
#include "qspi.h"
#include "rcc.h"

#include <stdio.h>

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
    {QSPI_BASE, ATTR_RX, 4 * 1024 * 1024, "QSPI FLASH"},
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

__attribute__((noreturn)) static void boot_to(uint32_t addr)
{
    const uint32_t *vtor = (uint32_t *)addr;
    __disable_irq();
    SCB_InvalidateDCache();
    SCB_InvalidateICache();
    NVIC->ICER[0] = 0xFFFFFFFF;
    NVIC->ICER[1] = 0xFFFFFFFF;
    NVIC->ICER[2] = 0xFFFFFFFF;
    NVIC->ICER[3] = 0xFFFFFFFF;
    NVIC->ICER[4] = 0xFFFFFFFF;
    NVIC->ICER[5] = 0xFFFFFFFF;
    NVIC->ICER[6] = 0xFFFFFFFF;
    NVIC->ICER[7] = 0xFFFFFFFF;

    NVIC->ICPR[0] = 0xFFFFFFFF;
    NVIC->ICPR[1] = 0xFFFFFFFF;
    NVIC->ICPR[2] = 0xFFFFFFFF;
    NVIC->ICPR[3] = 0xFFFFFFFF;
    NVIC->ICPR[4] = 0xFFFFFFFF;
    NVIC->ICPR[5] = 0xFFFFFFFF;
    NVIC->ICPR[6] = 0xFFFFFFFF;
    NVIC->ICPR[7] = 0xFFFFFFFF;

    SysTick->CTRL = 0;
    SysTick->LOAD = 0; // Needed?
    SysTick->VAL = 0;  // Needed?
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

    SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | //
                    SCB_SHCSR_BUSFAULTENA_Msk | //
                    SCB_SHCSR_MEMFAULTENA_Msk);
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

static void try_to_boot_qspi()
{
    MX_QUADSPI_EnterMMAP();
    if (valid_vtor(QSPI_BASE)) {
        uint32_t * const vtor = (uint32_t *const) QSPI_BASE;
        rcc_init_t *rcc_code = (rcc_init_t*)(vtor[7]);
        uint32_t rcc_code_version = vtor[8];
        rcc_init_t copy = *rcc_code;
        MX_QUADSPI_ExitMMAP();
        if (rcc_initialize(&copy, rcc_code_version) == RCC_INIT_FAIL) {
            puts("RCC Config init fail!");
            NVIC_SystemReset();
            for(;;);
        }
        if (LL_RCC_GetQSPIClockFreq(LL_RCC_QSPI_CLKSOURCE) == LL_RCC_PERIPH_FREQUENCY_NO) {
            puts("No QSPI clock... fail!");
            NVIC_SystemReset();
            for(;;);
        }
        MX_QUADSPI_Init();
        MX_QUADSPI_EnterMMAP();
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

void boot_process(void)
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

static SHELL_CMD(qspiboot, "Boot from QSPI", cmd_qspi_boot);
static SHELL_CMD(memory, "Show memory regions", print_memory_map);
static SHELL_CMD(boot, "Boot to address", cmd_boot);
static SHELL_CMD(bootf, "Forced boot to address", cmd_bootf);
static SHELL_CMD(validvtor, "Validate VTOR address", cmd_validate_vtor);
