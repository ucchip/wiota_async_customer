
#ifndef _UC_WIOTA_API_H__
#define _UC_WIOTA_API_H__

typedef unsigned long long u64_t;
typedef signed long long s64_t;
typedef unsigned long ul32_t;
typedef signed long sl32_t;
typedef signed int s32_t;
typedef unsigned int u32_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef char n8_t;
typedef signed char s8_t;
typedef unsigned char u8_t;
typedef unsigned char boolean;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX_USER_ID_LEN 8

//200 k
#define WIOTA_FREQUENCE_STEP 20
//470M ~ 510M
#define WIOTA_FREQUENCE_POINT(freq_idx, base_freq) (base_freq + freq_idx * WIOTA_FREQUENCE_STEP)
//0 - 200
#define WIOTA_FREQUENCE_INDEX(freq, base_freq) ((freq - base_freq) / WIOTA_FREQUENCE_STEP)

#define CRC16_LEN 2

#define PARTIAL_FAIL_NUM 8

#define UC_USER_HEAD_SIZE 2

#define UC_BC_LENGTH_LIMIT 1021

#define UC_DATA_LENGTH_LIMIT 310

typedef enum
{
    UC_CALLBACK_NORAMAL_MSG = 0, // normal msg from ap
    UC_CALLBACK_STATE_INFO,      // state info
} UC_CALLBACK_DATA_TYPE;


typedef enum
{
    UC_RECV_MSG = 0,   // normal msg
    UC_RECV_BC,        // broadcast msg
    UC_RECV_SCAN_FREQ, // result of freq scan by riscv
    UC_RECV_PHY_RESET, // if phy reseted once, tell app
    UC_RECV_SYNC_STATE,// when subf recv mode, if sync succ, report UC_OP_SUCC, if track fail, report UC_OP_FAIL
    UC_RECV_ACC_DATA,  // acc voice data
    UC_RECV_MAX_TYPE,
} UC_RECV_DATA_TYPE;

typedef enum
{
    UC_STATUS_NULL = 0,
    UC_STATUS_RECV_TRY,
    UC_STATUS_RECVING,
    UC_STATUS_SENDING,
    UC_STATUS_SLEEP,
    UC_STATUS_ERROR,
} UC_WIOTA_STATUS;

typedef enum
{
    UC_OP_SUCC = 0,
    UC_OP_TIMEOUT,
    UC_OP_FAIL,
    UC_OP_PART_SUCC,
    UC_OP_CRC_FAIL,
    UC_OP_QUEUE_FULL,
} UC_OP_RESULT;

typedef enum
{
    UC_MCS_LEVEL_0 = 0,
    UC_MCS_LEVEL_1,
    UC_MCS_LEVEL_2,
    UC_MCS_LEVEL_3,
    UC_MCS_LEVEL_4,
    UC_MCS_LEVEL_5,
    UC_MCS_LEVEL_6,
    UC_MCS_LEVEL_7,
    UC_MCS_AUTO = 8,
} UC_MCS_LEVEL;

typedef enum
{
    UC_RATE_NORMAL = 0,
    UC_RATE_OTHER,
} UC_DATA_RATE_MODE;

typedef enum
{
    UC_LOG_UART = 0,
    UC_LOG_SPI,
} UC_LOG_TYPE;

typedef enum
{
    UC_STATS_READ = 0,
    UC_STATS_WRITE,
} UC_STATS_MODE;

typedef enum
{
    UC_STATS_TYPE_ALL = 0,
    UC_STATS_UNI_SEND_TOTAL,
    UC_STATS_UNI_SEND_SUCC,
    UC_STATS_BC_SEND_TOTAL,
    UC_STATS_BC_SEND_SUCC,
    UC_STATS_UNI_RECV_FAIL,
    UC_STATS_UNI_RECV_SUCC,
    UC_STATS_BC_RECV_FAIL,
    UC_STATS_BC_RECV_SUCC,
    UC_STATS_TYPE_MAX,
} UC_STATS_TYPE;

typedef enum
{
    MODULE_STATE_NULL = 0,
    MODULE_STATE_INIT,
    MODULE_STATE_RUN,
    MODULE_STATE_INVALID
} UC_TASK_STATE;

