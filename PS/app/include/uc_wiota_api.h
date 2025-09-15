
#ifndef _UC_WIOTA_API_H__
#define _UC_WIOTA_API_H__

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX_USER_ID_LEN 8

//200 k
// #define WIOTA_FREQUENCE_STEP 20

// unit is 10Hz, accuracy is also 10Hz
// like freq_idx is 100, base_freq is 47000000 (470M), freq_spacing is 20000 (200KHz), calc result is 49000000
#define WIOTA_FREQUENCE_POINT(freq_idx, base_freq, freq_spacing) (base_freq + freq_idx * freq_spacing)

#define WIOTA_FREQUENCE_INDEX(freq, base_freq, freq_spacing) ((freq - base_freq) / freq_spacing)

#define CRC16_LEN 2

#define PARTIAL_FAIL_NUM 8

#define UC_USER_HEAD_SIZE 2

#define UC_BC_LENGTH_LIMIT 1021

#define UC_DATA_LENGTH_LIMIT 310

#define UC_MAX_SUBFRAME_NUM 10

#define UC_MIN_SUBFRAME_NUM 3

#define UC_MULTI_SUBF_NUM 8

typedef enum
{
    UC_CALLBACK_NORAMAL_MSG = 0, // normal msg from ap
    UC_CALLBACK_STATE_INFO,      // state info
} uc_callback_data_type_e;

typedef enum
{
    UC_RECV_MSG = 0,          // UC_CALLBACK_NORAMAL_MSG, normal msg
    UC_RECV_BC,               // UC_CALLBACK_NORAMAL_MSG, broadcast msg
    UC_RECV_SCAN_FREQ,        // UC_CALLBACK_NORAMAL_MSG, result of freq scan by riscv
    UC_RECV_PHY_RESET,        // UC_CALLBACK_STATE_INFO, if phy reseted once, tell app
    UC_RECV_SYNC_STATE,       // UC_CALLBACK_STATE_INFO, when subf recv mode, if sync succ, report UC_OP_SUCC, if track fail, report UC_OP_FAIL
    UC_RECV_ACC_DATA,         // UC_CALLBACK_NORAMAL_MSG, acc voice data, need _VOICE_ACC_, not support now
    UC_RECV_SAVE_STATIC_DONE, // UC_CALLBACK_STATE_INFO, save static done when run
    UC_RECV_PG_TX_DONE,       // UC_CALLBACK_STATE_INFO, when pg tx end, tell app
    UC_RECV_SUBF_DATA,        // UC_CALLBACK_NORAMAL_MSG, subframe data report, need open report
    UC_RECV_MAX_TYPE,
} uc_recv_data_type_e;

typedef enum
{
    UC_STATUS_NULL = 0,
    UC_STATUS_RECV_TRY,
    UC_STATUS_RECVING,
    UC_STATUS_SENDING,
    UC_STATUS_SLEEP,
    UC_STATUS_ERROR,
} uc_wiota_status_e;

typedef enum
{
    UC_OP_SUCC = 0,
    UC_OP_TIMEOUT,
    UC_OP_FAIL,
    UC_OP_PART_SUCC,
    UC_OP_CRC_FAIL,
    UC_OP_QUEUE_FULL,
    UC_OP_MEM_ERR,
} uc_op_result_e;

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
} uc_mcs_level_e;

typedef enum
{
    UC_RATE_NORMAL = 0,
    UC_RATE_CRC_TYPE = 1,       // 0, normal mode, 1, only one byte crc
    UC_RATE_UNI_ACK_MODE = 2,   // uc_uni_ack_mode_e
    UC_RATE_FIRST_MCS_MODE = 3, // default 0, first bc mcs is 0, 1: calc access mcs
    UC_RATE_PRM_GAP_MODE = 4,   // uc_prm_gap_mode_e
    UC_RATE_OLD_UNI_MODE = 5,   // 1: old mode, default 0: new mode
    UC_RATE_OTHER,
} uc_data_rate_mode_e;

typedef enum
{
    UC_LOG_UART = 0,
    UC_LOG_SPI,
} uc_log_type_e;

typedef enum
{
    UC_STATS_READ = 0,
    UC_STATS_WRITE,
} uc_stats_mode_e;

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
} uc_stats_type_e;

