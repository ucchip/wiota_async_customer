
#include <rtthread.h>
#ifdef WIOTA_ASYNC_GATEWAY_API
#include <board.h>
#include "uc_wiota_api.h"
#include "wiota_subframe.h"

t_async_wiota_subframe wiota_subframe_info = {0};


// symbollength: 0 ~3
int gateway_wiota_subframe_init(unsigned char symbollength, unsigned char crc_flag)
{

    // short message of first sub frame data len
    wiota_subframe_info.unicast_first_subfram_data_len = uc_wiota_get_subframe_data_len(0, 0, 1);
    if (wiota_subframe_info.unicast_first_subfram_data_len  < 0)
    {
        rt_kprintf("unicast_first_subfram_data_len is error\n");
        return 1;
    }

    wiota_subframe_info.broadcast_first_subfram_data_len = 0;
    if (wiota_subframe_info.broadcast_first_subfram_data_len  < 0)
    {
        rt_kprintf("broadcast_first_subfram_data_len is error\n");
        return 2;
    }

    uc_wiota_set_subframe_num(ASYNC_WIOTA_DEFAULT_SUBFRAM);

    wiota_subframe_info.current_subframe = ASYNC_WIOTA_DEFAULT_SUBFRAM;

    wiota_subframe_info.crc_flag = crc_flag;

    wiota_subframe_info.symbollength = symbollength;


    return 0;
}

static int gateway_get_subframe(short len, short first_subfram_data_len, short other_subframe_data_len)
{
    unsigned char  sub_fram_num = 0;
    unsigned short max_subframe_data_len;
    unsigned char frame_num = 0;
    unsigned short every_frame_data_len;

    // data length of the current max sub frame.
    max_subframe_data_len = (ASYNC_WIOTA_MAX_SUBFRAME - 1) * other_subframe_data_len +
                                        first_subfram_data_len;

    // number of frames required for data thansmission to complete
    frame_num = len / max_subframe_data_len + (len % max_subframe_data_len ? 1: 0);

    // average data length per frame
    every_frame_data_len = len/ frame_num + (len% frame_num ? 1: 0);

    // averge data length per frame number of subframe required.
    sub_fram_num = (every_frame_data_len - first_subfram_data_len)/ other_subframe_data_len +
                                   ((every_frame_data_len - first_subfram_data_len) %  other_subframe_data_len ? 1: 0)+
                                   1;

    if (sub_fram_num < ASYNC_WIOTA_MIN_SUBFRAME)
        sub_fram_num = ASYNC_WIOTA_MIN_SUBFRAME;

    if (sub_fram_num > ASYNC_WIOTA_MAX_SUBFRAME)
        sub_fram_num = ASYNC_WIOTA_MAX_SUBFRAME;

    return sub_fram_num;
}

// current_mcs: 0 - 7
int gateway_wiota_set_subframe(WIOTA_DATA_TYPE type, unsigned char current_mcs,unsigned int len)
{
    //when bt is 0.3  MOD_TYPE is fixed to 2

    const u16_t mcs_table_without_crc_03[4][MCS_NUM] = {{7, 9, 52, 66, 80, 0, 0, 0}, // symbolLength = 0. is 128
                                                        {7, 15, 22, 52, 108, 157, 192, 0},// symbolLength = 1. is 256
                                                        {7, 15, 31, 42, 73, 136, 255, 297},
                                                        {7, 15, 31, 63, 108, 220, 451, 619}};

    unsigned char  sub_fram_num = 0;

    if (WIOTA_SEND_UNICAST == type)
    {
        sub_fram_num = gateway_get_subframe(wiota_subframe_info.crc_flag ? len + 2: len,
                                wiota_subframe_info.unicast_first_subfram_data_len,
                                mcs_table_without_crc_03[wiota_subframe_info.symbollength][current_mcs] - 1);
    }
    else
    {
        sub_fram_num = gateway_get_subframe(wiota_subframe_info.crc_flag ? len + 2: len,
                                wiota_subframe_info.broadcast_first_subfram_data_len,
                                mcs_table_without_crc_03[wiota_subframe_info.symbollength][current_mcs]);
    }

    if (wiota_subframe_info.current_subframe != sub_fram_num)
    {
        uc_wiota_set_subframe_num(sub_fram_num);
        wiota_subframe_info.current_subframe = sub_fram_num;
    }

    return 0;
}

#endif
