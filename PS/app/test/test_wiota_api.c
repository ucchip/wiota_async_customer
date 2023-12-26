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
#include "uc_rtc.h"
#include "uc_uart.h"
#include "uc_event.h"
#endif

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#endif

void *testTaskHandle = NULL;
// void *test2TaskHandle = NULL;

// void *test1TaskHandle = NULL;
// void *test2TaskHandle = NULL;
// void *test3TaskHandle = NULL;
// void *test4TaskHandle = NULL;
// void *test5TaskHandle = NULL;
// void *test6TaskHandle = NULL;
// void *test7TaskHandle = NULL;
// void *test8TaskHandle = NULL;

const unsigned int USER_ID_LIST[7] = {0x4c00ccdb, 0xc11cc34c, 0x488c558a, 0xabe44fcb, 0x1c1138b8, 0xdba09b6f, 0x9768c6cc};
const unsigned int USER_DCXO_LIST[7] = {0x25000, 0x29000, 0x30000, 0x2d000, 0x14000, 0x30000, 0x2E000};
#define USER_IDX 3
#define TEST_DATA_MAX_SIZE 1024
#define TEST_FREQ_SINGLE 125
unsigned char g_send_cnt = 0;
unsigned char g_recv_cnt = 0;

unsigned char is_need_reset = FALSE;
unsigned char is_need_scaning_freq = FALSE;
unsigned char new_freq_idx = 0;
unsigned int test_flash_addr = 0x3F000; //FLASH_OPEN_START_ADDRESS;
unsigned char *test_flash_data = NULL;
unsigned short test_flash_data_len = 0;

rt_base_t test_led_pin = 7;
// int led_value = 1;
unsigned char g_voice_acc_start = FALSE;
unsigned char g_voice_data_handle = FALSE;

// void uart1_handler(void)
// {
//     // unsigned int rx = UART_ITR_RD | UART_ITR_RT;
//     // unsigned int src = *(volatile unsigned int*)UART1_REG_IIR;

//     // if (rx & src)
//     {
// #ifdef _L1_FACTORY_FUNC_
//         l1_factory_irq_entry();
// #endif
//     }
//     ICP |= (1 << UART1_INT_ID);
// }

void test_send_callback(uc_send_back_p sendResult)
{
    g_send_cnt--;
    rt_kprintf("app send result %d data 0x%x\n", sendResult->result, sendResult->oriPtr);
    // rt_free(sendResult->oriPtr);
}