typedef enum
{
    MODULE_STATE_NULL = 0,
    MODULE_STATE_INIT,
    MODULE_STATE_RUN,
    MODULE_STATE_SWITCHING,
    MODULE_STATE_INVALID
} uc_task_state_e;

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
} uc_freq_div_mode_e;

typedef enum
{
    VOL_MODE_CLOSE = 0, // 1.82v
    VOL_MODE_OPEN = 1,  // 1.47v
    VOL_MODE_TEMP_MAX,
} uc_vol_mode_e;

typedef enum
{
    UC_AUTO_RECV_START = 0, // default
    UC_AUTO_RECV_STOP = 1,
    UC_AUTO_RECV_ERROR,
} uc_auto_recv_mode_e;

typedef enum
{
    ADJUST_MODE_SEND = 0,
    ADJUST_MODE_RECV = 1,
} uc_adjust_mode_e;

typedef enum
{
    AWAKENED_CAUSE_HARD_RESET = 0, // also watchdog reset, spi cs reset
    AWAKENED_CAUSE_SLEEP = 1,
    AWAKENED_CAUSE_PAGING = 2,          // then get uc_lpm_paging_waken_cause_e
    AWAKENED_CAUSE_GATING = 3,          // no need care
    AWAKENED_CAUSE_FORCED_INTERNAL = 4, // not use
    AWAKENED_CAUSE_OTHERS,
} uc_awakened_cause_e;

typedef enum
{
    PAGING_WAKEN_CAUSE_NULL = 0,            // not from paging
    PAGING_WAKEN_CAUSE_PAGING_TIMEOUT = 1,  // from lpm timeout
    PAGING_WAKEN_CAUSE_PAGING_SIGNAL = 2,   // from lpm signal
    PAGING_WAKEN_CAUSE_SYNC_PG_TIMEOUT = 3, // from sync paging timeout, not use in async
    PAGING_WAKEN_CAUSE_SYNC_PG_SIGNAL = 4,  // from sync paging signal, not use in async
    PAGING_WAKEN_CAUSE_SYNC_PG_TIMING = 5,  // from sync paging timing set, not use in async
    PAGING_WAKEN_CAUSE_MAX,
} uc_lpm_paging_waken_cause_e;

typedef enum
{
    RF_STATUS_IDLE = 0,
    RF_STATUS_TX,
    RF_STATUS_RX,
    INVALD_RF_STATUS
} uc_rf_status_e;

typedef enum
{
    GATING_WK_NULL = 0,
    GATING_WK_INTERRUPT, // 1
    GATING_WK_PERIOD,    // 2
    GATING_WK_RF_SIGNAL, // 3
} uc_gating_wk_cause_e;

typedef enum
{
    FIRST_MCS_FIX0 = 0, // fix to 0
    FIRST_MCS_FLEX = 1, // calc by mcs set
    FIRST_MCS_UNVALID,
} uc_first_mcs_mode_e;

typedef enum
{
    UNI_ACK_NONE = 0,  // every uni frame has no ack
    UNI_ACK_EVERY = 1, // every uni frame has ack
    UNI_ACK_LAST = 2,  // only last frame has ack
    UNI_ACK_UNVALID,
} uc_uni_ack_mode_e;

typedef enum
{
    PRM_GAP_NONE = 0,
    PRM_GAP_HAS = 1,
    PRM_GAP_UNVALID,
} uc_prm_gap_mode_e;

typedef enum
{
    BAND_WIDTH_400KHZ = 0, // not support
    BAND_WIDTH_200KHZ,     // 1
    BAND_WIDTH_100KHZ,     // 2
    BAND_WIDTH_50KHZ,      // 3
    BAND_WIDTH_25KHZ,      // 4
    BAND_WIDTH_12P5KHZ,    // 5
    BAND_WIDTH_6P25KHZ,    // 6, not support
    BAND_WIDTH_MAX,        // 7
} uc_band_width_e;

