/******************************************************************************
* @file      test_wiota_send_data.c
* @brief     Asynchronous transceiver testing between two IoTs
* @author    ypzhang
* @version   1.0
* @date      2023.11.10
*
* @copyright Copyright (c) 2018 UCchip Technology Co.,Ltd. All rights reserved.
*
******************************************************************************/
#include <rtthread.h>

#ifdef WIOTA_ASYNC_SEND_TEST
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "test_wiota_api.h"
#include "uc_string_lib.h"
#include "uc_gpio.h"

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#endif
#include <stdlib.h>
#include <drivers/pin.h>

// #define MAX_SEND_BYTE (314)
#define MAX_SEND_BYTE (20)

#define LED1_GPIO_NUM (2)  // send success
#define LED2_GPIO_NUM (3)  // senf dail
#define LED3_GPIO_NUM (17) // recv success

// Receive data counter
uint32_t g_recv_count = 0;
uint32_t g_remember_recv_count = 0;

// Successfully sent data counter
uint32_t g_succ_send_count = 0;
uint32_t g_remember_succ_send_count = 0;

// data send fail counter
uint32_t g_fail_send_count = 0;
uint32_t g_remember_fail_send_count = 0;

static int pin_app_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_pin_mode(LED1_GPIO_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED2_GPIO_NUM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED3_GPIO_NUM, PIN_MODE_OUTPUT);

    return ret;
}

static void wiota_async_recv_cb(uc_recv_back_p data)
{
    // data successfully received
    if (0 == data->result)
    {
        rt_kprintf("wiota_recv_callback result = %d\n", data->result);

        for (uint16_t index = 0; index < data->data_len; index++)
        {
            rt_kprintf("%c", *(data->data + index));
        }
        rt_kprintf(", data_len %d\n", data->data_len);

        // must free data.
        rt_free(data->data);
        g_recv_count++;
    }
}

static void wiota_async_send_task(void *parameter)
{
    UC_OP_RESULT send_result;
    uint16_t data_len;
    uint32_t my_user_id = 0xabe44fcb;
    uint32_t target_user_id = 0xabe44fca;
    uint8_t sendbuffer[MAX_SEND_BYTE];
    sub_system_config_t config;

    while (1)
    {
        g_recv_count = 0;
        g_succ_send_count = 0;
        g_fail_send_count = 0;

        // wiota init
        uc_wiota_init();

        // set wiota config
        uc_wiota_get_system_config(&config);
        // The subsystem IDs of all terminals within the same system need to be consistent
        config.subsystemid = 1;

        // Set the user ID and configure it after protocol stack initialization before startup
        uc_wiota_set_userid(&my_user_id, 4);

        // set trial freq index
        uc_wiota_set_freq_info(10);

        uc_wiota_set_system_config(&config);
        rt_kprintf("id_len %d, symbol_length %d, pz %d, btvalue %d, spectrum_idx %d, subsystemid 0x%x \n",
                   config.id_len, config.symbol_length, config.pz, config.btvalue, config.spectrum_idx, config.subsystemid);

        // 1 means that any length of data is subject to CRC verification
        uc_wiota_set_crc(1);

        uc_wiota_set_data_rate(0, 3);

        // Set the wait time for detection before sending
        uc_wiota_set_detect_time(0);

        // Set the number of unicast retransmissions
        uc_wiota_set_unisend_fail_cnt(2);

        // Set the number of subframes per frame when sending
        uc_wiota_set_subframe_num(4);

        // uc_set_power(0);
        uc_wiota_set_cur_power(0);

        // wiota run
        uc_wiota_run();

        // register the receive data callback function.
        uc_wiota_register_recv_data_callback(wiota_async_recv_cb, UC_CALLBACK_NORAMAL_MSG);

        while (1)
        {
            uint32_t my_id;
            uint8_t id_len;

            // Random string
            data_len = rand() % MAX_SEND_BYTE;
            data_len = (data_len == 0)? 1 : data_len;
            rt_kprintf("send_data info: ");
            for (uint16_t i = 0; i < data_len; i++)
            {
                sendbuffer[i] = (char)rand() % 94 + 33;
                rt_kprintf("%c", sendbuffer[i]);
            }
            rt_kprintf(", data_len = %d \n", data_len);

            uc_wiota_get_userid(&my_id, &id_len);
            rt_kprintf("my_freq = %d, my_user_id = %x, target user_id = %x\n", uc_wiota_get_freq_info(), my_id, target_user_id);

            // send test data.
            send_result = uc_wiota_send_data(target_user_id, sendbuffer, data_len, NULL, 0, 60000, RT_NULL);
            if (send_result == UC_OP_SUCC)
            {
                g_succ_send_count++;
                rt_kprintf("uc_wiota_send_data send success\n");
            }
            else if (send_result == UC_OP_TIMEOUT)
            {
                g_fail_send_count++;
                rt_kprintf("uc_wiota_send_data send timeout!!!\n");
                break;
            }
            else
            {
                g_fail_send_count++;
                rt_kprintf("uc_wiota_send_data send fail!!!\n");
                break;
            }

            rt_thread_mdelay(5000);
        }

        uc_wiota_exit();
    }
}

static void wiota_async_led_strl_task(void *parameter)
{
    // init GPIO pin
    pin_app_init();

    while (1)
    {
        // recv success
        if (g_remember_recv_count != g_recv_count)
        {
            g_remember_recv_count = g_recv_count;
            rt_pin_write(LED3_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(500);
            rt_pin_write(LED3_GPIO_NUM, PIN_LOW);
        }
        
        // send data success
        if (g_remember_succ_send_count != g_succ_send_count)
        {
            g_remember_succ_send_count = g_succ_send_count;
            rt_pin_write(LED1_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(500);
            rt_pin_write(LED1_GPIO_NUM, PIN_LOW);
        }

        // send data fail
        if (g_remember_fail_send_count != g_fail_send_count)
        {
            g_remember_fail_send_count = g_fail_send_count;
            rt_pin_write(LED2_GPIO_NUM, PIN_HIGH);
            rt_thread_mdelay(500);
            rt_pin_write(LED2_GPIO_NUM, PIN_LOW);
        }

        rt_thread_mdelay(1000);
    }
}

void wiota_async_data_recv_and_send_demo(void)
{
    rt_thread_t testTaskHandle2 = rt_thread_create("send_data_task", wiota_async_send_task, NULL, 1024, 3, 3);
    if (testTaskHandle2 != NULL)
    {
        rt_thread_startup(testTaskHandle2);
    }
    else
    {
        uc_wiota_exit();
        return;
    }

    rt_thread_t testTaskHandle3 = rt_thread_create("led_ctrl_task", wiota_async_led_strl_task, NULL, 1024, 3, 3);
    if (testTaskHandle3 != NULL)
    {
        rt_thread_startup(testTaskHandle3);
    }
    else
    {
        uc_wiota_exit();
        return;
    }
}

#endif // WIOTA_ASYNC_SEND_TEST