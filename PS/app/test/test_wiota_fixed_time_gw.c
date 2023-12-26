/******************************************************************************
* @file      test_wiota_fixed_time_gw.c
* @brief     Send a broadcast at a specified time
* @author    ypzhang
* @version   1.0
* @date      2023.12.06
*
* @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
*
******************************************************************************/
#include <rtthread.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#ifdef WIOTA_ASYNC_FIXED_TIME_TEST
#include "test_wiota_fixed_time.h"

static uint32_t g_my_userid = 0x385e5a;
typedef struct
{
    uint8_t type;
    uint8_t report_interval;
    uint8_t bc_interval; 
    uint32_t bc_count;
    uint32_t gw_id;
    uint32_t dev_id[2];
} bc_data_t;

typedef void (*recv_callback)(void *recv_data);

static int wiota_init(uint16_t freq, uc_recv recv_cb)
{
    sub_system_config_t config;

    // start wiota.
    uc_wiota_init();

    // set wiota config
    uc_wiota_get_system_config(&config);
    // config.subsystemid = subsystem_id;
    // uc_wiota_set_system_config(&config);
    rt_kprintf("id_len %d, symbol_length %d, pz %d, btvalue %d, spectrum_idx %d, subsystemid 0x%x",
            config.id_len, config.symbol_length, config.pz, config.btvalue, config.spectrum_idx, config.subsystemid);
    uc_wiota_set_userid(&g_my_userid, 3);

    uc_wiota_log_switch(UC_LOG_UART, 1);

    uc_wiota_set_freq_info(freq);

    uc_wiota_set_crc(1);

    uc_wiota_set_data_rate(0, ASYNC_WIOTA_MCS_LEVEL);

    uc_wiota_set_detect_time(0);

    // uc_wiota_set_is_osc(uc_wiota_get_is_osc());

    uc_wiota_set_unisend_fail_cnt(WIOTA_SEND_FAIL_CNT);

    uc_wiota_set_subframe_num(ASYNC_WIOTA_MIN_SUBFRAM_NUM);

    uc_wiota_set_bc_round(ASYNC_WIOTA_BC_ROUND);

    // rr_set_power(0);

    uc_wiota_run();

    if (RT_NULL != recv_cb)
        uc_wiota_register_recv_data_callback(recv_cb, UC_CALLBACK_NORAMAL_MSG);

    return 0;
}

static void wiota_end(void)
{
    uc_wiota_exit();
}

static void user_recv_callback(uc_recv_back_p recv_data)
{
    // data successfully received
    if (UC_OP_SUCC == recv_data->result)
    {
        rt_kprintf("----------wiota_recv_callback----------\n");
        rt_kprintf("recv data : ");
        for (uint16_t index = 0; index < recv_data->data_len; index++)
        {
            rt_kprintf("%c", *(recv_data->data + index));
        }
        rt_kprintf(", data_len %d\n", recv_data->data_len);

        if (recv_data)
        {
            rt_free(recv_data->data);  
        }
    }   
}

static void manager_operation_task(void *pPara)
{
    uint16_t freq = 25;
    int res = -1;
    uint32_t recv_id[4] = {0x385e5b, 0x385e5c};
    bc_data_t msg = {0};
    
    uint32_t start_time = 0;
    uint32_t bc_complete_time = 0;
    uint32_t delay_time = 0;

    wiota_init(freq, user_recv_callback);

    msg.type = ASYNC_BC_CMD;
    msg.report_interval = ASYNC_REPORT_INTAERVAL; // ms, dev report interval
    msg.bc_interval = ASYNC_BC_SLOT;
    msg.bc_count = 0;
    msg.gw_id = g_my_userid;  // my userid 0x385e5a
    msg.dev_id[0] = recv_id[0]; // userid 0x385e5b
    msg.dev_id[1] = recv_id[1]; // userid 0x385e5c

    while (1)
    {
        start_time = uc_wiota_get_curr_rf_cnt(); // us        

        res = uc_wiota_send_data(0, (uint8_t*)&msg, sizeof(msg), RT_NULL, 0, 60000, RT_NULL);
        bc_complete_time = uc_wiota_get_curr_rf_cnt();

        if (res == UC_OP_SUCC)
        {
            rt_kprintf("bc_count %d send success\n", msg.bc_count);
            msg.bc_count++;
        }
        else
        {
            rt_kprintf("bc_count %d send fail\n", msg.bc_count);
            msg.bc_count++;
        }             

        /******************************************************************************
        * @brief    
        * Delay starts from the completion of the previous broadcast 
        * and continues until all terminals have reported information. 
        * After the delay is complete, continue to send the next broadcast.
        * Please refer to the official website documentation for a detailed explanation.
        * 
        ******************************************************************************/
        delay_time = ASYNC_BC_SLOT + ASYNC_REPORT_INTAERVAL * ASYNC_BC_DEV_NUM - (bc_complete_time - start_time)/1000;
        rt_kprintf("start_time = %d bc_complete_time = %d delay_time = %d \n", start_time, bc_complete_time, delay_time);

        rt_thread_delay(delay_time);
    }

    wiota_end();
}

// Create task
static int manager_thread_create_task(void **thread,
                               char *name, void (*entry)(void *parameter),
                               void *parameter, uint32_t stack_size,
                               uint8_t priority,
                               uint32_t tick)
{
    // Allocate memory space
    *thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack || RT_NULL == *thread)
    {
        return 1;
    }

    // init task
    if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}

int wiota_gw_manger_init(void)
{
    void *app_operation_handle = RT_NULL;

    // init task
    if (0 != manager_thread_create_task(&app_operation_handle,
                                        "wit_app",
                                        manager_operation_task,
                                        RT_NULL,
                                        512,
                                        RT_THREAD_PRIORITY_MAX / 3 + 1, 5))
    {
        rt_kprintf("manager_thread_create_task");
        return 1;
    }

    // start task
    rt_thread_startup((rt_thread_t)app_operation_handle);

    return 0;
}
#endif // WIOTA_ASYNC_FIXED_TIME_TEST