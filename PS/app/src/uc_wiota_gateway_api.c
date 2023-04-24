
#include <rtthread.h>
#ifdef WIOTA_ASYNC_GATEWAY_API
#include <rtdevice.h>
#include "uc_wiota_gateway_api.h"
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "uc_coding.h"
#include "wiota_subframe.h"
#include "uc_ota_flash.h"

#define GATEWAY_OTA_BLOCK_SIZE           512

#define SET_BIT(value, bit)             (value |= (1 << bit))
#define CLEAR_BIT(value, bit)           (value &= ~(1 << bit))
#define JUDGMENT_BIT(value, bit)        (value >> bit & 1)

uc_wiota_gateway_api_mode_t gateway_mode = {0};
static unsigned int gateway_wiota_id = 0xffffffff;
static unsigned char wiota_freq_num = 0xff;
static boolean wiota_gateway_mode_flag = RT_FALSE;


static void uc_wiota_gateway_set_dev_address(unsigned int dev_address)
{
    gateway_mode.dev_address = dev_address;
}

static unsigned int uc_wiota_gateway_get_dev_address(void)
{
    return gateway_mode.dev_address;
}

static void *uc_wiota_gateway_create_queue(const char *name, unsigned int max_msgs, unsigned char flag)
{
    rt_mq_t mq;
    void *msgpool;

    mq = rt_malloc(sizeof(struct rt_messagequeue));
    if (RT_NULL == mq)
    {
        return RT_NULL;
    }

    msgpool = rt_malloc(4 * max_msgs);
    if ( RT_NULL == msgpool)
    {
        rt_free(mq);
        return RT_NULL;
    }

    if (RT_EOK != rt_mq_init(mq, name, msgpool, 4, 4 * max_msgs, flag))
    {
        rt_free(mq);
        rt_free(msgpool);
        return RT_NULL;
    }

    return (void *)mq;
}

static int uc_wiota_gateway_recv_queue(void *queue, void **buf, signed int timeout)
{
    unsigned int address = 0;
    int result = 0;

    if(queue == RT_NULL)
    {
        rt_kprintf("uc_wiota_gateway_recv_queue null.\n");
        return RT_ERROR;
    }

    result = rt_mq_recv(queue, &address, 4, timeout);
    *buf = (void *)address;

    return result;
}

static int uc_wiota_gateway_send_queue(void *queue, void *buf, signed int timeout)
{
    unsigned int address = (unsigned int)buf;

    if(queue == RT_NULL)
    {
        rt_kprintf("uc_wiota_gateway_send_queue send null point.\n");
        return 0;
    }

    return rt_mq_send_wait(queue, &address, 4, timeout);
}

static int uc_wiota_gateway_dele_queue(void *queue)
{
    rt_err_t ret = rt_mq_detach(queue);
    rt_free(((rt_mq_t)queue)->msg_pool);
    rt_free(queue);
    return ret;
}

static void uc_wiota_gateway_send_msg_to_queue(void* data)
{
    if(RT_EOK != uc_wiota_gateway_send_queue(gateway_mode.gateway_mq, data, 0))
    {
        rt_kprintf("uc_wiota_gateway_send_msg_to_queue error.\n");
    }
}


void gateway_recv_callback(uc_recv_back_p data)
{
    if (data->data != RT_NULL && (UC_OP_SUCC == data->result || UC_OP_PART_SUCC == data->result ))
    {
        uc_wiota_gateway_msg_t *recv_data_buf = RT_NULL;

        recv_data_buf = rt_malloc(sizeof(uc_wiota_gateway_msg_t));
        if(recv_data_buf == NULL)
        {
            rt_kprintf("uc_wiota_gateway_recv_data_callback req recv_data_buf memory failed.\n");
            return;
        }
        recv_data_buf->data = rt_malloc(sizeof(uc_recv_back_t));
        if(recv_data_buf->data == NULL)
        {
            rt_kprintf("uc_wiota_gateway_recv_data_callback req recv_data_buf->data  memory failed.\n");
            rt_free(recv_data_buf);
            return;
        }

        rt_memcpy(recv_data_buf->data, data, sizeof(uc_recv_back_t));

        recv_data_buf->msg_type = GATEWAY_MAG_DL_RECV;
    
        if(RT_EOK != uc_wiota_gateway_send_queue(gateway_mode.gateway_mq, recv_data_buf, 10))
        {
            rt_free(data->data);
            rt_kprintf("uc_wiota_gateway_recv_data_callback send queue failed.\n");
        }
    }
}

