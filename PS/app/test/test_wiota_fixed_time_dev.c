/******************************************************************************
* @file      test_wiota_fixed_time_dev.c
* @brief     Report information at a specified time
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

static rt_mq_t operation_queue_handle = RT_NULL;
static uint32_t g_my_userid = 0x385e5b;

typedef struct
{
    uint8_t type;
    uint8_t report_interval;
    uint8_t bc_interval; 
    uint32_t bc_count;
    uint32_t gw_id;
    uint32_t dev_id[2];
} bc_data_t;
typedef struct
{
    uint32_t cur_rf_cnt;
    bc_data_t *data;
} recv_mq_t;

typedef void (*recv_callback)(void *recv_data);

static void user_recv_callback(uc_recv_back_p recv_data)
{
     if (UC_OP_SUCC == recv_data->result)
     {
          recv_mq_t recv_info = {0};
          recv_info.cur_rf_cnt = recv_data->cur_rf_cnt;
          recv_info.data = (bc_data_t *)recv_data->data;

          rt_kprintf("----------user_recv_callback----------\n");

          if (RT_EOK != rt_mq_send(operation_queue_handle, &recv_info, sizeof(recv_mq_t)))
          {              
               rt_kprintf("%s line %d rt_mq_send fail\n", __FUNCTION__, __LINE__);
          }
          if (recv_data)
          {
               rt_free(recv_data->data);  
          }
          
     }

}
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

static void manager_operation_task(void *pPara)
{
     int res = 0;
     uint8_t order = 0xff;
     uint16_t freq = 25;
     uint32_t dest_addr;
     uint32_t dev_start_time = 0;
     uint8_t buff[] = {"hello dtu! my id 0x385e5b"};

     wiota_init(freq, user_recv_callback);

     // bc_data_t *transimit_data;
     recv_mq_t recv_info = {0};
    while (1)
    {
          // if (rt_mq_recv(operation_queue_handle, &transimit_data, sizeof(bc_data_t), RT_WAITING_FOREVER) == RT_EOK)
          if (rt_mq_recv(operation_queue_handle, &recv_info, sizeof(recv_mq_t), RT_WAITING_FOREVER) == RT_EOK)
          {
               order = 0xff;
               // printf recv data
               rt_kprintf("recv data : ");
               // transimit_data = (bc_data_t*)recv_info.data;
               rt_kprintf("%d %d %d %d %x %x %x\n",
                          recv_info.data->type, 
                          recv_info.data->report_interval, 
                          recv_info.data->bc_interval,  
                          recv_info.data->bc_count, 
                          recv_info.data->gw_id, 
                          recv_info.data->dev_id[0], 
                          recv_info.data->dev_id[1]);

               rt_kprintf("cur_rf_cnt = %d my id = %x \n", recv_info.cur_rf_cnt, g_my_userid);

               dest_addr = recv_info.data->gw_id;

               // This example only has two terminals
               if (recv_info.data->dev_id[0] == g_my_userid)
               {
                    order = 0;
               }
               else if (recv_info.data->dev_id[1] == g_my_userid)
               {
                    order = 1;
               }
               else
               {
                    rt_kprintf("No need to report \n");
                    continue;
               }    
               rt_kprintf("order = %d\n", order);
               
               dev_start_time = recv_info.cur_rf_cnt + recv_info.data->bc_interval * 1000 + order * recv_info.data->report_interval * 1000;
               res = uc_wiota_send_data_with_start_time(dest_addr,
                                                  &buff,
                                                  sizeof(buff),
                                                  RT_NULL,
                                                  0,
                                                  60000,
                                                  RT_NULL,
                                                  dev_start_time);
               if (res == UC_OP_SUCC)
               {
                    rt_kprintf("uc_wiota_send_data_with_start_time send success\n");
               }else if(res == UC_OP_TIMEOUT)
               {
                   rt_kprintf("uc_wiota_send_data_with_start_time send timeout\n"); 
               }
               else
               {
                    rt_kprintf("uc_wiota_send_data_with_start_time send fail\n");
               }
          }

    }

    wiota_end();
}

static int wiota_manager_create_queue(void)
{
     // create wiota app manager queue.

     operation_queue_handle = rt_mq_create("opWiota", sizeof(bc_data_t), 16, RT_IPC_FLAG_FIFO);
     RT_ASSERT(operation_queue_handle);

     return 0;
}

static int manager_thread_create_task(void **thread,
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

int wiota_device_manger_init(void)
{
     void *app_operation_handle = RT_NULL;

     // init queue
     wiota_manager_create_queue();

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