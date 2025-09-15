#include <rtthread.h>
#include <rtdevice.h>

#ifdef _FPGA_
#include <board.h>
#include "uc_event.h"
#endif

#ifdef UC8288_MODULE
#include "uc_wiota_static.h"
#include "uc_wiota_api.h"

#ifdef RT_USING_AT
#include "at.h"
#include "at_wiota.h"
#include "at_wiota_gpio_report.h"
#endif

#endif

#ifdef _QUICK_CONNECT_
#include "quick_connect.h"
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
extern void at_handle_log_uart(int uart_number);
#endif

#ifdef RT_USING_AT
#ifdef UC8288_MODULE
extern int at_wiota_gpio_report_init(void);
extern int wake_out_pulse_init(void);
#endif
#endif

#ifdef RT_USING_WDT

rt_device_t wdg_dev = RT_NULL;

static void wiota_app_wdt_idle_hook(void)
{
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL);
}

static int wiota_app_wdt_init(void)
{
    int ret = 0;
    rt_uint32_t timeout = 5;
    char *wdt_device_name = "wdt";

    wdg_dev = rt_device_find(wdt_device_name);
    if (!wdg_dev)
    {
        rt_kprintf("find %s failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("set %s timeout failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    rt_thread_idle_sethook(wiota_app_wdt_idle_hook);

    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", wdt_device_name);
        return -RT_ERROR;
    }

    return RT_EOK;
}

#endif

int wiota_app_init(void)
{
    rt_kprintf("wiota_app_init\n");

#ifdef _ROMFUNC_
    dll_open();
#endif

#ifdef RT_USING_WDT
    wiota_app_wdt_init();
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

#ifdef RT_USING_AT
    at_wiota_gpio_report_init();
    wake_out_pulse_init();
#endif
#endif

#ifdef WIOTA_RELAY_APP
    extern int uc_wiota_relay_app_init(void);
    if (0 == uc_wiota_relay_app_init())
    {
        rt_kprintf("uc_wiota_relay_app_init suc\n");
    }
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    // at_handle_log_uart(0);
#endif

#ifdef _QUICK_CONNECT_
#ifdef RT_USING_AT
    quick_connect_task_init();
#endif
#endif

    return 0;
}