typedef enum
{
    FREQ_SPACING_400KHZ = 0, // if set 0, use bandwidth
    FREQ_SPACING_200KHZ,     // 1
    FREQ_SPACING_100KHZ,     // 2
    FREQ_SPACING_50KHZ,      // 3
    FREQ_SPACING_25KHZ,      // 4
    FREQ_SPACING_12P5KHZ,    // 5
    FREQ_SPACING_6P25KHZ,    // 6
    FREQ_SPACING_5KHZ,       // 7
    FREQ_SPACING_500HZ,      // 8
    FREQ_SPACING_2P5HZ,      // 9
    FREQ_SPACING_100HZ,      // 10
    FREQ_SPACING_MAX,        // 11
} uc_freq_spacing_e;

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
    unsigned char freqSpacing;   // default 0, use bandwidth, else use uc_freq_spacing_e
    unsigned char id_len;        // 0: 2, 1: 4, 2: 6, 3: 8
    unsigned char pp;            // 0: 1, 1: 2, 2: 4, 3: not use
    unsigned char symbol_length; // 0: 128, 1: 256, 2: 512, 3: 1024
    unsigned char pz;            // default 8
    unsigned char btvalue;       // bt from rf 1: 0.3, 0: 1.2
    unsigned char bandwidth;     // default 1, 200KHz, uc_band_width_e
    unsigned char spectrum_idx;  // default 3, 470M~510M;
                                 // unsigned int systemId; // no use !
    unsigned short freq_idx;     // freq idx
    unsigned char reserved1[2];
    unsigned int subsystemid;
    unsigned short freq_list[16];
    unsigned char na[16];
} sub_system_config_t;

typedef struct
{
    unsigned char result;
    unsigned char ack_info;
    unsigned short reserved;
    unsigned char *oriPtr;
} uc_send_back_t, *uc_send_back_p;

typedef struct
{
    void *sem;
    unsigned char result;
    unsigned char ack_info;
    unsigned short reserved;
} uc_send_result_t, *uc_send_result_p;

typedef struct
{
    unsigned char result; // uc_op_result_e
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
    unsigned short freq_idx;
} uc_freq_scan_req_t, *uc_freq_scan_req_p;

typedef struct
{
    unsigned short freq_idx;
    signed char rssi;
    unsigned char reserved;
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
    unsigned char mode; // 0: old id range(narrow), 1: extend id range(wide)
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned short awaken_id; // indicate which id should send
    unsigned short freq;
    unsigned int send_time; // ms, at least rx detect period
} uc_lpm_tx_cfg_t, *uc_lpm_tx_cfg_p;

typedef struct
{
    unsigned char mode; // 0: old id range(narrow), 1: extend id range(wide)
    unsigned char spectrum_idx;
    unsigned char bandwidth;
    unsigned char symbol_length;
    unsigned char lpm_nlen;   // 1,2,3,4, default 4
    unsigned char lpm_utimes; // 1,2,3, default 2
    unsigned char threshold;  // detect threshold, 1~15, default 10
    unsigned char extra_flag; // defalut, if set 1, last period will use extra_period, then wake up
    unsigned short awaken_id; // indicate which id should detect
    unsigned short freq;
    unsigned int detect_period; // ms, like 1000 ms
    unsigned int extra_period;  // ms, extra new period before wake up
    unsigned char reserved1;
    unsigned char period_multiple;    // the multiples of detect_period using awaken_id_ano, if 0, no need
    unsigned short awaken_id_another; // another awaken_id
} uc_lpm_rx_cfg_t, *uc_lpm_rx_cfg_p;

typedef struct
{
    unsigned short data_len;
    unsigned char is_right; // for recv data
    unsigned char rssi;
    unsigned char *data;
} uc_subf_data_t, *uc_subf_data_p;

typedef struct
{
    unsigned char is_ack_info;  // if 1, use ack 1bit to trans info, need set same value both sides
    unsigned char is_same_info; // if 1, all recv id use same ack info, if 0, use ack_list
    unsigned char same_info;    // 0 or 1, if use this, below are not used
    unsigned char list_num;     // list num of id_list and ack_list
    unsigned short start_byte;  // protocol will get id from app data, need to know which byte to get from
    unsigned char id_len;       // id len in app data
    unsigned char reserved;     // 4 bytes align
    unsigned int *id_list;      // ptr malloc/free by app, if null, set same_info, defaul int, may not use full 4bytes
    unsigned char *ack_list;    // ptr malloc/free by app, if null, set same_info
} uc_uni_ack_info_t, *uc_uni_ack_info_p;

