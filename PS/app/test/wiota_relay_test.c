
#include <rtthread.h>
#include <rtdevice.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

#define TEST_ASYNC_FREQ_SINGLE 96
//define ONLY_SEND_TEST is send.(contine send, delay 5s)
//no define ONLY_SEND_TEST, only relay data.(contine send, delay 5s)

// #define ONLY_SEND_TEST

typedef struct
{
    unsigned int id;
    unsigned int num;
} async_test_data;

static void *test_aync_taskHandle = NULL;
static void *test_aync_queueHandle = NULL;

//int test_seq_num = 0;
int test_recv_num = (~0);

rt_base_t led_pin = 2;
int led_value = 1;

rt_timer_t timer_recv_manager;

static void *uc_create_test_queue(const char *name, unsigned int msg_size, unsigned int max_msgs, unsigned char flag)
{
    //return rt_mq_create(name, 4, max_msgs, flag);
    //return  rt_mq_create(name, msg_size, max_msgs, flag);
    rt_mq_t mq = rt_malloc(sizeof(struct rt_messagequeue));
    void *msgpool = rt_malloc(4 * max_msgs);

    if (RT_NULL == mq || RT_NULL == msgpool)
        return RT_NULL;

    if (RT_EOK != rt_mq_init(mq, name, msgpool, 4, 4 * max_msgs, flag))
        return RT_NULL;

    return (void *)mq;

}

int uc_recv_test_queue(void *queue, void **buf, unsigned int size, signed int timeout)
{

    unsigned int address = 0;
    int result = 0;
    result = rt_mq_recv(queue, &address, 4, timeout);
    *buf = (void *)address;
    //TRACE_PRINTF("recv address 0x%x, buf=0x%x\n", address,  *buf);
    return result;

}

int uc_send_test_queue(void *queue, void *buf, unsigned int size, signed int timeout)
{
    unsigned int address = (unsigned int)buf;
    //TRACE_PRINTF("send address 0x%x\n", address);
    return rt_mq_send_wait(queue, &address, 4, timeout);
}

int uc_dele_test_queue(void *queue)
{

    // return rt_mq_delete( queue);
    rt_err_t ret = rt_mq_detach(queue);
    rt_free(((rt_mq_t)queue)->msg_pool);
    rt_free(queue);
    return ret;
}

