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
#ifdef WIOTA_API_TEST
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "test_wiota_api.h"
#ifdef _LINUX_
#include <stdlib.h>
#include <string.h>
#else
#include "uc_string_lib.h"
#include "uc_gpio.h"
#endif

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#endif

void *testTaskHandle = NULL;
void *test2TaskHandle = NULL;

const unsigned int USER_ID_LIST[7] = {0x4c00ccdb, 0xc11cc34c, 0x488c558a, 0xabe44fcb, 0x1c1138b8, 0xdba09b6f, 0x9768c6cc};
const unsigned int USER_DCXO_LIST[7] = {0x25000, 0x29000, 0x30000, 0x2d000, 0x14000, 0x30000, 0x2E000};
#define USER_IDX 3
#define TEST_DATA_MAX_SIZE 1024
#define TEST_FREQ_SINGLE 118
u8_t g_queue_cnt = 0;
u8_t g_recv_cnt = 0;

boolean is_need_reset = FALSE;
boolean is_need_scaning_freq = FALSE;
u8_t new_freq_idx = 0;
u32_t test_flash_addr = 0x3F000; //FLASH_OPEN_START_ADDRESS;
u8_t *test_flash_data = NULL;
u16_t test_flash_data_len = 0;

void test_send_callback(uc_send_back_p sendResult)
{
    g_queue_cnt--;
    rt_kprintf("app send result %d data 0x%x\n", sendResult->result, sendResult->oriPtr);
    // rt_free(sendResult->oriPtr);
}

void test_recv_callback(uc_recv_back_p recvData)
{
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    if (recvData->data != RT_NULL && (UC_OP_SUCC == recvData->result || UC_OP_PART_SUCC == recvData->result || UC_OP_CRC_FAIL == recvData->result))
    {
        at_server_printf("app recv,%d,%d,%d,", recvData->result, recvData->type, recvData->data_len);
        at_send_data(recvData->data, recvData->data_len);
        at_server_printfln("");
    }
    else
    {
        at_server_printfln("app recv,%d,%d,%d", recvData->result, recvData->type, recvData->data_len);
    }
#endif
#endif
    rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
               recvData->result, recvData->type, recvData->data_len, recvData->data);

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

    if (recvData->data != RT_NULL && (UC_OP_SUCC == recvData->result || UC_OP_PART_SUCC == recvData->result || UC_OP_CRC_FAIL == recvData->result))
    {
        g_recv_cnt = 1;
        rt_free(recvData->data);
    }
}

void test_send_data_callback(uc_send_back_p send_back)
{
    rt_kprintf("app send data result %d addr 0x%x\n", send_back->result, send_back->oriPtr);
}

void app_test_case_recv(void)
{
    unsigned short send_len = 70;
    unsigned char *testData = rt_malloc(send_len + 2);
    unsigned int i;
    unsigned int user_id[2] = {0x12345678, 0x0}; // recv use
    unsigned int dest_user_id = 0x87654321;      // recv use
    u8_t app_count = 48;
    int result = 0;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    memcpy(testData, " Hello, WIoTa.", 14);

    // lpm test!
    // uc_wiota_log_switch(0, 0);   // close wiota log
    // uc_wiota_set_lpm_mode(1);    // enter lpm mode

    // first of all, init wiota
    uc_wiota_init();

    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.systemid, wiota_config.subsystemid);

    wiota_config.symbol_length = 1; // 256,1024
    wiota_config.pz = 4;    // for lpm test
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(0);
    if (!uc_wiota_get_is_osc())
    {
        uc_wiota_set_dcxo(0x1F000);
        // uc_wiota_set_dcxo(0x20000);
    }

    // set freq
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id, 4);

    uc_wiota_set_cur_power(21);

    uc_wiota_set_data_rate(0, 0);

    uc_wiota_set_subframe_num(4);

    uc_wiota_set_bandwidth(4);

    // uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);  // no recv

    // uc_wiota_set_detect_time(50); // for send quick
    // uc_wiota_set_bc_round(6); // for send more
    // uc_wiota_set_bc_round(1); // default 3

    // rt_kprintf("frame len %d\n",uc_wiota_get_frame_len(0));

    uc_wiota_set_tx_mode(0);

    rt_thread_mdelay(100);

    // after config set, run wiota !
    uc_wiota_run();

    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    rt_thread_mdelay(100);
    g_recv_cnt = 0;

    while (1)
    {
        rt_thread_mdelay(10000);
        // wait recv
    }

    while (1)
    {
        rt_thread_mdelay(10000);
        if (1 == g_recv_cnt)
        {
            rt_thread_mdelay(100);
            g_recv_cnt = 0;
            app_count++;
            if (app_count > 57)
            {
                app_count = 48;
            }
            algo_srand(l1_read_dfe_counter());
            for (i = 0; i < send_len; i++)
            {
                testData[i] = algo_rand() & 0xFF;
            }
            result = uc_wiota_send_data(0, testData, send_len, 10000, NULL);
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
            at_server_printfln("app send data result %d cnt %d", result, app_count - 48);
#endif
#endif
        }
    }

    rt_free(testData);

    return;
}