static int uc_wiota_gateway_send_ps_cmd_data(unsigned char *data,
    int len,
    app_ps_header_t *ps_header)
{
    int ret = 0;
    unsigned char *cmd_coding = data;
    unsigned int cmd_coding_len = len;
    unsigned char *data_coding = RT_NULL;
    unsigned int data_coding_len = 0;
    UC_OP_RESULT send_result = 0;

    if (0 != app_data_coding(ps_header, cmd_coding, cmd_coding_len, &data_coding, &data_coding_len))
    {
        rt_kprintf("uc_wiota_gateway_send_ps_cmd_data coding head failed.\n");
        rt_free(cmd_coding);
        return ret;
    }
    gateway_wiota_set_subframe(WIOTA_SEND_UNICAST, ASYNC_WIOTA_DEFAULT_MCS_LEVEL, data_coding_len);
    // wiota send data
    send_result = uc_wiota_send_data(uc_wiota_get_gateway_id(), data_coding, data_coding_len, RT_NULL, 0, 10000, RT_NULL);
    if(send_result == UC_OP_SUCC)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    rt_kprintf("uc_wiota_gateway_send_ps_cmd_data send data res %d.\n", send_result);

    ///rt_kprintf
    // rt_kprintf("send data_len %d:", data_coding_len);
    // for (int i = 0; i < data_coding_len; i++)
    //     rt_kprintf("%x ", data_coding[i]);
    // rt_kprintf("\n");


    //rt_free(cmd_coding);
    rt_free(data_coding);

    return ret;
}

static boolean uc_wiota_gateway_check_if_upgrade_required(app_ps_ota_upgrade_req_t *ota_upgrade_req)
{
    boolean is_upgrade_range = FALSE;
    boolean is_required = FALSE;
    unsigned int dev_address = uc_wiota_gateway_get_dev_address();
    u8_t version[15] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL,RT_NULL);

    if (ota_upgrade_req->upgrade_range == 1)
    {
        for (int i = 0; i < APP_MAX_IOTE_UPGRADE_NUM; i++)
        {
            if (dev_address == ota_upgrade_req->iote_list[i])
            {
                if (gateway_mode.ota_state == GATEWAY_OTA_STOP)
                {
                    gateway_mode.ota_state = GATEWAY_OTA_DEFAULT;
                }

                is_upgrade_range = TRUE;
                break;
            }
        }
    }
    else if (ota_upgrade_req->upgrade_range == 0 && gateway_mode.ota_state != GATEWAY_OTA_STOP)
    {
        is_upgrade_range = TRUE;
    }

    if (is_upgrade_range)
    {
        if (0 == rt_strncmp((char*)version, ota_upgrade_req->old_version, rt_strlen(ota_upgrade_req->old_version)) &&
            0 == rt_strncmp(gateway_mode.device_type, ota_upgrade_req->device_type, rt_strlen(ota_upgrade_req->device_type)))
        {
            is_required = TRUE;
        }
    }

    return is_required;
}

