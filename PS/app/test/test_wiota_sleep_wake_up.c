/******************************************************************************
* @file      test_wiota_sleep_wake_up.c
* @brief     Set alarm timing to wake up in SLEEP mode
* @author    ypzhang
* @version   1.0
* @date      2023.12.01
*
* @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
*
******************************************************************************/
#include <rtthread.h>
#include "string.h"

#ifdef WIOTA_ASYNC_SLEEP_WAKE_UP_TEST
#include "uc_wiota_api.h"
#include "test_wiota_sleep_wake_up.h"

#define ASYNC_WIOTA_MCS_LEVEL 3
#define WIOTA_SEND_FAIL_CNT 3
#define ASYNC_WIOTA_MIN_SUBFRAM_NUM 3
#define ASYNC_WIOTA_MAX_SUBFRAM_NUM 10

typedef void (*recv_callback)(void *recv_data);

static void wiota_set_power(uint16_t power)
{
     uint8_t current_power = 127;
     rt_kprintf("wiota_set_power %d", power);

     if (power != current_power)
     {
          // set power
          uc_wiota_set_cur_power(power);

          current_power = power;
     }
}

static void user_recv_callback(void *recv_data)
{
     uc_recv_back_t *recv_page = recv_data;

     if (UC_OP_SUCC == recv_page->result)
     {
          rt_free(recv_page->data);
     }
}

static void wiota_init(uint16_t freq, recv_callback recv_cb)
{
     uint32_t my_user_id = 0x385e5a;
     sub_system_config_t config;

     // start wiota.
     uc_wiota_init();

     // uc_wiota_log_switch(UC_LOG_UART, user_get_wiota_log() & 0x1);

     // set wiota config
     uc_wiota_get_system_config(&config);
     config.subsystemid = 1;

     uc_wiota_set_freq_info(freq);
     
     rt_kprintf("id_len %d, symbol_length %d, pz %d, btvalue %d, spectrum_idx %d, subsystemid 0x%x \n",
                    config.id_len, config.symbol_length, config.pz, config.btvalue, config.spectrum_idx, config.subsystemid);

     uc_wiota_set_system_config(&config);

     // Set the user ID and configure it after protocol stack initialization before startup
     uc_wiota_set_userid(&my_user_id, 3);

     uc_wiota_set_crc(1);

     uc_wiota_set_data_rate(0, ASYNC_WIOTA_MCS_LEVEL);

     uc_wiota_set_detect_time(0);

     // uc_wiota_set_is_osc(uc_wiota_get_is_osc());

     uc_wiota_set_unisend_fail_cnt(WIOTA_SEND_FAIL_CNT);

     uc_wiota_set_subframe_num(ASYNC_WIOTA_MIN_SUBFRAM_NUM);

     wiota_set_power(0); // -18~22dbm

     uc_wiota_run();

     if (RT_NULL != recv_cb)
     {
          uc_wiota_register_recv_data_callback((uc_recv)recv_cb, UC_CALLBACK_NORAMAL_MSG);          
     }       
}

static void wiota_end(void)
{
     uc_wiota_exit();
}

static void manager_operation_task(void *pPara)
{
     uint16_t freq = 25;
     int32_t result = -1;
     uint32_t dest_addr = 0x385e5b;// 3694171
     uint8_t msg[14] = {"hello WIOTA!!!"};
     int32_t timeout = 60000;
     int32_t sleep_time = 5; // 5s

     wiota_init(freq, user_recv_callback);
   
     // send data
     result = uc_wiota_send_data(dest_addr, msg, 14, RT_NULL, 0, timeout, RT_NULL);
     
     if (UC_OP_SUCC == result)
     {
          rt_kprintf("uc_wiota_send_data succ\n");
     }
     else
     {
          rt_kprintf("uc_wiota_send_data fail\n");
     }

     // Set an alarm for timed wake-up
     uc_wiota_set_alarm_time(sleep_time);

     // enter sleep mode
     uc_wiota_sleep_enter(0, 0);  

     wiota_end();  
}

int manager_thread_create_task(void **thread,
                               char *name, void (*entry)(void *parameter),
                               void *parameter, uint32_t stack_size,
                               uint8_t priority,
                               uint32_t tick)
{
     *thread = rt_malloc(sizeof(struct rt_thread));
     void *start_stack = rt_malloc(stack_size * 4);

     if (RT_NULL == start_stack || RT_NULL == *thread)
     {
          return 1;
     }

     if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
     {
          return 2;
     }

     return 0;
}

int wiota_sleep_wake_up_demo(void)
{
     void *app_operation_handle = RT_NULL;

     // init task
     if (0 != manager_thread_create_task(&app_operation_handle,
                                         "wi_app",
                                         manager_operation_task,
                                         RT_NULL,
                                         512,
                                         RT_THREAD_PRIORITY_MAX / 3, 5))
     {
          rt_kprintf("manager_thread_create_task");
          return 1;
     }

     // start task
     rt_thread_startup((rt_thread_t)app_operation_handle);

     return 0;
}
#endif // WIOTA_ASYNC_SLEEP_WAKE_UP_TEST