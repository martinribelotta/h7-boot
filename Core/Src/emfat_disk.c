#include "emfat.h"

emfat_t emfat;

#define CMA_TIME EMFAT_ENCODE_CMA_TIME(1,12,2020, 13,0,0)
#define CMA { CMA_TIME, CMA_TIME, CMA_TIME }

#define DECL_READ_PROC(fname) void fname##_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata)
#define IMPL_READ_PROC(fname, array) DECL_READ_PROC(fname) { \
    int len = 0; \
	if (offset > sizeof(array)) return; \
	len = (offset + size > sizeof(array))? sizeof(array) - offset : size; \
	memcpy(dest, &array[offset], len); \
}
#define MAKE_RO_FILE_STR(fname, content) \
static const uint8_t fname##_array[] = { content }; \
static const int fname##_SIZE = sizeof(fname##_array) - 1; \
void fname##_read_proc(uint8_t *dest, int size, uint32_t offset, size_t userdata) { \
    int len = 0; \
	if (offset > fname##_SIZE) return; \
	len = (offset + size > sizeof(fname##_array))? fname##_SIZE - offset : size; \
	memcpy(dest, &fname##_array[offset], len); \
}

MAKE_RO_FILE_STR(INFO_UF2, "Content of file info_uf2.txt");
MAKE_RO_FILE_STR(INDEX, "Content of index.htm");
MAKE_RO_FILE_STR(CURRENT, "Content of current.uf2");

static emfat_entry_t entries[] =
{
	// name           dir    lvl off size           max_size       user time read                write
	{ "",             true,  0,  0,  0,             0,             0,   CMA, NULL,               NULL },
	{ "INFO_UF2.TXT", false, 1,  0,  INFO_UF2_SIZE, INFO_UF2_SIZE, 0,   CMA, INFO_UF2_read_proc, NULL },
	{ "INDEX.HTM",    false, 1,  0,  INDEX_SIZE,    INDEX_SIZE,    0,   CMA, INDEX_read_proc,    NULL },
	{ "CURRENT.UF2",  false, 1,  0,  CURRENT_SIZE,  CURRENT_SIZE,  0,   CMA, CURRENT_read_proc,  NULL },
	{ NULL }
};

void EMFATDisk_Init(void)
{
    emfat_init(&emfat, "emfat", entries);
}