static boolean uc_wiota_gateway_whether_the_ota_upgrade_data_is_recved(void)
{
    unsigned int offset = 0;
    unsigned int block_count = 0;

    for (; offset < gateway_mode.block_count; offset++)
    {
        if (0x1 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
        {
            block_count++;
        }
    }

    rt_kprintf("gateway_ota_download %d/%d\r\n", block_count, gateway_mode.block_count);

    if (block_count >= gateway_mode.block_count)
    {
        rt_kprintf("ota data recv end\n");
        return TRUE;
    }

    return FALSE;
}

static void uc_wiota_gateway_send_miss_data_req_to_queue(void)
{
    // if(!gateway_mode.miss_data_req)
    {
        static uc_wiota_gateway_msg_t miss_data_req_msg;

        miss_data_req_msg.data = RT_NULL;
        miss_data_req_msg.msg_type = GATEWAY_MAG_CODE_UL_MISS_DATA_REQ;
    
        uc_wiota_gateway_send_msg_to_queue((void *)&miss_data_req_msg);
        gateway_mode.miss_data_req = RT_TRUE;
    }
}

static void uc_wiota_gateway_ota_upgrade_res_msg(unsigned char *data, unsigned int data_len)
{
    //unsigned char *cmd_decoding = RT_NULL;
    app_ps_ota_upgrade_req_t *ota_upgrade_req = RT_NULL;
    u8_t version[15] = {0};
    int bin_size = 0;
    int reserved_size = 0;
     int ota_size = 0;

    uc_wiota_get_version(version, RT_NULL, RT_NULL,RT_NULL);

     get_partition_size(&bin_size , &reserved_size , &ota_size);
     rt_kprintf("bin size:%d, reserved_size:%d, ota_size:%d\r\n", bin_size, reserved_size, ota_size);
    #if 0
    if (app_cmd_decoding(OTA_UPGRADE_REQ, data, data_len, &cmd_decoding) < 0)
    {
        rt_kprintf("%s line %d app_cmd_decoding error\n", __FUNCTION__, __LINE__);
        return;
    }
    #endif
    ota_upgrade_req = (app_ps_ota_upgrade_req_t *)data;

    if (uc_wiota_gateway_check_if_upgrade_required(ota_upgrade_req))
    {
        int file_size = ota_upgrade_req->file_size;

        if (gateway_mode.ota_state == GATEWAY_OTA_DEFAULT)
        {
            unsigned int mask_map_size = file_size / GATEWAY_OTA_BLOCK_SIZE / 8 + 1;

            gateway_mode.mask_map = rt_malloc(mask_map_size);
            RT_ASSERT(gateway_mode.mask_map);
            rt_memset(gateway_mode.mask_map, 0x00, mask_map_size);

            gateway_mode.block_size = GATEWAY_OTA_BLOCK_SIZE;
            gateway_mode.block_count = file_size / GATEWAY_OTA_BLOCK_SIZE;
            if (file_size % GATEWAY_OTA_BLOCK_SIZE)
            {
                gateway_mode.block_count++;
            }

            uc_wiota_ota_flash_erase(bin_size + reserved_size, file_size);
            gateway_mode.ota_state = GATEWAY_OTA_DOWNLOAD;
            gateway_mode.upgrade_type = ota_upgrade_req->upgrade_type;
            rt_strncpy(gateway_mode.new_version, ota_upgrade_req->new_version, rt_strlen(ota_upgrade_req->new_version));

            rt_timer_control(gateway_mode.ota_timer, RT_TIMER_CTRL_SET_TIME, (void *)&ota_upgrade_req->upgrade_time);
            rt_timer_start(gateway_mode.ota_timer);

            rt_kprintf("GATEWAY_OTA_DEFAULT file_size %d, mask_map_size %d, block_size %d, block_count %d, ota_state %d, upgrade_type %d, upgrade_time %d\n",
                       file_size, mask_map_size, gateway_mode.block_size, gateway_mode.block_count, gateway_mode.ota_state, gateway_mode.upgrade_type, ota_upgrade_req->upgrade_time);
            rt_kprintf("new_version %s, old_version %s, device_type %s\n", gateway_mode.new_version, version, gateway_mode.device_type);
        }

        if (gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
        {
            unsigned int offset = ota_upgrade_req->data_offset / GATEWAY_OTA_BLOCK_SIZE;

            // if (gateway_mode.miss_data_req)
            {
                int timeout = ota_upgrade_req->upgrade_time / gateway_mode.block_count * gateway_mode.miss_data_num + 5000;

                rt_timer_control(gateway_mode.ota_timer, RT_TIMER_CTRL_SET_TIME, (void *)&timeout);
                rt_timer_start(gateway_mode.ota_timer);
                gateway_mode.miss_data_req = FALSE;
                rt_kprintf("miss_data_req recv begin, upgrade_time %d\n", timeout);
            }

            if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
            {
                uc_wiota_ota_flash_write(ota_upgrade_req->data, bin_size + reserved_size + ota_upgrade_req->data_offset, ota_upgrade_req->data_length);
                SET_BIT(gateway_mode.mask_map[offset / 8], offset % 8);
                rt_kprintf("GATEWAY_OTA_DOWNLOAD offset %d mask_map[%d] = 0x%x\n", offset, offset / 8, gateway_mode.mask_map[offset / 8]);
            }

            if (uc_wiota_gateway_whether_the_ota_upgrade_data_is_recved())
            {
                if (0 == uc_wiota_ota_check_flash_data(bin_size + reserved_size, file_size, ota_upgrade_req->md5))
                {
                    rt_kprintf("ota data checkout ok, jump to program\n");

                    gateway_mode.ota_state = GATEWAY_OTA_PROGRAM;
                    rt_free(gateway_mode.mask_map);
                    gateway_mode.mask_map = RT_NULL;
                    rt_timer_stop(gateway_mode.ota_timer);

                    wiota_gateway_mode_flag = RT_FALSE;
                    //set_partition_size(GATEWAY_OTA_FLASH_BIN_SIZE, GATEWAY_OTA_FLASH_REVERSE_SIZE, GATEWAY_OTA_FLASH_OTA_SIZE);
                    uc_wiota_ota_jump_program(file_size, ota_upgrade_req->upgrade_type);
                }
                else
                {
                    rt_kprintf("ota data checkout error, upgrade fail\n");

                    gateway_mode.ota_state = GATEWAY_OTA_DEFAULT;
                    rt_free(gateway_mode.mask_map);
                    gateway_mode.mask_map = RT_NULL;
                    rt_timer_stop(gateway_mode.ota_timer);
                }
            }
        }
    }

    //rt_free(cmd_decoding);
}

static void uc_wiota_gateway_ota_upgrade_stop_msg(unsigned char *data, unsigned int data_len)
{
    //unsigned char *cmd_decoding = RT_NULL;
    app_ps_ota_upgrade_stop_t *ota_upgrade_stop = RT_NULL;
    unsigned int dev_address = uc_wiota_gateway_get_dev_address();
#if 0
    if (app_cmd_decoding(OTA_UPGRADE_STOP, data, data_len, &cmd_decoding) < 0)
    {
        rt_kprintf("%s line %d app_cmd_decoding error\n", __FUNCTION__, __LINE__);
        return;
    }
#endif
    ota_upgrade_stop = (app_ps_ota_upgrade_stop_t *)data;

    for (int i = 0; i < APP_MAX_IOTE_UPGRADE_STOP_NUM; i++)
    {
        rt_kprintf("iote_list 0x%x\n", ota_upgrade_stop->iote_list[i]);
        if (dev_address == ota_upgrade_stop->iote_list[i])
        {
            if (gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
            {
                rt_free(gateway_mode.mask_map);
                gateway_mode.mask_map = RT_NULL;
                rt_timer_stop(gateway_mode.ota_timer);
            }
            gateway_mode.ota_state = GATEWAY_OTA_STOP;
            rt_kprintf("0x%x stop ota upgrade\n", dev_address);
            break;
        }
    }

    //rt_free(cmd_decoding);
}

static void uc_wiota_gateway_ota_upgrade_state_msg(unsigned char *data, unsigned int data_len)
{
    //unsigned char *cmd_decoding = RT_NULL;
    app_ps_ota_upgrade_state_t *ota_upgrade_state = (app_ps_ota_upgrade_state_t *)data;
    u8_t version[15] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL,RT_NULL);
#if 0
    if (app_cmd_decoding(OTA_UPGRADE_STATE, data, data_len, &cmd_decoding) < 0)
    {
        rt_kprintf("%s line %d app_cmd_decoding error\n", __FUNCTION__, __LINE__);
        return;
    }
    ota_upgrade_state = (app_ps_ota_upgrade_state_t *)data;
#endif
    if (0 == rt_strncmp(ota_upgrade_state->old_version, (const char*)version, rt_strlen((const char *)version)) &&
        0 == rt_strncmp(ota_upgrade_state->new_version, gateway_mode.new_version, rt_strlen(gateway_mode.new_version)) &&
        0 == rt_strncmp(ota_upgrade_state->device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type)) &&
        ota_upgrade_state->upgrade_type == gateway_mode.upgrade_type)
    {
        if (ota_upgrade_state->process_state == GATEWAY_OTA_END && gateway_mode.ota_state == GATEWAY_OTA_DOWNLOAD)
        {
            rt_kprintf("recv GATEWAY_OTA_END cmd, checkout mask_map\n");
            // rt_timer_stop(gateway_mode.ota_timer);
            for (int offset = 0; offset < gateway_mode.block_count; offset++)
            {
                if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
                {
                    gateway_mode.miss_data_num++;
                }
            }

            if (gateway_mode.miss_data_num > 0)
            {
                rt_kprintf("there are %d packets recvived not completely, send miss data req\n", gateway_mode.miss_data_num);
                uc_wiota_gateway_send_miss_data_req_to_queue();
            }
        }
    }

    //rt_free(cmd_decoding);
}


static void uc_gateway_analisys_dl_msg(unsigned char type, unsigned char *data, unsigned int data_len)
{
    app_ps_header_t ps_header = {0};
    unsigned char *data_decoding = RT_NULL;
    unsigned int data_decoding_len = 0;

    if (0 != app_data_decoding(data, data_len, &data_decoding, &data_decoding_len, &ps_header))
    {
        rt_kprintf("uc_wiota_gateway_analisys_dl_msg decode failed.\n");
        return;
    }
    
    switch (ps_header.cmd_type)
    {
        case SYSTEM_MSG:
        {
            app_system_msg_t *sys_msg = (app_system_msg_t *)data_decoding;
            sys_msg = sys_msg;
            
            break;
        }
        case POLL_MSG:
        case VERSION_VERIFY:
        case IOTE_MISSING_DATA_REQ:
        case IOTE_STATE_UPDATE:
        {
            break;
        }
        case OTA_UPGRADE_REQ:
            uc_wiota_gateway_ota_upgrade_res_msg(data_decoding, data_decoding_len);
            break;
        case OTA_UPGRADE_STOP:
            uc_wiota_gateway_ota_upgrade_stop_msg(data_decoding, data_decoding_len);
            break;
        case OTA_UPGRADE_STATE:
            uc_wiota_gateway_ota_upgrade_state_msg(data_decoding, data_decoding_len);
            break;
        case IOTE_USER_DATA:
        {
            if (RT_NULL != gateway_mode.recv_cb)
            {
                gateway_mode.recv_cb(ps_header.addr.src_addr, type, data_decoding, data_decoding_len);
            }
            break;
        }
    }

    rt_free(data_decoding);
}

static int uc_wiota_gateway_send_ota_req_msg(void)
{
    app_ps_version_verify_t version_verify = {0};
    app_ps_header_t ps_header = {0};
    u8_t version[15] = {0};

    if(wiota_gateway_mode_flag)
    {
        uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);

        rt_strncpy(version_verify.software_version, (const char*)version, rt_strlen((const char*)version));
        uc_wiota_get_hardware_ver((unsigned char *)version_verify.hardware_version);
        rt_strncpy(version_verify.device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type));

        app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
        ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
        ps_header.cmd_type = VERSION_VERIFY;
        ps_header.packet_num = app_packet_num();

        return uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&version_verify,
            sizeof(app_ps_version_verify_t),
            &ps_header);
    }

    return 0;
}

