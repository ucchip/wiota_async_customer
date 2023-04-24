
#include <rtthread.h>
#ifdef WIOTA_ASYNC_GATEWAY_AT
#include <rtdevice.h>
#include "uc_wiota_gateway_api.h"
#include "uc_wiota_static.h"

static void uc_gateway_recv_test(unsigned int id, unsigned char type, void *data, int len)
{
    rt_kprintf("uc_gateway_recv_test id = 0x%x, type = %d, len = %d\r\n", id, type, len);
    rt_kprintf("data:\r\n");
    for (uint32_t index = 0; index < len; index++)
    {
        rt_kprintf("%c", ((char *)data)[index]);
    }
    rt_kprintf("\r\n");
}

static void gateway_api_test(void *para)
{
    unsigned char serial[16] = {0};
    unsigned int delay_time = 0;
    char dev_id_buf[16] = {0};

    for (int n = 0; n < 16; n++)
        dev_id_buf[n] = '0' + n;

    uc_wiota_get_dev_serial(serial);

    delay_time = ((serial[12] << 8) | serial[13]);

    while (1)
    {
        unsigned char connect_fail_counter = 0;

        if (UC_GATEWAY_OK != uc_wiota_gateway_init(1, 0, 1, 168, 3, UC_GATEWAY_BROADCAST_UNICAST_CONT_MODE, 0, 3))
        {
            rt_kprintf("uc_wiota_gateway_init error\n");
            uc_wiota_gateway_exit();
            rt_thread_delay(1000);
            continue;
        }

        // set recv callback
        uc_wiota_gateway_set_recv_register(uc_gateway_recv_test);

        //send data.
        while (connect_fail_counter < 10)
        {

            if (uc_wiota_gateway_send(uc_wiota_get_gateway_id(), dev_id_buf, 16, 6000, RT_NULL))
            {
                connect_fail_counter++;
            }
            else
            {
                connect_fail_counter = 0;
            }

            rt_kprintf("now delay time %d(0x%x)ms. 0 is default 10s\n", delay_time, delay_time);
            rt_thread_mdelay(delay_time == 0 ? 10000 : delay_time);

            if (dev_id_buf[15] == 'Z')
            {
                for (int n = 0; n < 16; n++)
                    dev_id_buf[n] = '0' + n;
            }
            else
            {
                for (int n = 0; n < 16; n++)
                    dev_id_buf[n] += 1;
            }
        }

        uc_wiota_gateway_exit();
    }
}

int gateway_test_demo(void)
{
    rt_thread_t gateway_handler;
    gateway_handler = rt_thread_create("test",
                                       gateway_api_test,
                                       RT_NULL,
                                       1024,
                                       6,
                                       3);

    if (gateway_handler == RT_NULL)
    {
        return 1;
    }

    // start task
    rt_thread_startup(gateway_handler);

    return 0;
}

#endif