typedef enum
{
    FREQ_DIV_MODE_1 = 0,  // default: 96M
    FREQ_DIV_MODE_2 = 1,  // 48M
    FREQ_DIV_MODE_4 = 2,  // 24M
    FREQ_DIV_MODE_6 = 3,  // 16M
    FREQ_DIV_MODE_8 = 4,  // 12M
    FREQ_DIV_MODE_10 = 5, // 9.6M
    FREQ_DIV_MODE_12 = 6, // 8M
    FREQ_DIV_MODE_14 = 7, // 48/7 M
    FREQ_DIV_MODE_16 = 8, // 6M
    FREQ_DIV_MODE_32 = 9, // 3M
    FREQ_DIV_MODE_MAX,
} UC_FREQ_DIV_MODE_E;

typedef enum
{
    VOL_MODE_CLOSE = 0, // 1.82v
    VOL_MODE_OPEN = 1,  // 1.47v
    VOL_MODE_TEMP_MAX,
} UC_VOL_MODE_E;

typedef enum
{
    UC_AUTO_RECV_START = 0, // default
    UC_AUTO_RECV_STOP = 1,
    UC_AUTO_RECV_ERROR,
} UC_AUTO_RECV_MODE;

typedef enum
{
    ADJUST_MODE_SEND = 0,
    ADJUST_MODE_RECV = 1,
} UC_ADJUST_MODE;

typedef enum
{
    AWAKENED_CAUSE_HARD_RESET = 0,      // also watchdog reset, spi cs reset
    AWAKENED_CAUSE_SLEEP = 1,
    AWAKENED_CAUSE_PAGING = 2,
    AWAKENED_CAUSE_GATING = 3,          // no need care
    AWAKENED_CAUSE_FORCED_INTERNAL = 4, // not use
    AWAKENED_CAUSE_OTHERS,
} UC_AWAKENED_CAUSE;

typedef enum
{
    RF_STATUS_IDLE = 0,
    RF_STATUS_TX,
    RF_STATUS_RX,
    INVALD_RF_STATUS
} UC_RF_STATUS_E;

typedef struct
{
    unsigned char rssi; // absolute value, 0~150, always negative
    unsigned char ber;
    signed char snr;
    signed char cur_power;
    signed char min_power;
    signed char max_power;
    unsigned char cur_mcs;
    unsigned char max_mcs;
    signed int frac_offset;
} radio_info_t;

typedef struct
{
    char reserved0;
    unsigned char id_len;        // 0: 2, 1: 4, 2: 6, 3: 8
    unsigned char pp;            // 0: 1, 1: 2, 2: 4, 3: not use
    unsigned char symbol_length; // 0: 128, 1: 256, 2: 512, 3: 1024
    unsigned char pz;            // default 8
    unsigned char btvalue;       // bt from rf 1: 0.3, 0: 1.2
    unsigned char bandwidth;     // default 1, 200KHz
    unsigned char spectrum_idx;  // default 3, 470M~510M;
    unsigned int systemid;
    unsigned int subsystemid;
    unsigned char freq_list[16];
    unsigned char na[32];
} sub_system_config_t;

typedef struct
{
    unsigned int result;
    unsigned char *oriPtr;
} uc_send_back_t, *uc_send_back_p;

typedef struct
{
    void *sem;
    unsigned int result;
} uc_send_result_t, *uc_send_result_p;

typedef struct
{
    unsigned char result; // UC_OP_RESULT
    unsigned char type;
    unsigned short data_len;
    unsigned char rssi; // absolute value, 0~150, always negative
    signed char snr;
    unsigned char sender_pow;                   // the power of sender, only effective when UC_RECV_MSG
    unsigned char head_right;                   // indicate headData is right or not
    unsigned char head_data[UC_USER_HEAD_SIZE]; // bc head data of user
    unsigned short packet_size;                 // indicate packet size
    unsigned char *data;                        // if result is UC_OP_SUCC or UC_OP_PART_SUCC, data is not null, need free
    unsigned char fail_idx[PARTIAL_FAIL_NUM];   // packet number 1 ~ 255, if 0, means no packet fail, like 1,2,0,0... means only packet 1 and 2 is fail.
    unsigned int cur_rf_cnt;                    // algined by subframe struct
} uc_recv_back_t, *uc_recv_back_p;