static void uc_wiota_gateway_ota_miss_data_req_msg(void)
{
    app_ps_header_t ps_header = {0};
    app_ps_iote_missing_data_req_t miss_data_req = {0};
    u8_t version[15] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL,RT_NULL);

    miss_data_req.miss_data_num = 0;
    for (int offset = 0; offset < gateway_mode.block_count; offset++)
    {
        if (0x0 == JUDGMENT_BIT(gateway_mode.mask_map[offset / 8], offset % 8))
        {
            miss_data_req.miss_data_offset[miss_data_req.miss_data_num] = offset * GATEWAY_OTA_BLOCK_SIZE;
            miss_data_req.miss_data_length[miss_data_req.miss_data_num] = GATEWAY_OTA_BLOCK_SIZE;
            rt_kprintf("miss_data_offset[%d] %d, miss_data_length[%d] %d\n",
                       miss_data_req.miss_data_num, miss_data_req.miss_data_offset[miss_data_req.miss_data_num],
                       miss_data_req.miss_data_num, miss_data_req.miss_data_length[miss_data_req.miss_data_num]);
            miss_data_req.miss_data_num++;
            if (offset == APP_MAX_MISSING_DATA_BLOCK_NUM - 1)
            {
                break;
            }
        }
    }
    if (miss_data_req.miss_data_num == 0)
    {
        return;
    }

    miss_data_req.upgrade_type = gateway_mode.upgrade_type;
    rt_strncpy(miss_data_req.device_type, gateway_mode.device_type, rt_strlen(gateway_mode.device_type));
    rt_strncpy(miss_data_req.new_version, gateway_mode.new_version, rt_strlen(gateway_mode.new_version));
    rt_strncpy(miss_data_req.old_version, (const char*)version, rt_strlen((const char*)version));

    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    ps_header.addr.src_addr = uc_wiota_gateway_get_dev_address();
    ps_header.cmd_type = IOTE_MISSING_DATA_REQ;
    ps_header.packet_num = app_packet_num();

    uc_wiota_gateway_send_ps_cmd_data((unsigned char *)&miss_data_req,
        sizeof(app_ps_iote_missing_data_req_t),
        &ps_header);
}

