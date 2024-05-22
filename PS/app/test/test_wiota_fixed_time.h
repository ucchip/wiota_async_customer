#ifndef __TEST_WIOTA_FIXED_TIME_H__
#define __TEST_WIOTA_FIXED_TIME_H__

// #ifdef WIOTA_ASYNC_FIXED_TIME_TEST

#define ASYNC_MAX_POWER (16)
#define ASYNC_MIN_POWER (-10)
#define ASYNC_WIOTA_MCS_LEVEL (2)
#define WIOTA_SEND_FAIL_CNT (1)
#define ASYNC_WIOTA_MIN_SUBFRAM_NUM (7)
#define ASYNC_WIOTA_BC_ROUND  (1)

#define ASYNC_BC_CMD (10)

#define ASYNC_BC_DEV_NUM (15)
#define ASYNC_REPORT_INTAERVAL (60) // ms
#define ASYNC_BC_SLOT (70) // ms

typedef struct
{
    uint8_t type;
    uint8_t report_interval;
    uint8_t bc_interval;
    uint8_t reserved;
    uint32_t bc_count;
    uint32_t gw_id;
    uint32_t dev_id[2];
} bc_data_t;

int wiota_gw_manger_init(void);
int wiota_device_manger_init(void);

// #endif // WIOTA_ASYNC_FIXED_TIME_TEST
#endif // __TEST_WIOTA_FIXED_TIME_H__