typedef struct
{
    unsigned char is_close; // default 0, can set 1, then not use adj
    unsigned char is_valid; // if got valid data from factory test
    unsigned char adc_trm;  // adc config
    unsigned char reserved; // re
    unsigned short adc_ka;  // adc calc
    short adc_mida;         // adc calc
    unsigned short adc_kb;  // adc calc
    short adc_midb;         // adc calc
} uc_adc_adj_t, *uc_adc_adj_p;

typedef void (*uc_recv)(uc_recv_back_p recv_data);
typedef void (*uc_send)(uc_send_back_p send_result);

extern unsigned char factory_msg_handler(signed int subtype, signed int value);

void uc_wiota_get_version(unsigned char *wiota_version, unsigned char *git_info,
                          unsigned char *time, unsigned int *cce_version);

unsigned char uc_wiota_execute_state(void);

void uc_wiota_init(void);

void uc_wiota_run(void);

void uc_wiota_exit(void);

uc_wiota_status_e uc_wiota_get_state(void);

unsigned char uc_wiota_set_dcxo(unsigned int dcxo);

unsigned int uc_wiota_get_dcxo(void);

void uc_wiota_set_dcxo_by_temp_curve(void);

unsigned char uc_wiota_set_freq_info(unsigned short freq_idx);

unsigned short uc_wiota_get_freq_info(void);

unsigned char uc_wiota_set_subsystem_id(unsigned int subsystemid);

unsigned int uc_wiota_get_subsystem_id(void);

unsigned char uc_wiota_set_system_config(sub_system_config_t *config);

void uc_wiota_get_system_config(sub_system_config_t *config);

void uc_wiota_get_radio_info(radio_info_t *radio);

uc_op_result_e uc_wiota_send_data(unsigned int userId, unsigned char *data, unsigned short len,
                                  unsigned char *bcHead, unsigned char headLen, unsigned int timeout, uc_send callback);

void uc_wiota_recv_data(uc_recv_back_p recv_result, unsigned short timeout, unsigned int userId, uc_recv callback);

void uc_wiota_register_recv_data_callback(uc_recv callback, uc_callback_data_type_e type);

void uc_wiota_set_lpm_mode(unsigned char lpm_mode);

#ifdef _CLK_GATING_
// not support gating func
void uc_wiota_set_is_gating(unsigned char is_gating, unsigned char is_subrecv);
void uc_wiota_set_gating_config(unsigned int timeout, unsigned int period); // second, micro second
void uc_wiota_set_extra_rf_before_send(unsigned int duration);              // ms
unsigned char uc_wiota_get_is_gating(void);
void uc_wiota_set_gating_event(unsigned char action, unsigned char event_id);
#endif // _CLK_GATING_

unsigned char uc_wiota_set_data_rate(unsigned char rate_mode, unsigned short rate_value);

unsigned char uc_wiota_set_cur_power(signed char power);

void uc_wiota_set_max_power(signed char power);

void uc_wiota_log_switch(unsigned char log_type, unsigned char is_open);

unsigned int uc_wiota_get_stats(unsigned char type);

void uc_wiota_get_all_stats(uc_stats_info_p stats_info_ptr);

void uc_wiota_reset_stats(unsigned char type);

void uc_wiota_set_crc(unsigned short crc_limit);

unsigned short uc_wiota_get_crc(void);

#ifdef _LIGHT_PILOT_
void uc_wiota_light_func_enable(unsigned char func_enable);
#endif

#ifdef _SUBFRAME_MODE_
unsigned char uc_wiota_set_two_way_subframe_num(unsigned char subf_num_even, unsigned char subf_num_odd);
#endif

unsigned char uc_wiota_set_subframe_num(unsigned char subframe_num);

unsigned char uc_wiota_get_subframe_num(void);

unsigned char uc_wiota_set_bc_round(unsigned char bc_round);

void uc_wiota_set_detect_time(unsigned int wait_cnt);

// void uc_wiota_set_bandwidth(unsigned char bandwidth);