static void uc_wiota_gateway_task(void *para)
{
    uc_wiota_gateway_msg_t *recv_data;
    
    while(1)
    {
        if (RT_EOK != uc_wiota_gateway_recv_queue(gateway_mode.gateway_mq, (void **)&recv_data, RT_WAITING_FOREVER))
        {
            continue;
        }
        
        rt_kprintf("recv_data->msg_type %d\n", recv_data->msg_type);
        
        switch(recv_data->msg_type)
        {
            case GATEWAY_MAG_DL_RECV:
            {
                uc_recv_back_p wiota_data = recv_data->data;
                
                uc_gateway_analisys_dl_msg(wiota_data->type, wiota_data->data, wiota_data->data_len);
                
                rt_free(wiota_data->data);
                rt_free(recv_data->data);
                rt_free(recv_data);
                break;
            }
            case GATEWAY_MAG_CODE_OTA_REQ:
                uc_wiota_gateway_send_ota_req_msg();
                break;
            case GATEWAY_MAG_CODE_UL_MISS_DATA_REQ:
                uc_wiota_gateway_ota_miss_data_req_msg();
                break;
            default:
                break;
        }
        
    }
}

static void uc_gateway_handle_ota_recv_timer_msg(void *para)
{
    if(wiota_gateway_mode_flag)
    {
        uc_wiota_gateway_send_miss_data_req_to_queue();
    }
    //else
    //{
    //    rt_kprintf("uc_wiota_gateway_handle_ota_recv_timer_msg error.\n");
    //}
    // uc_wiota_gateway_send_msg_to_queue(GATEWAY_MAG_CODE_OTA_REQ);
}