void test_recv_callback(uc_recv_back_p recvData)
{
#ifdef UC8288_MODULE
#ifdef RT_USING_AT
    if (recvData->data != RT_NULL && (UC_OP_SUCC == recvData->result || UC_OP_PART_SUCC == recvData->result || UC_OP_CRC_FAIL == recvData->result))
    {
        // at_server_printf("app recv,%d,%d,%d,", recvData->result, recvData->type, recvData->data_len);
        // at_send_data(recvData->data, recvData->data_len);
        // at_server_printfln("");
    }
    else
    {
        at_server_printfln("app recv,%d,%d,%d", recvData->result, recvData->type, recvData->data_len);
    }
#endif
#endif
    // rt_kprintf("app recv result %d type %d len %d addr 0x%x\n",
    //            recvData->result, recvData->type, recvData->data_len, recvData->data);

    switch (recvData->type)
    {
    case UC_RECV_MSG:
    case UC_RECV_BC:

        //            at_server_printfln("recv type %d result %d",recvData->type,recvData->result);

        break;

    case UC_RECV_SYNC_STATE:
        break;

    case UC_RECV_ACC_DATA:
        g_voice_data_handle = FALSE;
        break;

    default:
        // rt_kprintf("Type ERROR!!!!!!!!!!!\n");
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
    unsigned short send_len = 204;
    unsigned char *testData = rt_malloc(send_len + 2);
    // unsigned int i;
    unsigned int user_id[2] = {0x12345678, 0x0}; // recv use
    // unsigned int dest_user_id = 0x87654321;      // recv use
    unsigned char app_count = 48;
    int result = 0;
    // unsigned char send_type = 2;
    uc_subf_data_t subf_data = {0};
    // unsigned int subf_data_num = 0;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    // memcpy(testData, " Hello, WIoTa.", 14);

    // lpm test!
    // uc_wiota_log_switch(0, 0);   // close wiota log
    // uc_wiota_set_lpm_mode(1);    // enter lpm mode

    rt_pin_mode(test_led_pin, PIN_MODE_OUTPUT);
    rt_pin_write(test_led_pin, 0);

    // first of all, init wiota
    uc_wiota_init();

    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.freq_idx, wiota_config.subsystemid);

    wiota_config.symbol_length = 0; // 256,1024
    wiota_config.bandwidth = 4;
    wiota_config.pz = 8; // for lpm test
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(1);
    if (!uc_wiota_get_is_osc())
    {
        // uc_wiota_set_dcxo(0x1F000);
        uc_wiota_set_dcxo(0x20000);
    }

    // set freq
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id, 4);

    uc_wiota_set_cur_power(21);

    uc_wiota_set_data_rate(0, 2);

    uc_wiota_set_subframe_num(8);

    // uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);  // no recv

    uc_wiota_set_detect_time(0); // for send quick
    // uc_wiota_set_bc_round(6); // for send more
    uc_wiota_set_bc_round(1); // default 3

    // rt_kprintf("frame len %d\n",uc_wiota_get_frame_len(0));

    // uc_wiota_set_tx_mode(0);

    rt_thread_mdelay(100);

    uc_wiota_set_subframe_recv(TRUE);

    // after config set, run wiota !
    uc_wiota_run();

    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_STATE_INFO);

    rt_thread_mdelay(100);
    g_recv_cnt = 0;

    while (1)
    {
        rt_thread_mdelay(100);
        // wait recv
        if (1 == g_recv_cnt)
        {
            at_server_printf("recv subf %d r %d data %d,", uc_wiota_get_subframe_data_num(TRUE), subf_data.is_right, subf_data.data_len);
            at_send_data(subf_data.data, subf_data.data_len);
            at_server_printfln("");
            rt_free(subf_data.data);
        }
    }

    while (0)
    {
        rt_thread_mdelay(1);
        if (1 == g_recv_cnt)
        {
            rt_thread_mdelay(20);
            uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);
            g_recv_cnt = 0;
            app_count++;
            if (app_count > 57)
            {
                app_count = 48;
            }
            // algo_srand(uc_wiota_get_curr_rf_cnt());
            // for (i = 0; i < send_len; i++)
            // {
            //     testData[i] = algo_rand() & 0xFF;
            // }
            result = uc_wiota_send_data(0, testData, send_len, NULL, 0, 10000, NULL);
            // if (send_type == 0)
            // {
            //     send_type = 1;
            //     result = uc_wiota_send_data(0, testData, send_len, NULL, 0, 10000, NULL);
            // }
            // else
            // {
            //     send_type = 0;
            //     result = uc_wiota_send_data(dest_user_id, testData, send_len, NULL, 0, 10000, NULL);
            // }
#if defined(UC8288_MODULE) && defined(RT_USING_AT)
            at_server_printfln("app send data result %d cnt %d", result, app_count - 48);
#endif
            uc_wiota_set_recv_mode(UC_AUTO_RECV_START);
        }
    }

    rt_free(testData);

    return;
}

// unsigned char gTestData[258] = {0};