unsigned int uc_wiota_get_frame_len(unsigned char type);

unsigned int uc_wiota_get_frame_len_us(unsigned char type);

unsigned int uc_wiota_get_frame_len_with_params(unsigned char symbol_len, unsigned char symbol_mode,
                                                unsigned char subf_num, unsigned char band_width, unsigned char suf_mode);

unsigned int uc_wiota_get_subframe_len(void);

void uc_wiota_set_continue_send(unsigned char c_send_flag);

#ifdef _LPM_PAGING_
void uc_wiota_set_embed_pgtx(unsigned char is_embed_pgtx);
#endif

#ifdef _SUBFRAME_MODE_
void uc_wiota_set_subframe_send(unsigned char s_send_flag);
unsigned char uc_wiota_get_subframe_send(void);
void uc_wiota_set_subframe_recv(unsigned char s_recv_flag);
unsigned char uc_wiota_get_subframe_recv(void);
void uc_wiota_set_subrecv_fail_limit(unsigned char fail_limit);
void uc_wiota_set_two_way_mode(unsigned char mode, unsigned char is_open);
void uc_wiota_get_two_way_mode(unsigned char *is_send, unsigned char *is_recv);
// void uc_wiota_set_subframe_head(unsigned char head_data);
void uc_wiota_set_two_mode_recv(unsigned char tm_recv_flag);
unsigned char uc_wiota_get_two_mode_recv(void);
#endif // _SUBFRAME_MODE_

void uc_wiota_set_lna_pa_gpio(unsigned char lna_gpio, unsigned char pa_gpio,
                              unsigned char lna_trigger, unsigned char pa_trigger);

void uc_wiota_set_lna_pa_switch(unsigned char is_open); // open control

void uc_wiota_set_incomplete_recv(unsigned char recv_flag);

unsigned char uc_wiota_set_tx_mode(unsigned char mode);

unsigned char uc_wiota_get_tx_mode(void);

void uc_wiota_set_freq_div(unsigned char div_mode);

void uc_wiota_set_vol_mode(unsigned char vol_mode);

void uc_wiota_set_alarm_time(unsigned int sec);

void uc_wiota_set_is_ex_wk(unsigned char is_need_ex_wk);

void uc_wiota_sleep_enter(unsigned char is_need_ex_wk, unsigned char is_need_32k_div);

unsigned char uc_wiota_set_recv_mode(uc_auto_recv_mode_e mode);

unsigned short uc_wiota_get_subframe_data_len(unsigned char mcs, unsigned char is_bc,
                                              unsigned char is_first_sub, unsigned char is_first_fn);

void uc_wiota_set_unisend_fail_cnt(unsigned char cnt);

void uc_wiota_scan_freq(unsigned char *data, unsigned short len, unsigned char scan_round, unsigned int timeout,
                        uc_recv callback, uc_recv_back_p recv_result);

#ifdef _LPM_PAGING_
void uc_wiota_paging_rx_enter(unsigned char is_need_32k_div, unsigned int timeout_max);
unsigned char uc_wiota_paging_tx_start(void);
unsigned char uc_wiota_set_paging_tx_cfg(uc_lpm_tx_cfg_t *config);
unsigned char uc_wiota_set_paging_rx_cfg(uc_lpm_rx_cfg_t *config);
void uc_wiota_get_paging_tx_cfg(uc_lpm_tx_cfg_t *config);
void uc_wiota_get_paging_rx_cfg(uc_lpm_rx_cfg_t *config);
unsigned short uc_wiota_get_awaken_id_limit(unsigned char symbol_len);
#endif // _LPM_PAGING_

#ifdef _SUBFRAME_MODE_
unsigned char uc_wiota_add_subframe_data(uc_subf_data_p subf_data);
void uc_wiota_get_subframe_data(uc_subf_data_p subf_data);
unsigned int uc_wiota_get_subframe_data_num(unsigned char is_recv);
void uc_wiota_set_subframe_data_limit(unsigned int num_limit);
unsigned char uc_wiota_set_is_report_subf_data(unsigned char mode);
#endif

unsigned char uc_wiota_get_physical_status(void); // uc_rf_status_e

