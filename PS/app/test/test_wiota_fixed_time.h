#ifndef __TEST_WIOTA_FIXED_TIME_H__
#define __TEST_WIOTA_FIXED_TIME_H__

// #ifdef WIOTA_ASYNC_FIXED_TIME_TEST

#define ASYNC_MAX_POWER (16)
#define ASYNC_MIN_POWER (-10)
#define ASYNC_WIOTA_MCS_LEVEL (2)
#define WIOTA_SEND_FAIL_CNT (0)
#define ASYNC_WIOTA_MIN_SUBFRAM_NUM (3)
#define ASYNC_WIOTA_BC_ROUND  (2)

#define ASYNC_BC_CMD (10)

#define ASYNC_BC_DEV_NUM (2)
#define ASYNC_REPORT_INTAERVAL (200) // ms
#define ASYNC_BC_SLOT (200) // ms

int wiota_gw_manger_init(void);
int wiota_device_manger_init(void);

// #endif // WIOTA_ASYNC_FIXED_TIME_TEST
#endif // __TEST_WIOTA_FIXED_TIME_H__