
#ifndef _MM_ICC_H__
#define _MM_ICC_H__

#define DATA_ETH                    0xAA
#define DATA_CMD                    0xBB
#define MAX_ICC_SEND_LEN_DEFAULT    1600
#define MAX_ICC_DATA_LEN_DEFAULT    (1600 - sizeof (struct ty_data_header))

#ifdef  USER_MAX_ICC_LEN
#define MAX_ICC_SEND_LEN USER_MAX_ICC_LEN
#define MAX_ICC_DATA_LEN (USER_MAX_ICC_LEN - sizeof (struct ty_data_header))
#else
#define MAX_ICC_SEND_LEN MAX_ICC_SEND_LEN_DEFAULT
#define MAX_ICC_DATA_LEN MAX_ICC_DATA_LEN_DEFAULT
#endif
struct ty_data_header {
    uint32_t cmd;
    uint32_t len;
    uint32_t crc32;
};

int ty_icc_init(void);
int ty_icc_start(void);
int ty_icc_stop(void);
int ty_icc_ready(void);
int ty_icc_send_cmd(void* buf, int size);
int ty_icc_send_eth(void* buf, int size);
int ty_icc_recv(void* buf, int size);
int ty_icc_transfer_to_wifi(void* buf, int size);

#endif