#ifdef _VOICE_ACC_
unsigned char uc_wiota_voice_data_acc(unsigned char is_first, unsigned int *data, unsigned short data_len);
#endif

void uc_wiota_set_outer_32K(unsigned char is_open);

#ifdef _LPM_PAGING_
unsigned char uc_wiota_get_awakened_cause(unsigned char *is_cs_awakened);                                // uc_awakened_cause_e
unsigned char uc_wiota_get_paging_awaken_cause(unsigned int *detected_times, unsigned char *detect_idx); // uc_lpm_paging_waken_cause_e
#endif

unsigned int uc_wiota_get_curr_rf_cnt(void);

uc_op_result_e uc_wiota_send_data_with_start_time(unsigned int userId, unsigned char *data, unsigned short len, unsigned char *bcHead, unsigned char headLen, unsigned int timeout, uc_send callback, unsigned int start_time);

void uc_wiota_set_symbol_mode(unsigned char symbol_mode);

void uc_wiota_set_rf_ctrl_type(unsigned char rf_ctrl_type);

void uc_wiota_set_auto_ack_pow(unsigned char is_open);

unsigned char uc_wiota_get_auto_ack_pow(void);

unsigned char uc_wiota_suspend(void);

unsigned char uc_wiota_recover(void);

#ifdef _CLK_GATING_
void uc_wiota_check_gating_with_interrupt(void);
unsigned char uc_wiota_get_gating_wk_cause(void);
#endif

#ifdef _L1_FACTORY_FUNC_
unsigned char uc_wiota_factory_task_init(void);
void uc_wiota_factory_fill_data(unsigned char *data, unsigned short data_len);
void uc_wiota_factory_register_data_callback(uc_recv callback);
void uc_wiota_factory_task_exit(void);
#endif

unsigned char uc_wiota_calc_suitable_subframe_num(unsigned char type, unsigned short data_len);

void uc_wiota_get_uni_ack_info(uc_uni_ack_info_p uni_ack_info_ptr);

void uc_wiota_set_uni_ack_info(uc_uni_ack_info_p uni_ack_info_ptr);

unsigned char uc_wiota_get_module_id(unsigned char *module_id);

unsigned char uc_wiota_set_data_limit(unsigned char mode, unsigned short limit);

unsigned short uc_wiota_get_data_limit(unsigned char mode);

uc_uni_ack_mode_e uc_wiota_get_uni_ack_mode(void);

void uc_wiota_set_recv_seq_mult_mode(unsigned char is_seq_mult);

void uc_wiota_reset_rf(void);

void uc_wiota_get_adc_adj_info(uc_adc_adj_p adc_adj);

void uc_wiota_set_adc_adj_close(unsigned char is_close); // 1 means close

unsigned char uc_wiota_set_search_list(unsigned char bandwidth, unsigned int value);

void uc_wiota_set_scan_sorted(unsigned char is_sort);

void uc_wiota_set_scan_max(unsigned char is_max);

unsigned char uc_wiota_flash_id_is_puya(void);

void uc_wiota_set_new_ldo(unsigned char new_ldo);

void uc_wiota_set_gating_print(unsigned char is_open);

void uc_wiota_set_en_aagc_ajust(unsigned char en_aagc_ajust);

void uc_wiota_set_init_agc(unsigned char agc_idx);

unsigned char uc_wiota_mem_addr_value(unsigned int mem_addr, unsigned int value);


// below is about uboot
void get_uboot_version(unsigned char *version);

void get_uboot_baud_rate(int *baudrate);

void set_uboot_baud_rate(int baud_rate);

void get_uboot_mode(unsigned char *mode);

void set_uboot_mode(unsigned char mode);

void set_uboot_wait_sec(unsigned char wait_sec);

unsigned char get_uboot_wait_sec();

void set_partition_size(int bin_size, int reserverd_size, int ota_size);

void get_partition_size(int *bin_size, int *reserved_size, int *ota_size);

void boot_riscv_reboot(void);

void set_uboot_log(unsigned char uart_flag, unsigned char log_flag, unsigned char select_flag);

void get_uboot_log_set(unsigned char *uart_flag, unsigned char *log_flag, unsigned char *select_flag);

#endif