void app_test_case_send(void)
{
    unsigned short send_len = 204;
    unsigned char *testData = rt_malloc(send_len + 2);
    // unsigned char testData[256] = {0};
    // unsigned char testIdx = 0;
    unsigned int i;
    unsigned int user_id[2] = {0x87654321, 0x0}; // send use
    // unsigned int dest_user_id = 0x12345678;         // send use
    unsigned char app_count = 48;
    // int result = 0;
    // unsigned char send_type = 0;
    // unsigned char head_data[UC_USER_HEAD_SIZE] = {1, 2};
    // unsigned int dfe_cnt = 0;
    unsigned char subframe_data_head = 3;

    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};

    rt_thread_delay(1000);

    // memcpy(testData, " Hello, WIoTa.", 14);

    // lpm test!
    // uc_wiota_log_switch(0, 0);   // close wiota log
    // uc_wiota_set_lpm_mode(1);    // enter lpm mode

    // first of all, init wiota
    uc_wiota_init();

    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.freq_idx, wiota_config.subsystemid);

    wiota_config.symbol_length = 0; // 256,1024
    wiota_config.bandwidth = 4;
    wiota_config.subsystemid = 0x21456981;
    // wiota_config.pz = 4; // for lpm test
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(1);
    if (!uc_wiota_get_is_osc())
    {
        // uc_wiota_set_dcxo(0x1F000);
        uc_wiota_set_dcxo(0x20000);
    }

    // set freq
    uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
                                              //    uc_wiota_set_freq_info(150);    // 470+0.2*150=500
                                              //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id, 4);

    uc_wiota_set_cur_power(21);

    uc_wiota_set_data_rate(0, 2);

    uc_wiota_set_subframe_num(8);

    uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP); // no recv

    uc_wiota_set_detect_time(0); // for send quick
    // uc_wiota_set_bc_round(6); // for send more
    uc_wiota_set_bc_round(1); // default 3

    // rt_kprintf("frame len %d\n",uc_wiota_get_frame_len(0));

    // uc_wiota_set_tx_mode(0);

    // after config set, run wiota !
    uc_wiota_run();

    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_STATE_INFO);

    // uc_wiota_set_incomplete_recv(TRUE);

    rt_thread_mdelay(2000);

    //     algo_srand(uc_wiota_get_curr_rf_cnt());
    //     for (i = 0; i < send_len; i++)
    //     {
    //         testData[i] = algo_rand() & 0xFF;
    //     }
    //     result = uc_wiota_send_data(0, testData, send_len, 10000, NULL, 0, test_send_callback);
    // #ifdef UC8288_MODULE
    // #ifdef RT_USING_AT
    //     at_server_printfln("app send data result %d cnt %d", result);
    // #endif
    // #endif

    g_voice_acc_start = TRUE;

    // algo_srand(uc_wiota_get_curr_rf_cnt());
    for (i = 0; i < send_len; i++)
    {
        testData[i] = i;
    }

    unsigned int frame_len = uc_wiota_get_frame_len(0);
    uc_subf_data_t subf_data;
    unsigned char data_test[52] = {0}; // if malloc, need free after add_subframe_data

    for (i = 0; i < 52; i++)
    {
        data_test[i] = i;
    }

    data_test[0] = app_count;

    subf_data.data_len = 52;
    subf_data.data = data_test;

    for (i = 0; i < 8; i++)
    {
        uc_wiota_add_subframe_data(&subf_data);
        app_count++;
        if (app_count > 57)
        {
            app_count = 48;
        }
        data_test[0] = app_count;
    }

    uc_wiota_set_subframe_head(subframe_data_head);

    uc_wiota_set_subframe_send(1);

    while (1)
    {
        // frame_len = uc_wiota_get_frame_len(0);
        // rt_thread_delay(240);
        rt_thread_delay(frame_len);

        // algo_srand(l1_read_dfe_counter());
        // for (i = 0; i < 10; i++)
        // {
        //     testData[0][i] = algo_rand() & 0xFF;
        //     // testData[0][i] = 0;
        // }

        for (i = 0; i < 8; i++)
        {
            uc_wiota_add_subframe_data(&subf_data);
            app_count++;
            if (app_count > 57)
            {
                app_count = 48;
            }
            data_test[0] = app_count;
        }

        app_count++;
        if (app_count > 57)
        {
            app_count = 48;
        }
        // if (app_count > 255)
        // {
        //     rt_thread_mdelay(10000);
        //     total_send_cnt = 0;
        // }

        // for (i = 0; i < app_count; i++)
        // {
        //     gTestData[i] = i;
        // }
        // gTestData[app_count] = 0;
        // gTestData[app_count + 1] = 0;

        // testData[0][0] = app_count;

        // testData[0] = app_count;
        // uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);
        // result = uc_wiota_send_data(0, testData, send_len, NULL, 0, 10000, NULL);
        // uc_wiota_set_recv_mode(UC_AUTO_RECV_START);
        // rt_thread_mdelay(1000);

        // result = uc_wiota_send_data(dest_user_id, testData[0], send_len, NULL, 0, 10000, NULL);

        // dfe_cnt = l1_read_dfe_counter();
        // result = uc_wiota_send_data(0, gTestData, app_count, NULL, 0, 10000, NULL);
        // rt_kprintf("use time %u result %d\n", l1_read_dfe_counter() - dfe_cnt, result);
        // rt_thread_delay(100);
        // result = uc_wiota_send_data(dest_user_id, testData[0], 10, NULL, 0, 10000, NULL);
        // rt_thread_mdelay(10);

        // at_server_printfln("app send data result %d cnt %d, num %d", result, app_count - 48, uc_wiota_get_subframe_data_num(FALSE));
    }

    // #if defined(UC8288_MODULE) && defined(RT_USING_AT)
    //     at_server_printfln("app send data result %d cnt %d", result);
    // #endif

    rt_thread_delay(3000);
    uc_wiota_set_alarm_time(10); // wakeup in 3s
    rt_thread_delay(2);
    uc_wiota_sleep_enter(0, 0); // sleep, not open ex wk

    // algo_srand(l1_read_dfe_counter());
    // for (i = 0; i < send_len; i++)
    // {
    //     testData[0][i] = algo_rand() & 0xFF;
    //     testData[1][i] = algo_rand() & 0xFF;
    // }

    // uc_wiota_set_continue_send(1);
    // uc_wiota_set_incomplete_recv(1);

    //     while (0)
    //     {
    //         // rt_thread_mdelay(3000);
    //         // rt_thread_mdelay(2);
    //         // if (g_send_cnt < 2 && total_send_cnt < 20)
    //         if (g_send_cnt < 2)
    //         {
    //             g_send_cnt++;
    //             // total_send_cnt++;

    //             head_data[0] += 1;
    //             head_data[1] += 1;

    //             app_count++;
    //             if (app_count > 57)
    //             {
    //                 app_count = 48;
    //             }

    //             testData[testIdx][0] = app_count;

    //             result = uc_wiota_send_data(0, testData[testIdx], send_len, head_data, 3, 5100, test_send_callback);

    // #if defined(UC8288_MODULE) && defined(RT_USING_AT)
    //             at_server_printfln("app send data result %d cnt %d", result, testIdx);
    // #endif
    //             if (0 == testIdx)
    //             {
    //                 testIdx = 1;
    //             }
    //             else
    //             {
    //                 testIdx = 0;
    //             }
    //         }

    //         rt_thread_mdelay(2);

    //         // if (total_send_cnt == 20)
    //         // {
    //         //     rt_thread_mdelay(10000);
    //         //     total_send_cnt = 0;
    //         // }
    //     }

    //     rt_free(testData);

    return;
}

