/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-26     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "test_wiota_api.h"
// #include "at.h"
#ifdef _LINUX_
#include <stdlib.h>
#include <string.h>
#else
#include "uc_string_lib.h"
#include "uc_gpio.h"
#endif


void *testTaskHandle = NULL;
void *test2TaskHandle = NULL;

const unsigned int USER_ID_LIST[7] = {0x4c00ccdb, 0xc11cc34c, 0x488c558a, 0xabe44fcb, 0x1c1138b8, 0xdba09b6f, 0x9768c6cc};
const unsigned int USER_DCXO_LIST[7] = {0x25000, 0x29000, 0x30000, 0x2d000, 0x14000, 0x30000, 0x2E000};
#define USER_IDX 3
#define TEST_DATA_MAX_SIZE 310

boolean is_need_reset = FALSE;
boolean is_need_scaning_freq = FALSE;
u8_t new_freq_idx = 0;
u32_t test_flash_addr = 0x3F000; //FLASH_OPEN_START_ADDRESS;
u8_t *test_flash_data = NULL;
u16_t test_flash_data_len = 0;

void test_send_callback(uc_send_back_p sendResult)
{
    rt_kprintf("app send result %d\n", sendResult->result);
}

void test_recv_callback(uc_recv_back_p recvData)
{
    rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

    // at_server_printfln("app recv result %d type %d len %d addr 0x%x",
    // recvData->result,recvData->type,recvData->data_len,recvData->data);

    switch (recvData->type)
    {
    case UC_RECV_MSG:
    case UC_RECV_BC:

        //            at_server_printfln("recv type %d result %d",recvData->type,recvData->result);

        break;

    default:
        rt_kprintf("Type ERROR!!!!!!!!!!!\n");
        break;
    }

    if (UC_OP_SUCC == recvData->result && recvData->data != RT_NULL)
    {
        rt_free(recvData->data);
    }
}

void test_recv_req_callback(uc_recv_back_p recvData)
{
    rt_kprintf("app recv req result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

    if (UC_OP_SUCC == recvData->result)
    {
        rt_free(recvData->data);
    }
}

void test_send_data_callback(uc_send_back_p send_back)
{
    rt_kprintf("app send data result %d addr 0x%x\n", send_back->result, send_back->oriPtr);
}

#define TEST_FREQ_SINGLE 100
extern u32_t g_tracking_succ_test_cnt;

// small demo for all auto
void app_test_main_task(void *pPara)
{
    unsigned int total;
    unsigned int used;
    unsigned int max_used;
    unsigned char *testData = rt_malloc(TEST_DATA_MAX_SIZE);
    unsigned char *testData1 = rt_malloc(20);
    unsigned int i;
    unsigned int user_id[2] = {0x6d980e0a, 0x0};
    // unsigned char user_id_len = 0;
    // unsigned char scan_data[4] = {TEST_FREQ_SINGLE, TEST_FREQ_SINGLE, TEST_FREQ_SINGLE, TEST_FREQ_SINGLE}; //{172,125,160,134}; // {172,174,176,178};
    // unsigned short scan_len = 4;
    u8_t app_count = 48;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    for (i = 0; i < TEST_DATA_MAX_SIZE; i++)
    {
        testData[i] = (i + 30) & 0x7F;
    }

    memcpy(testData, " Hello, WIoTa.", 14);

    // first of all, init wiota
    uc_wiota_init();
    //    uc_wiota_set_active_time(5);

    // test! whole config
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pn_num, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.systemid, wiota_config.subsystemid);

    wiota_config.symbol_length = 1; // 256,1024
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(1);

    if (!uc_wiota_get_is_osc())
    {
        uc_wiota_set_dcxo(0x36000);
    }

    // test! freq test
    //    uc_wiota_set_freq_info(85);    // 470+0.2*100=490
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id,4);

    uc_wiota_set_cur_power(-16);
    uc_wiota_set_max_power(17);

    // after config set, run wiota !
    uc_wiota_run();

    rt_thread_mdelay(100);

    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    rt_kprintf("app test data addr 0x%x 0x%x\n", testData, testData1);

    while (1)
    {
        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("total %d used %d maxused %d \n", total, used, max_used);
        //        rt_kprintf("dfe test %d \n",l1_read_dfe_counter());

        rt_thread_mdelay(5000);

        memcpy(&(testData[0]), &app_count, 1);
        app_count++;
        if (app_count > 57)
        {
            app_count = 48;
        }

        // bc
        uc_wiota_send_data(0, testData, 2, 10000, NULL);
        //        uc_wiota_send_data(0, testData, 2, 10000, test_send_data_callback);

        rt_thread_mdelay(4000);
    }

    rt_free(testData);

    return;
}

int uc_thread_create_test(void **thread,
                          char *name, void (*entry)(void *parameter),
                          void *parameter, unsigned int stack_size,
                          unsigned char priority,
                          unsigned int tick)
{
    *thread = rt_malloc(sizeof(struct rt_thread));
    void *start_stack = rt_malloc(stack_size * 4);

    if (RT_NULL == start_stack)
    {
        return 1;
    }
    if (RT_EOK != rt_thread_init(*thread, name, entry, parameter, start_stack, stack_size * 4, priority, tick))
    {
        return 2;
    }

    return 0;
}


void app_task_init(void)
{
    uc_thread_create_test(&testTaskHandle, "test1", app_test_main_task, NULL, 256, 3, 3);
    rt_thread_startup((rt_thread_t)testTaskHandle);
}
