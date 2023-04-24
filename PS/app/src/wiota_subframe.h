#ifndef  _WIOTA_SUBFRAME_H_
#define  _WIOTA_SUBFRAME_H_

#define MCS_NUM 8

#define ASYNC_WIOTA_DEFAULT_SUBFRAM 8
#define ASYNC_WIOTA_MAX_SUBFRAME 10
#define ASYNC_WIOTA_MIN_SUBFRAME 3

typedef enum
{
    WIOTA_SEND_UNICAST = 0,
    WIOTA_SEND_BROADCAST,
} WIOTA_DATA_TYPE;


typedef struct async_wiota_subframe
{
    short unicast_first_subfram_data_len;    // one sub sub fram data len
    short broadcast_first_subfram_data_len; // one sub sub fram data len
    unsigned char symbollength;
    unsigned char crc_flag;
    unsigned char current_subframe;
} t_async_wiota_subframe;

int gateway_wiota_subframe_init(unsigned char symbollength, unsigned char crc_flag);

int gateway_wiota_set_subframe(WIOTA_DATA_TYPE type, unsigned char current_mcs,unsigned int len);


#endif