void app_test_case_send_always(void)
{
    sub_system_config_t wiota_config = {0};

    uc_wiota_init();
    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.freq_idx, wiota_config.subsystemid);

    wiota_config.symbol_length = 0; // 256,1024
    wiota_config.bandwidth = 1;
    wiota_config.pz = 8;
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(1);

    uc_wiota_set_freq_info(115); // 470+0.2*100=490

    uc_wiota_set_cur_power(21);

    // uc_wiota_set_adjust_mode(ADJUST_MODE_SEND);

    uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP); // no recv

    rt_thread_mdelay(1);

    // after config set, run wiota !
    uc_wiota_run();

    uc_wiota_set_subframe_send(1);

    while (1)
    {
        rt_thread_mdelay(10000);
    }

    return;
}

void app_test_rf_lpm(void)
{
    unsigned int user_id[2] = {0x87654321, 0x0}; // send use
    unsigned int dest_user_id = 0x12345678;      // send use
    //    uc_recv_back_t recv_result_t;
    sub_system_config_t wiota_config = {0};
    unsigned char testData[2][35] = {0};
    uc_lpm_rx_cfg_t config = {0};

    // memcpy(testData, " Hello, WIoTa.", 14);

    // lpm test!
    // uc_wiota_log_switch(0, 0); // close wiota log
    // uc_wiota_set_lpm_mode(1);  // enter lpm mode

    // first of all, init wiota
    uc_wiota_init();

    // config get
    uc_wiota_get_system_config(&wiota_config);
    rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
               wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
               wiota_config.btvalue, wiota_config.freq_idx, wiota_config.subsystemid);

    wiota_config.symbol_length = 1; // 256,1024
    wiota_config.bandwidth = 1;
    wiota_config.pz = 8; // for lpm test
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_set_is_osc(1);
    if (!uc_wiota_get_is_osc())
    {
        // uc_wiota_set_dcxo(0x1F000);
        uc_wiota_set_dcxo(0x20000);
    }

    // set freq
    // uc_wiota_set_freq_info(TEST_FREQ_SINGLE); // 470+0.2*100=490
    uc_wiota_set_freq_info(55); // 470+0.2*150=500
    //    rt_kprintf("test get freq point %d\n", uc_wiota_get_freq_info());

    uc_wiota_set_userid(user_id, 4);

    uc_wiota_set_cur_power(17);

    uc_wiota_set_data_rate(0, 0);

    uc_wiota_set_subframe_num(3);

    // uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP); // no recv

    // uc_wiota_set_detect_time(50); // for send quick
    // uc_wiota_set_bc_round(6); // for send more
    uc_wiota_set_bc_round(4); // default 3

    // rt_kprintf("frame len %d\n",uc_wiota_get_frame_len(0));

    // uc_wiota_set_tx_mode(0);

    // rt_thread_mdelay(100);

    // after config set, run wiota !
    uc_wiota_run();

    // uc_wiota_register_recv_data_callback(test_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    // uc_wiota_set_lpm_mode(1);

    // uc_wiota_set_freq_div(FREQ_DIV_MODE_8);
    // uc_wiota_set_vol_mode(1);

    // rt_thread_delay(3000);
    // uc_wiota_set_alarm_time(3); // wakeup in 3s
    // rt_thread_delay(1000);

    // rt_thread_mdelay(2000);

    while (1)
    {
        rt_thread_mdelay(2000);
        uc_wiota_set_gating_config(10, 200); // 10 seconds, 200ms
        uc_wiota_set_is_gating(1, 0);

        while (1)
        {
            rt_thread_mdelay(2000);
            if (!uc_wiota_get_is_gating())
            {
                // uc_wiota_set_recv_mode(UC_AUTO_RECV_START);
                break;
            }
        }
    }

    uc_wiota_get_paging_rx_cfg(&config);
    config.freq = 57;
    config.spectrum_idx = 3;
    config.symbol_length = 1;
    config.bandwidth = 1;
    config.threshold = 10;
    config.awaken_id = 23;
    config.detect_period = 100;
    config.extra_flag = 1;
    config.extra_period = 100;
    uc_wiota_set_paging_rx_cfg(&config);

    uc_wiota_paging_rx_enter(0, 0);

    // uc_wiota_paging_tx_start();

    // while (1)
    // {

    //     rt_thread_mdelay(2000);

    // }

    // uc_wiota_log_switch(0, 0); // close wiota log
    // uc_wiota_set_lpm_mode(1);  // enter lpm mode

    // while (1)
    {
        // algo_srand(l1_read_dfe_counter());
        // for (int i = 0; i < 20; i++)
        // {
        //     testData[0][i] = algo_rand() & 0xFF;
        // }
        uc_wiota_send_data(dest_user_id, testData[0], 6, NULL, 0, 5100, NULL);

        rt_thread_mdelay(1000);

        uc_wiota_send_data(0, testData[0], 6, NULL, 0, 5100, NULL);

        rt_thread_mdelay(2000);
    }

    rt_thread_delay(3000);
    uc_wiota_set_alarm_time(3); // wakeup in 3s
    rt_thread_delay(2);
    uc_wiota_sleep_enter(0, 1); // sleep, not open ex wk

    // uc_wiota_paging_rx_enter(1, 0);

    while (1)
    {
        rt_thread_mdelay(1000);
    }

    rt_thread_mdelay(100);

    return;
}

