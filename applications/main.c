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
#ifdef _RT_THREAD_
#include <rtdevice.h>
#endif

#ifdef _FPGA_
#include <board.h>
#include "uc_event.h"
#endif

#ifdef UC8288_MODULE
#ifdef RT_USING_AT
#include "at.h"
#endif
#endif

#ifdef _WATCHDOG_APP_
#include "uc_watchdog_app.h"
#endif

#ifdef _ADC_APP_
#include "uc_adc_app.h"
#endif

#ifdef _CAN_APP_
#include "uc_can_app.h"
#endif

#ifdef _DAC_APP_
#include "uc_dac_app.h"
#endif

#ifdef _IIC_APP_
#include "uc_iic_app.h"
#endif

#ifdef _PIN_APP_
#include "uc_pin_app.h"
#endif

#ifdef _PWM_APP_
#include "uc_pwm_app.h"
#endif

#ifdef _RTC_APP_
#include "uc_rtc_app.h"
#endif

#ifdef _SPI_FLASH_APP_
#include "uc_spi_flash_app.h"
#endif

#ifdef _SPIM_FLASH_APP_
#include "uc_spim_flash_app.h"
#endif

#ifdef _UART_APP_
#include "uc_uart_app.h"
#endif

#ifdef _RS485_APP_
#include "uc_rs485_app.h"
#endif

#ifdef _ROMFUNC_
#include "dll.h"
#endif

#ifdef RT_TASK_RESOURCE_TOOL
#include "resource_manager.h"
#endif

#ifdef _QUICK_CONNECT_
#include "quick_connect.h"
#endif

#include "uc_wiota_static.h"
#include "uc_wiota_api.h"

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
extern void at_handle_log_uart(int uart_number);
#endif
extern int at_wiota_gpio_report_init(void);
extern int wake_out_pulse_init(void);

// extern void at_wiota_manager(void);

//void task_callback(struct rt_thread* from, struct rt_thread* to)
//{
//    rt_kprintf("name = %s, 0x%x\n", from->name, from);
//}
//
//
//void init_statistical_task_info(void)
//{
//    rt_scheduler_sethook(task_callback);
//}

void app_device_demo(void)
{
#ifdef _ADC_APP_
    adc_app_sample();
#endif

#ifdef _CAN_APP_
    can_app_sample();
#endif

#ifdef _DAC_APP_
    dac_app_sample();
#endif

#ifdef _IIC_APP_
    iic_app_sample();
#endif

#ifdef _PIN_APP_
    pin_app_sample();
#endif

#ifdef _PWM_APP_
    pwm_app_sample();
#endif

#ifdef _RTC_APP_
    rtc_app_sample();
//    alarm_app_sample();
#endif

#ifdef _SPI_FLASH_APP_
    spi_flash_app_sample();
#endif

#ifdef _SPIM_FLASH_APP_
    spim_flash_app_sample();
#endif

#ifdef _RS485_APP_
    rs485_app_sample();
#endif

#ifdef _UART_APP_
    uart_app_sample();
#endif
}

int main(void)
{
#ifdef _ROMFUNC_
    dll_open();
#endif

    uc_wiota_static_data_init();

#ifdef RT_TASK_RESOURCE_TOOL
    resource_manager_init();
#endif

#ifdef _WATCHDOG_APP_
    if (!watchdog_app_init())
        watchdog_app_enable();
#endif

#ifdef UC8288_MODULE
    if (!uc_wiota_get_factory_ctrl())
    {
        #ifdef RT_USING_AT
        at_server_init();
        // at_wiota_manager();
        #endif
    }
    else
    {
        #ifdef _L1_FACTORY_FUNC_
        uc_wiota_factory_task_init();
        #endif
    }
#endif

    at_wiota_gpio_report_init();
    wake_out_pulse_init();

#ifdef WIOTA_RELAY_APP
    extern int uc_wiota_relay_app_init(void);
    if (0 == uc_wiota_relay_app_init())
    {
        rt_kprintf("uc_wiota_relay_app_init suc\n");
    }
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
//    at_handle_log_uart(0);
#endif

    // app_test_aync_relay_init();

    //    uc_wiota_light_func_enable(0);

    // rt_kprintf("before while\n");
    app_device_demo();

#ifdef _QUICK_CONNECT_
    quick_connect_task_init();
#endif

    while (1)
    {
        unsigned int total;
        unsigned int used;
        unsigned int max_used;

        rt_thread_delay(7000);

        // resource_manager(RESOURCE_DETAIL_MODE);
        // resource_manager(RESOURCE_SIMPLE_MODE);

        rt_memory_info(&total, &used, &max_used);
        rt_kprintf("total %d used %d maxused %d\n", total, used, max_used);

    }

    //    init_statistical_task_info();

    return 0;
}