typedef struct
{
    void *sem;
    unsigned char result;
    unsigned char type;
    unsigned short data_len;
    unsigned char *data;
} uc_recv_result_t, *uc_recv_result_p;

typedef struct
{
    unsigned char freq_idx;
} uc_freq_scan_req_t, *uc_freq_scan_req_p;

typedef struct
{
    unsigned char freq_idx;
    signed char rssi;
} uc_freq_scan_result_t, *uc_freq_scan_result_p;

typedef struct
{
    unsigned int uni_send_total;
    unsigned int uni_send_succ;
    unsigned int bc_send_total; // not use, is same as bc_send_succ
    unsigned int bc_send_succ;
    unsigned int uni_recv_fail;
    unsigned int uni_recv_succ;
    unsigned int bc_recv_fail;
    unsigned int bc_recv_succ;
} uc_stats_info_t, *uc_stats_info_p;

typedef struct
{
    unsigned int uni_send_succ_data_len;
    unsigned int bc_send_succ_data_len;
    unsigned int uni_recv_succ_data_len;
    unsigned int bc_recv_succ_data_len;
} uc_throughput_info_t, *uc_throughput_info_p;

typedef struct
{
    unsigned char freq;
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned short awaken_id; // indicate which id should send
    unsigned short reserved;
    unsigned int send_time; // ms, at least rx detect period
} uc_lpm_tx_cfg_t, *uc_lpm_tx_cfg_p;

typedef struct
{
    unsigned char freq;
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned char lpm_nlen; // 1,2,3,4, default 4
    unsigned char lpm_utimes; // 1,2,3, default 2
    unsigned char threshold; // detect threshold, 1~15, default 10
    unsigned char extra_flag; // defalut, if set 1, last period will use extra_period, then wake up
    unsigned short awaken_id; // indicate which id should detect
    unsigned short reserved;
    unsigned int detect_period; // ms, like 1000 ms
    unsigned int extra_period; // ms, extra new period before wake up
} uc_lpm_rx_cfg_t, *uc_lpm_rx_cfg_p;

typedef struct
{
    unsigned short data_len;
    unsigned char is_right; // for recv data
    unsigned char reserved;
    unsigned char *data;
} uc_subf_data_t, *uc_subf_data_p;

typedef void (*uc_recv)(uc_recv_back_p recv_data);
typedef void (*uc_send)(uc_send_back_p send_result);

extern unsigned char factory_msg_handler(signed int subtype, signed int value);

void uc_wiota_get_version(unsigned char *wiota_version, unsigned char *git_info, unsigned char *time, unsigned int *cce_version);

void uc_wiota_init(void);

void uc_wiota_run(void);

void uc_wiota_exit(void);

UC_WIOTA_STATUS uc_wiota_get_state(void);

void uc_wiota_set_dcxo(unsigned int dcxo);

void uc_wiota_set_freq_info(unsigned char freq_idx);

unsigned char uc_wiota_get_freq_info(void);

void uc_wiota_set_system_config(sub_system_config_t *config);

void uc_wiota_get_system_config(sub_system_config_t *config);

void uc_wiota_get_radio_info(radio_info_t *radio);

UC_OP_RESULT uc_wiota_send_data(unsigned int userId, unsigned char *data, unsigned short len, unsigned char *bcHead, unsigned char headLen, unsigned int timeout, uc_send callback);

// void uc_wiota_recv_data(uc_recv_back_p recv_result, unsigned short timeout, uc_recv callback);

void uc_wiota_register_recv_data_callback(uc_recv callback, UC_CALLBACK_DATA_TYPE type);

void uc_wiota_set_lpm_mode(unsigned char lpm_mode);

void uc_wiota_set_is_gating(unsigned char is_gating);

void uc_wiota_set_gating_event(unsigned char action, unsigned char event_id);

void uc_wiota_set_data_rate(unsigned char rate_mode, unsigned short rate_value);

void uc_wiota_set_cur_power(signed char power);

void uc_wiota_set_max_power(signed char power);

void uc_wiota_log_switch(unsigned char log_type, unsigned char is_open);

unsigned int uc_wiota_get_stats(unsigned char type);

void uc_wiota_get_all_stats(uc_stats_info_p stats_info_ptr);

void uc_wiota_reset_stats(unsigned char type);

void uc_wiota_set_crc(unsigned short crc_limit);

unsigned short uc_wiota_get_crc(void);