extern unsigned int rtc_end_dfe;
extern unsigned int rtc_handle_cnt;

void app_test_rtc_32k(void)
{
    // unsigned int last_cnt = 0xFFFFFFFF;
    // unsigned int last_dfe = 0;
    volatile unsigned int *PMC = (unsigned int *)0x1a10a000;

    *(PMC) |= (1 << 4);

    rtc_handle_cnt = 0;

    // rtc_enable_alarm_interrupt(UC_RTC);
    // int_enable();

    // uc_wiota_init();
    // uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);
    // uc_wiota_run();

    // rt_thread_delay(3000);

    // uc_wiota_set_alarm_time(1);

    uc_wiota_set_alarm_time(10); // wakeup in 3s
    rt_thread_delay(2);
    // uc_wiota_sleep_enter(0, 0); // sleep, not open ex wk

    while (1)
    {
        rt_thread_mdelay(3000);
        // if (last_cnt != rtc_handle_cnt)
        // {
        //     last_cnt = rtc_handle_cnt;
        // rt_kprintf("cnt %u rtc %u %u diff %u\n", last_cnt, last_dfe, rtc_end_dfe, rtc_end_dfe - last_dfe);
        //     last_dfe = l1_read_dfe_counter();
        //     uc_wiota_set_alarm_time(10);
        // }

        rt_kprintf("cnt %d \n", rtc_handle_cnt);
    }
}

