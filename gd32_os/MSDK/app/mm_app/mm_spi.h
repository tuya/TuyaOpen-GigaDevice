
enum {
    ISR_READ = 5,
    ISR_READ_DONE = 6,
    ISR_WRITE_DONE = 7,
};

typedef struct mm_msg {
    int type;
    unsigned int len;
    unsigned int crc32;
    unsigned char* data;
}mm_msg_t;

typedef int (*cmd_from_spi_cb_t)( unsigned char* buf, unsigned int len, unsigned int crc32 );

/* ---- SPI hex dump — thread context only, placed after spi_isr_out() ACK ---- */
#define TUYA_SPI_DUMP    0
#define SPI_DUMP_MAX_LEN 256U

#if defined(TUYA_SPI_DUMP) && (TUYA_SPI_DUMP == 1)
static void spi_dump_hex(const char *tag, const uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0U) {
        return;
    }
    static const char HEX[] = "0123456789ABCDEF";
    uint32_t dump_len = (len > SPI_DUMP_MAX_LEN) ? SPI_DUMP_MAX_LEN : len;
    char line[16U * 3U + 1U]; /* 16 bytes × "XX " + '\0' = 49 bytes */
    uint32_t row, col, pos;
    printf("[SPI-DUMP %s] len=%u\r\n", tag, (unsigned)dump_len);
    for (row = 0U; row < dump_len; row += 16U) {
        uint32_t n = ((dump_len - row) < 16U) ? (dump_len - row) : 16U;
        pos = 0U;
        for (col = 0U; col < n; col++) {
            uint8_t b = buf[row + col];
            line[pos++] = HEX[b >> 4];
            line[pos++] = HEX[b & 0x0FU];
            line[pos++] = ' ';
        }
        line[pos > 0U ? pos - 1U : 0U] = '\0'; /* trim trailing space */
        printf("[%04X] %s\r\n", (unsigned)row, line);
    }
}
#define SPI_DUMP(tag, buf, len) spi_dump_hex((tag), (const uint8_t *)(buf), (uint32_t)(len))
#else
#define SPI_DUMP(tag, buf, len)                                                                                        \
    do {                                                                                                               \
    } while (0)
#endif


void mm_spi_deinit( void );
int mm_spi_init(void);
int mm_spi_reset(void);
int cmd_to_spi( unsigned char* buf, unsigned int len, unsigned int crc32 );
int packet_to_spi( unsigned char* buf, unsigned int len );
void mm_spi_start( void );
void mm_spi_stop( void );