void uc_wiota_reset_throughput(unsigned char type);

void uc_wiota_get_throughput(uc_throughput_info_t *throughput_info);

void uc_wiota_light_func_enable(unsigned char func_enable);

void uc_wiota_set_subframe_num(unsigned char subframe_num);

unsigned char uc_wiota_get_subframe_num(void);

void uc_wiota_set_bc_round(unsigned char bc_round);

void uc_wiota_set_detect_time(unsigned int wait_cnt);

// void uc_wiota_set_bandwidth(unsigned char bandwidth);

unsigned int uc_wiota_get_frame_len(unsigned char type);

u32_t uc_wiota_get_subframe_len(void);

void uc_wiota_set_continue_send(unsigned char c_send_flag);

void uc_wiota_set_subframe_send(unsigned char s_send_flag);

void uc_wiota_set_subframe_recv(u8_t s_recv_flag);

void uc_wiota_set_subframe_head(u8_t head_data);


void uc_wiota_set_incomplete_recv(unsigned char recv_flag);

void uc_wiota_set_tx_mode(unsigned char mode);

void uc_wiota_set_freq_div(unsigned char div_mode);

void uc_wiota_set_vol_mode(unsigned char vol_mode);

void uc_wiota_set_alarm_time(unsigned int sec);

void uc_wiota_sleep_enter(unsigned char is_need_ex_wk, unsigned char is_need_32k_div);

void uc_wiota_set_recv_mode(UC_AUTO_RECV_MODE mode);

unsigned short uc_wiota_get_subframe_data_len(unsigned char mcs, unsigned char is_bc, unsigned char is_first);

void uc_wiota_set_adjust_mode(UC_ADJUST_MODE adjust_mode);

void uc_wiota_set_unisend_fail_cnt(unsigned char cnt);

void uc_wiota_scan_freq(unsigned char* data, unsigned short len, unsigned char scan_round, unsigned int timeout, uc_recv callback, uc_recv_back_p recv_result);

void uc_wiota_paging_rx_enter(unsigned char is_need_32k_div);

void uc_wiota_paging_tx_start(void);

void uc_wiota_set_paging_tx_cfg(uc_lpm_tx_cfg_t *config);

boolean uc_wiota_set_paging_rx_cfg(uc_lpm_rx_cfg_t *config);

void uc_wiota_get_paging_tx_cfg(uc_lpm_tx_cfg_t *config);

void uc_wiota_get_paging_rx_cfg(uc_lpm_rx_cfg_t *config);

u16_t uc_wiota_get_awaken_id_limit(unsigned char symbol_len);

boolean uc_wiota_add_subframe_data(uc_subf_data_p subf_data);

void uc_wiota_get_subframe_data(uc_subf_data_p subf_data);

unsigned int uc_wiota_get_subframe_data_num(unsigned char is_recv);

void uc_wiota_set_subframe_data_limit(unsigned int num_limit);

unsigned char uc_wiota_get_pyhsical_status(void); // UC_RF_STATUS_E

boolean uc_wiota_voice_data_acc(unsigned char is_first, unsigned int *data, u16_t data_len);

void uc_wiota_set_outer_32K(boolean is_open);

unsigned char uc_wiota_get_awakened_cause(unsigned char *is_cs_awakened); // UC_AWAKENED_CAUSE

unsigned int uc_wiota_get_curr_rf_cnt(void);

UC_OP_RESULT uc_wiota_send_data_with_start_time(u32_t userId, u8_t *data, u16_t len, u8_t *bcHead, u8_t headLen, u32_t timeout, uc_send callback, u32_t start_time);


// below is about uboot
void get_uboot_version(unsigned char *version);

void get_uboot_baud_rate(int *baudrate);

void set_uboot_baud_rate(int baud_rate);

void get_uboot_mode(unsigned char *mode);

void set_uboot_mode(unsigned char mode);

void set_partition_size(int bin_size, int reserverd_size, int ota_size);

void get_partition_size(int *bin_size, int *reserved_size, int *ota_size);

void boot_riscv_reboot(void);

void set_uboot_log(unsigned char uart_flag, unsigned char log_flag, unsigned char select_flag);

void get_uboot_log_set(unsigned char *uart_flag, unsigned char *log_flag, unsigned char *select_flag);

#endif