static void uc_gateway_handle_ota_req_timer(void *para)
{
    if(wiota_gateway_mode_flag)
    {
        static uc_wiota_gateway_msg_t ota_req_msg;

        ota_req_msg.data = RT_NULL;
        ota_req_msg.msg_type = GATEWAY_MAG_CODE_OTA_REQ;
    
        uc_wiota_gateway_send_msg_to_queue((void *)&ota_req_msg);
    }
    else
    {
        rt_kprintf("uc_wiota_gateway_handle_ota_req_timer error.\r\n");
    }

    // if(wiota_gateway_mode_flag)
    // {
    //     uc_wiota_gateway_send_ota_req_msg();
    // }
    // else
    // {
    //     rt_kprintf("uc_wiota_gateway_handle_ota_req_timer error.\r\n");
    // }
}

int uc_wiota_gateway_scantf_freq(unsigned int subsystemid, unsigned char *freq_list, unsigned char freq_list_len)
{
    int freq_res = -1;
    unsigned char scantf_count = 0;
    unsigned char *scantf_result = RT_NULL;

    if (freq_list_len == 0)
    {
        return -2;
    }

    scantf_result = rt_malloc(freq_list_len);
    if (scantf_result == RT_NULL)
    {
        return -3;
    }
    rt_memset(scantf_result, 0x00, freq_list_len);

    for (unsigned char index = 0; index < freq_list_len; index++)
    {
        uc_wiota_set_freq_info(freq_list[index]);
        unsigned int gateway_id = (1 << 31) | ((subsystemid & 0xffff) << 8) | (freq_list[index] & 0xff);
        for (unsigned char offset = 0; offset < 3; offset++)
        {
            unsigned char scantf_data[1];
            scantf_data[0] = scantf_count++;                
            gateway_wiota_set_subframe(WIOTA_SEND_UNICAST, ASYNC_WIOTA_DEFAULT_MCS_LEVEL, 1);
            if (UC_OP_SUCC == uc_wiota_send_data(gateway_id, scantf_data, 1, NULL,  0, 5000, RT_NULL))
            {
                scantf_result[index] = 1;
                break;
            }
        }
        rt_kprintf("scantf freq:%d resunt:%d\r\n", freq_list[index], scantf_result[index]);
    }
    for (unsigned char index = 0; index < freq_list_len; index++)
    {
        if (scantf_result[index])
        {
            freq_res = freq_list[index];
            break;
        }
    }
    rt_free(scantf_result);

    return freq_res;
}