void app_test_case_send(void)
{
    unsigned short send_len = 20;
    unsigned char *testData = rt_malloc(send_len + 2);
    unsigned int i;
    unsigned int user_id[2] = {0x87654321, 0x0};    // send use
    unsigned int dest_user_id = 0x12345678;         // send use
    u8_t app_count = 48;
    int result = 0;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    memcpy(testData, " Hello, WIoTa.", 14);

    // lpm test!
    uc_wiota_log_switch(0, 0);   // close wiota log
    uc_wiota_set_lpm_mode(1);    // enter lpm mode

    // first of all, init wiota
    uc_wiota_init();

    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.systemid, wiota_config.subsystemid);

    wiota_config.symbol_length = 1; // 256,1024
    wiota_config.pz = 4;    // for lpm test
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(0);
    if (!uc_wiota_get_is_osc())
    {
        uc_wiota_set_dcxo(0x1F000);
        // uc_wiota_set_dcxo(0x20000);
    }

    // set freq
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id, 4);

    uc_wiota_set_cur_power(21);

    uc_wiota_set_data_rate(0, 0);

    uc_wiota_set_subframe_num(4);

    uc_wiota_set_bandwidth(4);

    uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);  // no recv

    // uc_wiota_set_detect_time(50); // for send quick
    // uc_wiota_set_bc_round(6); // for send more
    // uc_wiota_set_bc_round(1); // default 3

    // rt_kprintf("frame len %d\n",uc_wiota_get_frame_len(0));

    uc_wiota_set_tx_mode(0);

    rt_thread_mdelay(100);

    // after config set, run wiota !
    uc_wiota_run();

    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    rt_thread_mdelay(100);
    g_recv_cnt = 0;

    algo_srand(l1_read_dfe_counter());
    for (i = 0; i < send_len; i++)
    {
        testData[i] = algo_rand() & 0xFF;
    }
    result = uc_wiota_send_data(0, testData, send_len, 10000, test_send_callback);
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    at_server_printfln("app send data result %d cnt %d", result);
#endif
#endif

    rt_thread_delay(3000);
    uc_wiota_set_alarm_time(3); // wakeup in 3s
    rt_thread_delay(100);
    uc_wiota_sleep_enter(0);    // sleep, not open ex wk

    while (0)
    {
        rt_thread_mdelay(1);
        if (1 == g_recv_cnt)
        {
            rt_thread_mdelay(100);
            g_recv_cnt = 0;
            app_count++;
            if (app_count > 57)
            {
                app_count = 48;
            }
            algo_srand(l1_read_dfe_counter());
            for (i = 0; i < send_len; i++)
            {
                testData[i] = algo_rand() & 0xFF;
            }
            result = uc_wiota_send_data(0, testData, send_len, 10000, NULL);
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
            at_server_printfln("app send data result %d cnt %d", result, app_count - 48);
#endif
#endif
        }
    }

    rt_free(testData);

    return;
}

// small demo for all auto
void app_test_main_task(void *pPara)
{
    // app_test_case_recv();
    app_test_case_send();
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
#endif