static int uc_thread_create_test_task(void **thread,
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



static void test_recv_async_callback(uc_recv_back_p recvData)
{
    if (recvData->data == RT_NULL )
    {
        return ;
    }

    if (UC_OP_SUCC != recvData->result )
    {
       rt_kprintf("===========> test_recv_async_callback is error\n");
        rt_free(recvData->data);
        return ;
    }

#ifndef ONLY_SEND_TEST
    rt_kprintf("=======> test_recv_async_callback is suc\n");
    switch (recvData->type)
    {
        case UC_RECV_MSG:
        case UC_RECV_BC:
             uc_send_test_queue(test_aync_queueHandle, recvData->data, ( unsigned int )recvData->data_len, 4);
            return;

        default:
            //rt_kprintf("Type ERROR!!!!!!!!!!!\n");
            break;
    }
#else
     rt_free(recvData->data);
#endif
}


static void test_async_send_data(void *data, int len)
{
    int num = 0;
    int result = 0;

    for(num = 0; num < 3; num++)
    {
        //async_test_data *bf = data;
        result = uc_wiota_send_data(0, data, len, NULL, 0, 20000, NULL);

        //bf->num ++;
        rt_kprintf("====>uc_wiota_send_data result %d\n", result);
    }
}

static void manager_recv_timer_func(void *parameter)
{
    led_value = ~led_value;
    rt_pin_write(led_pin, led_value&0x01);
    rt_kprintf("====>switch led led_value %d\n", led_value);
}
static void test_async_relay_task(void *pPara)
{
    sub_system_config_t wiota_config = {0};
    async_test_data data;
    int id_len;
    unsigned char freq_list[16] = {255};
    unsigned char freq = TEST_ASYNC_FREQ_SINGLE;

    rt_pin_mode(led_pin, PIN_MODE_OUTPUT);
    rt_pin_write(led_pin, led_value);

    timer_recv_manager = rt_timer_create("led", manager_recv_timer_func, RT_NULL, 300, RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

    // first of all, init wiota
    uc_wiota_init();

    uc_wiota_light_func_enable(0);

    // test! whole config
    uc_wiota_get_system_config(&wiota_config);
    //rt_kprintf("config show %d %d %d %d 0x%x 0x%x\n",
    //           wiota_config.id_len, wiota_config.pp, wiota_config.symbol_length,
    //           wiota_config.btvalue, wiota_config.systemid, wiota_config.subsystemid);

    wiota_config.symbol_length = 3; // 3->1024
    wiota_config.bandwidth = 4; // 4->25k
    uc_wiota_set_system_config(&wiota_config);

    uc_wiota_get_freq_list(freq_list);
    if (freq_list[0] != 255 && 0 != freq_list[0])
        freq = freq_list[0];

    rt_kprintf("====> current freq %d\n", freq);
     uc_wiota_set_freq_info(freq);


    uc_wiota_set_data_rate(0, 0);

    uc_wiota_set_subframe_num(4);

    uc_wiota_set_bc_round(6);

    uc_wiota_set_detect_time(10);

    uc_wiota_run();

    uc_wiota_set_cur_power(21);//21

    led_value = 0;
    rt_pin_write(led_pin, led_value);

    uc_wiota_register_recv_data_callback(test_recv_async_callback, UC_CALLBACK_NORAMAL_MSG);

    uc_wiota_get_userid(&data.id, (unsigned char *)&id_len);

    //extern unsigned int l1_read_dfe_counter(void);
    data.num = (*(volatile unsigned int *)(0x14 + 0x31000 + 0x00380000));//rt_tick_get();

    #ifdef ONLY_SEND_TEST
    rt_kprintf("=====> data.num 0x%x\n", data.num);
     rt_timer_start(timer_recv_manager);

     // send bc data
    test_async_send_data(&data, sizeof(async_test_data));
    rt_timer_stop(timer_recv_manager);
    // close led
    led_value = 0;
    rt_pin_write(led_pin, led_value);
   #endif

    while(1)
    {
        async_test_data *buf;
         int res = uc_recv_test_queue(test_aync_queueHandle, (void *)&buf, 4,15000);
         if (0 != res)
         {
            rt_kprintf("uc_recv_test_queue error\n");
            #ifdef ONLY_SEND_TEST
            data.num ++;

            rt_timer_start(timer_recv_manager);
             test_async_send_data(&data, sizeof(async_test_data));

              rt_timer_stop(timer_recv_manager);

              // close led
             led_value = 0;
             rt_pin_write(led_pin, led_value);
             #endif
            continue;
         }

         rt_kprintf("delay %d rem num %d page num %d recv data from 0x%x. my id 0x%x frame len %d\n", data.id%1000 + 1000, test_recv_num, buf->num, buf->id, data.id, uc_wiota_get_frame_len(0));
         #ifndef ONLY_SEND_TEST
         if (buf->num != test_recv_num && buf->id != data.id)
         {
              //int frame_len = uc_wiota_get_frame_len(0) < 500? uc_wiota_get_frame_len(0):500;
              rt_timer_start(timer_recv_manager);

             //delay send
             rt_thread_mdelay(data.id%2000 + uc_wiota_get_frame_len(0) /1000 * 2);

            test_recv_num = buf->num;
             test_async_send_data(buf, sizeof(async_test_data));

              rt_timer_stop(timer_recv_manager);

              // close led
             led_value = 0;
             rt_pin_write(led_pin, led_value);
         }
         #endif
         rt_free(buf);
     }

    uc_wiota_exit();

    uc_dele_test_queue(test_aync_queueHandle);
}
void app_test_aync_relay_init(void)
{
    test_aync_queueHandle = uc_create_test_queue("relayTest", 4, 20, 1);
    if (RT_NULL == test_aync_queueHandle)
    {
        rt_kprintf("uc_create_test_queue error\n");
        return ;
    }

    uc_thread_create_test_task(&test_aync_taskHandle, "relayTest", test_async_relay_task, NULL, 256, 3, 3);
    rt_thread_startup((rt_thread_t)test_aync_taskHandle);
}


