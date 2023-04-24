#ifndef _UC_WIOTA_GATEWAY_API_H_
#define _UC_WIOTA_GATEWAY_API_H_

#include "rtthread.h"
#include "uc_wiota_api.h"

typedef struct uc_wiota_gateway_msg
{
    void *data;
    unsigned char msg_type;
}uc_wiota_gateway_msg_t;


#define ASYNC_WIOTA_DEFAULT_MCS_LEVEL 1
#define GATEWAY_OTA_VER_PERIOD (7200000) // 2 hours
#define GATEWAY_SEND_MAX_LEN 299


typedef enum
{
    UC_GATEWAY_CLOSE_CONTINUE_MODE = 0,
    UC_GATEWAY_BROADCAST_CONTINUE_MODE,
    UC_GATEWAY_UNICAST_CONTINUE_MODE,
    UC_GATEWAY_BROADCAST_UNICAST_CONT_MODE,
} UC_GATEWAY_CONTINUE_MODE_TYPE;

typedef enum {
    UC_GATEWAY_OK = 0,
    UC_GATEWAY_NO_CONNECT,
    UC_GATEWAY_TIMEOUT,
    UC_GATEWAY_NO_KEY,
    UC_CREATE_QUEUE_FAIL,
    UC_CREATE_MISSTIMER_FAIL,
    UC_CREATE_OTATIMER_FAIL,
    UC_CREATE_TASK_FAIL,
    UC_GATEWAY_AUTO_FAIL,
    UC_GATEWAY_OTHER_FAIL,
    UC_GATEWAY_SEDN_FAIL,
}uc_gateway_start_result;

typedef enum
{
    GATEWAY_DEFAULT = 0,
    GATEWAY_NORMAL ,
    GATEWAY_CONNECT,
    GATEWAY_END ,
} uc_gateway_state_t;

typedef enum {

    GATEWAY_MAG_DL_RECV = 0,
    GATEWAY_MAG_CODE_OTA_REQ,
    GATEWAY_MAG_CODE_UL_MISS_DATA_REQ,
}uc_gateway_MSG_TYPE;

typedef enum
{
    GATEWAY_OTA_DEFAULT = 0,
    GATEWAY_OTA_DOWNLOAD = 1,
    GATEWAY_OTA_PROGRAM = 2,
    GATEWAY_OTA_STOP = 3
} uc_gateway_ota_state_t;

typedef void (*uc_gateway_recv)(unsigned int id, unsigned char type, void* data, int len);
typedef void (*uc_gateway_send)(void* send_result);
typedef void (*uc_gateway_state)(int state);

typedef struct uc_wiota_gateway_api_mode
{
    //handler
    rt_thread_t gateway_handler;
    rt_mq_t gateway_mq;
    rt_timer_t ver_timer;

    boolean gateway_mode;
    char device_type[16];
    unsigned int dev_address;

    //ota para
    rt_timer_t ota_timer;
    char new_version[16];
    uc_gateway_ota_state_t ota_state;
    int upgrade_type;
    unsigned int block_count;
    unsigned int block_size;
    unsigned char *mask_map;
    int miss_data_num;
    boolean miss_data_req;

    // gateway run state
    unsigned char state;

    // gateway mode
    unsigned char continue_mode;

    // sub system id
    unsigned int sub_system_id[16];
    
    // recv callback function
    uc_gateway_recv recv_cb;
}uc_wiota_gateway_api_mode_t;


int uc_wiota_gateway_init(unsigned char crc_flag, 
    unsigned char power, 
    unsigned char order_scanf_freq,
    unsigned char default_freq,
    unsigned char broadcast_send_counter,  
    UC_GATEWAY_CONTINUE_MODE_TYPE continue_mode, 
    unsigned char send_detect_time, 
    unsigned char unicast_send_counter);

int uc_wiota_gateway_exit(void);

int uc_wiota_gateway_send(unsigned int id, void *data, int len, int timeout, uc_gateway_send send_callback);

int uc_wiota_gateway_set_recv_register(uc_gateway_recv recv_callback);

unsigned char uc_wiota_gateway_get_state(void);

int uc_wiota_gateway_set_state_register(uc_gateway_state state_callback);

unsigned int uc_wiota_get_gateway_id(void);

unsigned char uc_wiota_get_freq_num(void);

int uc_wiota_gateway_ota_req(void);

int uc_wiota_gateway_set_ota_period(unsigned int p_tick);

#endif