int uc_wiota_gateway_init(unsigned char crc_flag, 
    unsigned char power, 
    unsigned char order_scanf_freq,
    unsigned char default_freq,
    unsigned char broadcast_send_counter,  
    UC_GATEWAY_CONTINUE_MODE_TYPE continue_mode, 
    unsigned char send_detect_time, 
    unsigned char unicast_send_counter)
{
    // unsigned char serial[16] ={0};
    unsigned char len;
    unsigned int user_id = 0;
    sub_system_config_t config;
    unsigned char freq_num = 0;

    if (gateway_mode.state != GATEWAY_DEFAULT && gateway_mode.state !=GATEWAY_END)
    {
        return UC_GATEWAY_OTHER_FAIL;
    }

    // init para
    rt_strncpy(gateway_mode.device_type, "iote", 4);
    gateway_mode.gateway_mode = RT_TRUE;
    wiota_gateway_mode_flag = RT_TRUE;

    // init queue
    gateway_mode.gateway_mq = uc_wiota_gateway_create_queue("gw_mq", 5, RT_IPC_FLAG_PRIO);
    if(gateway_mode.gateway_mq == RT_NULL)
    {
        // rt_kprintf("req queue memory failed.\n");
        return UC_CREATE_OTATIMER_FAIL;
    }

    gateway_mode.ota_timer = rt_timer_create("t_ota",
                                            uc_gateway_handle_ota_recv_timer_msg,
                                            RT_NULL,
                                            10000,      //tick
                                            // RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
                                            RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if(gateway_mode.ota_timer == RT_NULL)
    {
        uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
        //rt_kprintf("uc_wiota_gateway_start req ota timer memory failed.\n");
        return UC_CREATE_MISSTIMER_FAIL;
    }
    
    // start ota timer
    gateway_mode.ver_timer = rt_timer_create("t_ver",
                                            uc_gateway_handle_ota_req_timer,
                                            RT_NULL,
                                            GATEWAY_OTA_VER_PERIOD,
                                            RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    
    if (RT_NULL == gateway_mode.ver_timer)
    {
        rt_timer_delete(gateway_mode.ota_timer);
        uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
        // rt_kprintf("%s line %d create timer fail\n", __FUNCTION__, __LINE__);
        return UC_CREATE_OTATIMER_FAIL;
    }

    // memcpy(gateway_mode.sub_system_id, sub_sys_id_list, sub_sys_id_len);

    // init wiota.
    uc_wiota_init();

    //get userid
    uc_wiota_get_userid(&user_id, &len);
    
    // set wiota id
    uc_wiota_set_userid( &user_id , 4);

    // set crc
    uc_wiota_set_crc(crc_flag & 0x1);

   // set parament.
    uc_wiota_get_system_config(&config);
    uc_wiota_set_system_config(&config);

    uc_wiota_set_data_rate(0, ASYNC_WIOTA_DEFAULT_MCS_LEVEL);
   
    uc_wiota_set_bc_round(broadcast_send_counter< 1 ? 1: broadcast_send_counter);
    
    uc_wiota_set_detect_time(send_detect_time);
    
    uc_wiota_set_cur_power(power);

    uc_wiota_set_unisend_fail_cnt(unicast_send_counter);

    gateway_wiota_subframe_init(config.symbol_length, crc_flag & 0x1);

    uc_wiota_run();

    if (order_scanf_freq)
    {
        unsigned char freq_list_len = 0;
        for (unsigned char index = 0; index < 16; index++)
        {
            if (config.freq_list[index] != 255)
            {
                freq_list_len++;
            }
        }
        int res = uc_wiota_gateway_scantf_freq(config.subsystemid, config.freq_list, freq_list_len);
        if (res >= 0)
        {
            freq_num = (unsigned char)res;
        }
        else
        {
            freq_num = default_freq;
        }
    }
    else
    {
        freq_num = default_freq;
    }
    uc_wiota_set_freq_info(freq_num);
    wiota_freq_num = freq_num;

    gateway_wiota_id = (1 << 31) | ((config.subsystemid & 0xffff) << 8) | (freq_num & 0xff);

    // set wiota callback
    uc_wiota_register_recv_data_callback(gateway_recv_callback, UC_CALLBACK_NORAMAL_MSG);

    // create gatway logic task
    gateway_mode.gateway_handler = rt_thread_create("gateway",
                                                    uc_wiota_gateway_task,
                                                    RT_NULL,
                                                    1024,
                                                    5,
                                                    3);
    
    if(gateway_mode.gateway_handler == RT_NULL)
    {
        // rt_kprintf("rt_thread_create task failed.\n");
        rt_timer_delete(gateway_mode.ota_timer);
        rt_timer_delete(gateway_mode.ver_timer);
        uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
        return UC_CREATE_TASK_FAIL;
    }

    // start task
    rt_thread_startup(gateway_mode.gateway_handler);

    gateway_mode.state = GATEWAY_NORMAL;
    uc_wiota_gateway_set_dev_address(user_id);

    rt_timer_start(gateway_mode.ver_timer);

    rt_kprintf("freq_num = %d, gateway_wiota_id = 0x%08x\r\n", freq_num, gateway_wiota_id);

    return UC_GATEWAY_OK;
    
}

int uc_wiota_gateway_exit(void)
{
    if (gateway_mode.state != GATEWAY_NORMAL)
    {
        rt_kprintf("gateway no start\n");
        return 1;
    }
    
    if(gateway_mode.ota_timer != RT_NULL)
    {
        rt_timer_delete(gateway_mode.ota_timer);
        gateway_mode.ota_timer = RT_NULL;
    } 
    
    if(gateway_mode.ver_timer != RT_NULL)
    {
        rt_timer_delete(gateway_mode.ver_timer);
        gateway_mode.ver_timer = RT_NULL;
    }  
    
    if(gateway_mode.gateway_handler != RT_NULL)
    {
        rt_thread_delete(gateway_mode.gateway_handler);
        gateway_mode.gateway_handler = RT_NULL;
    }


    while (1)
    {
        uc_wiota_gateway_msg_t *recv_data;
        int  ret = uc_wiota_gateway_recv_queue(gateway_mode.gateway_mq, (void **)&recv_data, 0);
        if (RT_EOK != ret)
            break;
    
        if (recv_data->msg_type == GATEWAY_MAG_DL_RECV)
        {
            uc_recv_back_p wiota_data = recv_data->data;
            rt_free(wiota_data->data);
            rt_free(recv_data);
        }            
    }

    if(gateway_mode.gateway_mq != RT_NULL)
    {
        uc_wiota_gateway_dele_queue(gateway_mode.gateway_mq);
        gateway_mode.gateway_mq = RT_NULL;
    }

    gateway_mode.state = GATEWAY_END;
    gateway_mode.gateway_mode = RT_FALSE;
    wiota_gateway_mode_flag = RT_FALSE;

    wiota_freq_num = 0xff;
    gateway_wiota_id = 0xffffffff;

    return 0;
}

int uc_wiota_gateway_send(unsigned int id, 
        void *data, 
        int len, 
        int timeout, 
        uc_gateway_send send_callback)
{
    unsigned char *data_coding = RT_NULL;
    unsigned int data_coding_len = 0;
    app_ps_header_t ps_header = {0};
    unsigned int userid;
    unsigned char userid_len;

    if (gateway_mode.state != GATEWAY_NORMAL)
    {
        rt_kprintf("gateway no start\n");
        return 1;
    }
    
    if (len > GATEWAY_SEND_MAX_LEN)
    {
        rt_kprintf("uc_wiota_gateway_send_data len error.len %d\n", len);
        return 0;
    }

    uc_wiota_get_userid(&userid, &userid_len);

    // coding protocol
    app_set_header_property(PRO_SRC_ADDR, 1, &ps_header.property);
    ps_header.cmd_type = IOTE_USER_DATA;
    ps_header.addr.src_addr = userid;
    app_set_header_property(PRO_PACKET_NUM, 1, &ps_header.property);
    ps_header.packet_num = app_packet_num();
    
    if (0 != app_data_coding(&ps_header, data, len, &data_coding, &data_coding_len))
    {
        rt_kprintf("uc_wiota_gateway_send_data code failed.\n");
        return 0;
    }
                
    gateway_wiota_set_subframe(0 != id ? WIOTA_SEND_UNICAST : WIOTA_SEND_BROADCAST, 
                                    ASYNC_WIOTA_DEFAULT_MCS_LEVEL, 
                                    data_coding_len);

    // wiota send data
    if (UC_OP_SUCC != uc_wiota_send_data(id, data_coding, data_coding_len, NULL, 0, timeout, (uc_send)send_callback))
    {
        rt_kprintf("uc_wiota_send_data error\n");
        rt_free(data_coding);
         return 1;
    }
    rt_free(data_coding);

    return 0;
}

unsigned int uc_wiota_get_gateway_id(void)
{
    return gateway_wiota_id;
}

unsigned char uc_wiota_get_freq_num(void)
{
    return wiota_freq_num;
}

int uc_wiota_gateway_set_recv_register(uc_gateway_recv recv_callback)
{
    // set gateway recv callback
    gateway_mode.recv_cb = recv_callback;    
    return 0;
}

unsigned char uc_wiota_gateway_get_state(void)
{
    return gateway_mode.state;
}

int uc_wiota_gateway_set_state_register(uc_gateway_state state_callback)
{
    return 0;
}

int uc_wiota_gateway_ota_req(void)
{
    if(gateway_mode.gateway_mode)
    {
        return uc_wiota_gateway_send_ota_req_msg();
    }

    return 0;
}

int uc_wiota_gateway_set_ota_period(unsigned int p_tick)
{
    if((gateway_mode.ver_timer != RT_NULL)\
        && (p_tick > GATEWAY_OTA_VER_PERIOD)\
        && (wiota_gateway_mode_flag))
    {
        rt_timer_control(gateway_mode.ver_timer, RT_TIMER_CTRL_SET_TIME, &p_tick);
        rt_timer_start(gateway_mode.ver_timer);
        return 1;
    }
    return 0;
}

#endif
