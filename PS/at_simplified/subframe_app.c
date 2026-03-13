#include <rtthread.h>
#include "uc_wiota_api.h"
#include "subfrmae_app.h"

#define ASYNC_WIOTA_APP_MAX_SUBFRAM_NUM (8)

/*获取子帧相关数据*/
static int wiota_get_sub_frame_info(unsigned short *sub_fram_len,
                                    unsigned short *bc_sub_fram_len,
                                    unsigned char *async_pramble_time,
                                    unsigned char *async_bc_pramble_time,
                                    unsigned short *sub_fram_time,
                                    unsigned char *first_sub_fram_len,
                                    unsigned char *first_bc_sub_fram_len)
{
    radio_info_t radio;
    // get sub fram num
    unsigned char subframe_num = uc_wiota_get_subframe_num(); // 获取当前WIoTa子帧数配置
    // get one fram time(short msg)
    unsigned int one_fram_time = uc_wiota_get_frame_len(1); // 获取单播发送一帧数据需要的时间

    if (RT_NULL != sub_fram_time)
        *sub_fram_time = uc_wiota_get_subframe_len(); // 获取当前配置下发送一个子帧需要的时间

    if (RT_NULL != async_pramble_time)
        *async_pramble_time = one_fram_time - subframe_num * (*sub_fram_time); // 计算当前配置下单播的帧头和帧尾发送需要的时间

    if (RT_NULL != async_bc_pramble_time)
    {
        // get one fram time(bc msg)
        one_fram_time = uc_wiota_get_frame_len(0);                                // 获取广播发送1帧数据的时间
        *async_bc_pramble_time = one_fram_time - subframe_num * (*sub_fram_time); // 计算当前配置下广播的帧头和帧尾发送需要的时间
    }

    uc_wiota_get_radio_info(&radio); // 主要是为了获取当前配置下的mcs信息

    if (RT_NULL != sub_fram_len)
    {
        // short message of sub frame len
        *sub_fram_len = uc_wiota_get_subframe_data_len(radio.cur_mcs, 0, 0, 0); // 当前mcs配置下单播子帧的数据负载长度
    }

    if (RT_NULL != bc_sub_fram_len)
    {
        // bc message of sub frame len
        *bc_sub_fram_len = uc_wiota_get_subframe_data_len(radio.cur_mcs, 1, 0, 0); // 当前mcs配置下广播子帧的数据负载长度
    }

    if (RT_NULL != first_sub_fram_len)
        *first_sub_fram_len = uc_wiota_get_subframe_data_len(radio.cur_mcs, 0, 1, 1); // 获取单播第一个子帧应用可用负载数据长度

    if (RT_NULL != first_bc_sub_fram_len)
        *first_bc_sub_fram_len = uc_wiota_get_subframe_data_len(radio.cur_mcs, 1, 1, 1); // 获取广播第一个子帧应用可用负载数据长度

    return 0;
}

/*
type 0: bc msg, 1: normal msg
len : 1 - 310 byte
*/
unsigned char wiota_app_subframe(unsigned int type, int len)
{
    unsigned short max_fram_len;
    unsigned short min_fram_len;
    unsigned short sub_fram_len;
    unsigned short crc_limit;
    unsigned char first_sub_fram_len;
    unsigned char sub_fram_num;

    int max_len = type ? UC_DATA_LENGTH_LIMIT : UC_BC_LENGTH_LIMIT;
    if (len < 1 || len > max_len)
    {
        rt_kprintf("len %d error, type %d\n", len, type);
        return 0;
    }

    if (type)
    {
        // 1: normal msg。获取当前配置下单播的子帧负载数据长度和第一个子帧应用可用负载数据长度
        wiota_get_sub_frame_info(&sub_fram_len, RT_NULL, RT_NULL, RT_NULL, RT_NULL, &first_sub_fram_len, RT_NULL);
    }
    else
    {
        // 0: bc msg。获取当前配置下广播播的子帧负载数据长度和第一个子帧应用可用负载数据长度
        wiota_get_sub_frame_info(RT_NULL, &sub_fram_len, RT_NULL, RT_NULL, RT_NULL, RT_NULL, &first_sub_fram_len);
    }

    // rt_kprintf("sub_fram_len %d\n", sub_fram_len);
    // rt_kprintf("first_sub_fram_len %d\n", first_sub_fram_len);

    // 计算最大最小子帧长度的数据负载长度. 最长子帧按照8计算
    max_fram_len = sub_fram_len * (ASYNC_WIOTA_APP_MAX_SUBFRAM_NUM - 1) + first_sub_fram_len;
    min_fram_len = sub_fram_len * (ASYNC_WIOTA_APP_MAX_SUBFRAM_NUM - 1) + first_sub_fram_len;

    if (len > min_fram_len)
    {
        // 控制多帧发送，抗干扰能力更强
        len = (len >> 1) + (len & 0x1);
    }

    crc_limit = uc_wiota_get_crc();
    if (len >= crc_limit)
    {
        len += 2;
    }

    if (len <= min_fram_len)
    {
        // 当发送数据长度小于最小子帧数据长度时配置为最小子帧数
        return UC_MIN_SUBFRAME_NUM;
    }
    else if (len >= max_fram_len)
    {
        // 当发送数据长度大于最大子帧的负载数据长度，计算需要发送的帧数。
        int complete_frame_num = (len + (max_fram_len - 1)) / max_fram_len;
        // 计算平均到每帧的数据长度
        len = (len + complete_frame_num - 1) / complete_frame_num;
    }
    // 计算最优子帧数
    sub_fram_num = 1 + (len - first_sub_fram_len + sub_fram_len - 1) / sub_fram_len;

    return sub_fram_num;
}

void wiota_test_subframe(void)
{
    char send_data[100] = {"122"};
    unsigned char subframe_num = 0;

    rt_kprintf("wiota_test_subframe start\n");

    // start wiota.
    uc_wiota_init();

    // set wiota freq
    uc_wiota_set_freq_info(100);

    // WIota start work
    uc_wiota_run();

    subframe_num = wiota_app_subframe(0, sizeof(send_data));
    if (subframe_num > 0)
        // set wiota subframe num
        uc_wiota_set_subframe_num(subframe_num);

    rt_kprintf("subframe_num %d\n", subframe_num);

    // send bc msg
    uc_wiota_send_data(0, (unsigned char *)send_data, sizeof(send_data), RT_NULL, 0, 6000, RT_NULL);

    // close wiota
    uc_wiota_exit();
}