void app_test_voice_acc_task(void *pPara)
{
    unsigned int *data = rt_malloc(1660);
    unsigned short i = 0;
    unsigned char is_first = TRUE;
    unsigned int cnt = 0;
    // s16_t data1;
    // s16_t *data2 = (s16_t *)data;
    unsigned char rf_status = 0;

    for (i = 0; i < 415; i++)
    {
        data[i] = i * 4;
        // data[i] = 0;
    }

    while (1)
    {
        rt_thread_mdelay(10);
        rt_kprintf("voice acc %d %d %u\n", cnt++, g_voice_acc_start, uc_wiota_get_curr_rf_cnt());
        // if (g_voice_acc_start && !g_voice_data_handle)
        rf_status = uc_wiota_get_physical_status();
        if (g_voice_acc_start && (RF_STATUS_TX == rf_status || RF_STATUS_IDLE == rf_status) && !g_voice_data_handle)
        {
            // if (data == RT_NULL)
            // {
            //     continue;
            // }

            // for (i = 0; i < 830; i++)
            // {
            //     // ((s16_t *)data)[i+1] = app_get_k83_data();
            //     ((s16_t *)data)[i] = app_get_k83_data();
            //     // i += 2;
            // }

            // l1_print_data_u32(data, 415);
            g_voice_data_handle = TRUE;

            if (uc_wiota_voice_data_acc(is_first, data, 1660))
            {
                is_first = FALSE;
            }
            else
            {
                g_voice_data_handle = FALSE;
            }
        }
    }

    rt_free(data);

    return;
}

// extern unsigned char g_isSingleTone;

// void app_test_factory(void)
// {
//     uc_wiota_init();
//     uc_wiota_set_recv_mode(UC_AUTO_RECV_STOP);
//     uc_wiota_run();

//     g_isSingleTone = TRUE;
//     l1_rf_init();
//     l1_rf_handle_rx_raw(0); // open tx

//     rt_thread_mdelay(10);

//     g_isSingleTone = FALSE;
//     l1_rf_init();
//     l1_rf_handle_rx_raw(1); // close tx
//     uc_wiota_set_recv_mode(UC_AUTO_RECV_START);
// }

// small demo for all auto
void app_test_main_task(void *pPara)
{
    // app_test_case_recv();
    // app_test_case_send();
    // app_test_case_send_always();
    // app_test_rf_lpm();
    // app_test_rtc_32k();
    // app_test_factory();
    return;
}

// void app_test1_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test1 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(11);
//     }
//     return;
// }

// void app_test2_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test2 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(12);
//     }
//     return;
// }

// void app_test3_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test3 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(13);
//     }
//     return;
// }

// void app_test4_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test4 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(4);
//     }
//     return;
// }

// void app_test5_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test5 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(5);
//     }
//     return;
// }

// void app_test6_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test6 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(6);
//     }
//     return;
// }

// void app_test7_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test7 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(2);
//     }
//     return;
// }

// void app_test8_task(void *pPara)
// {
//     unsigned int cnt = 0;
//     while (1)
//     {
//         if (!uc_wiota_get_is_gating())
//         {
//             rt_kprintf("test8 task %u %u\n", cnt++, uc_wiota_get_curr_rf_cnt());
//         }
//         rt_thread_mdelay(13);
//     }
//     return;
// }

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
    uc_thread_create_test(&testTaskHandle, "test0", app_test_main_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test2TaskHandle, "test2", app_test_voice_acc_task, NULL, 256, 3, 3);
    // rt_thread_startup((rt_thread_t)test2TaskHandle);
    rt_thread_startup((rt_thread_t)testTaskHandle);

    // uc_thread_create_test(&test1TaskHandle, "test1", app_test1_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test2TaskHandle, "test2", app_test2_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test3TaskHandle, "test3", app_test3_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test4TaskHandle, "test4", app_test4_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test5TaskHandle, "test5", app_test5_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test6TaskHandle, "test6", app_test6_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test7TaskHandle, "test7", app_test7_task, NULL, 256, 3, 3);
    // uc_thread_create_test(&test8TaskHandle, "test8", app_test8_task, NULL, 256, 3, 3);
    // rt_thread_startup((rt_thread_t)test1TaskHandle);
    // rt_thread_startup((rt_thread_t)test2TaskHandle);
    // rt_thread_startup((rt_thread_t)test3TaskHandle);
    // rt_thread_startup((rt_thread_t)test4TaskHandle);
    // rt_thread_startup((rt_thread_t)test5TaskHandle);
    // rt_thread_startup((rt_thread_t)test6TaskHandle);
    // rt_thread_startup((rt_thread_t)test7TaskHandle);
    // rt_thread_startup((rt_thread_t)test8TaskHandle);
}
#endif