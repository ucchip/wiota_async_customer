#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>
#include "uc_uart.h"
#include "uc_gpio.h"
#include "ati_prs.h"
#include "fastlz.h"
#include "subfrmae_app.h"
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"

typedef enum
{
    SYMBOL_LENGTH_128 = 0,
    SYMBOL_LENGTH_256,
    SYMBOL_LENGTH_512,
    SYMBOL_LENGTH_1024,
} symbol_length_e;

typedef enum
{
    DEEP_SLEEP_MODE = 0, // 深度睡眠
    ALARM_WAKEUP_MODE,   // 定时唤醒
    RF_WAKEUP_MODE,      // 射频唤醒
    SLEEP_MODE_MAX,
} sleep_mode_e;

typedef enum
{
    RATE_MODE_1 = 1, // 128+mcs3, 12.04kbps
    RATE_MODE_2,     // 128+mcs2, 9.49kbps
    RATE_MODE_3,     // 256+mcs3, 4.79kbps
    RATE_MODE_4,     // 256+mcs2, 2.03kbps
    RATE_MODE_5,     // 256+mcs1, 1.38kbps
    RATE_MODE_6,     // 256+mcs0, 0.64kbps
    RATE_MODE_7,     // 512+mcs0, 0.32kbps
    RATE_MODE_8,     // 1024+mcs0, 0.16kbps
    RATE_MODE_MAX,
} rate_mode_e;

typedef struct
{
    unsigned char symbol_length;
    unsigned char mcs;
} rate_mode_t;

typedef struct
{
    unsigned int addr;                            // 终端地址(user id)
    unsigned int subsystemid;                     // subsyetem id
    unsigned int base_freq;                       // 频谱基础频率
    unsigned int cur_freq;                        // 当前频率
    unsigned char bw_level;                       // 带宽等级
    unsigned char spectrum_idx;                   // 频谱索引
    unsigned char power;                          // 功率
    unsigned char rate_mode;                      // rate_mode_e
    rate_mode_t rate_mode_map[RATE_MODE_MAX - 1]; // 速率模式映射表
} at_simplified_t;

#define AT_SEND_TIMEOUT 60000 // ms
#define MIN_SLEEP_TIME 100    // ms
#define MAX_SLEEP_TIME 44000  // ms
#define SERIAL_NUM_LEN 20
#define SOFTWARE_VER_LEN 15

#define DEFAULT_SUBSYSTMEID 0x21456981
#define DEFAULE_FREQ 502000000        // 默认频率470MHz
#define DEFAULE_BW 100.0f             // 默认带宽100KHz
#define DEFAULE_POWER 20              // 默认功率20dbm
#define DEFAULE_RATE_MODE RATE_MODE_4 // 256+mcs2, 2.03kbps

#define RUN_STATE_PIN GPIO_PIN_15
#define TX_STATE_PIN GPIO_PIN_17
#define RX_STATE_PIN GPIO_PIN_16

#define RATE_MODE_MAP                            \
    {                                            \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_128,  \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .mcs = UC_MCS_LEVEL_3,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .mcs = UC_MCS_LEVEL_2,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .mcs = UC_MCS_LEVEL_1,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_256,  \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_512,  \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
        {                                        \
            .symbol_length = SYMBOL_LENGTH_1024, \
            .mcs = UC_MCS_LEVEL_0,               \
        },                                       \
    };

#define BASE_FREQ 223000000
const unsigned int FREQ_RANGES[8][2] = {
    {BASE_FREQ, 235000000}, // 223MHz~235MHz，sepctrum_idx 0
    {400000000, 480000000}, // 400MHz~480MHz，sepctrum_idx 1
    {443050000, 434790000}, // 433.05MHz~434.79MHz，sepctrum_idx 2
    {470000000, 510000000}, // 470MHz~510MHz，sepctrum_idx 3
    {779000000, 787000000}, // 779MHz~787MHz，sepctrum_idx 4
    {840000000, 845000000}, // 840MHz~845MHz，sepctrum_idx 5
    {863000000, 870000000}, // 863MHz~870MHz，sepctrum_idx 6
    {902000000, 928000000}, // 902MHz~928MHz，sepctrum_idx 7
};

extern at_server_t at_get_server(void);
static at_simplified_t at_simplified_local = {0};
static const unsigned char g_state_pins[] = {RUN_STATE_PIN, TX_STATE_PIN, RX_STATE_PIN};
#define STATE_PINS_COUNT (sizeof(g_state_pins) / sizeof(g_state_pins[0]))

static void at_state_pin_init(void)
{
    for (int i = 0; i < STATE_PINS_COUNT; i++)
    {
        gpio_set_pin_mux(UC_GPIO_CFG, g_state_pins[i], GPIO_FUNC_0);
        rt_pin_mode(g_state_pins[i], PIN_MODE_OUTPUT);
        rt_pin_write(g_state_pins[i], PIN_LOW);
    }
}

void at_simplified_running(void)
{
    rt_pin_write(RUN_STATE_PIN, PIN_HIGH);
    rt_thread_mdelay(1000);
    rt_pin_write(RUN_STATE_PIN, PIN_LOW);
    rt_thread_mdelay(1000);
}

static inline void at_tx_start(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_HIGH);
}

static inline void at_tx_end(void)
{
    rt_pin_write(TX_STATE_PIN, PIN_LOW);
}

static inline void at_rx_notity(void)
{
    rt_pin_write(RX_STATE_PIN, PIN_HIGH);
}

static inline void at_rx_complete(void)
{
    rt_thread_mdelay(1);
    rt_pin_write(RX_STATE_PIN, PIN_LOW);
}

float at_strtof(const char *nptr, char **endptr)
{
    const char *p = nptr;
    float result = 0.0f;
    int sign = 1;
    float decimal = 0.1f;

    // 直接处理正负号（移除空白符）
    if (*p == '+' || *p == '-')
    {
        sign = (*p == '-') ? -1 : 1;
        p++;
    }
    // 解析整数部分
    while (*p >= '0' && *p <= '9')
    {
        result = result * 10.0f + (*p - '0');
        p++;
    }
    // 解析小数部分
    if (*p == '.')
    {
        p++;
        while (*p >= '0' && *p <= '9')
        {
            result += (*p - '0') * decimal;
            decimal *= 0.1f;
            p++;
        }
    }
    // 设置endptr
    if (endptr != RT_NULL)
        *endptr = (char *)p;
    // 返回结果
    return result * sign;
}

static unsigned char at_get_bw_level(float bw)
{
    // if (bw == 6.25f)
    //     return 6;
    // else if (bw == 12.5f)
    //     return 5;
    // else
    if (bw == 25.0f)
        return 4;
    else if (bw == 50.0f)
        return 3;
    else if (bw == 100.0f)
        return 2;
    else if (bw == 200.0f)
        return 1;
    else
        return 0xFF;
}

static float at_get_bw_value(void)
{
    unsigned char level = at_simplified_local.bw_level;

    // if (level == 6)
    //     return 6.25f;
    // else if (level == 5)
    //     return 12.5f;
    // else
    if (level == 4)
        return 25.0f;
    else if (level == 3)
        return 50.0f;
    else if (level == 2)
        return 100.0f;
    else if (level == 1)
        return 200.0f;
    else
        return 0;
}

static char *at_get_bw_str(void)
{
    unsigned char level = at_simplified_local.bw_level;

    // if (level == 6)
    //     return "6.25";
    // else if (level == 5)
    //     return "12.5";
    // else
    if (level == 4)
        return "25";
    else if (level == 3)
        return "50";
    else if (level == 2)
        return "100 ";
    else if (level == 1)
        return "200";

    return "null";
}

static unsigned int at_get_base_freq_and_spectrum_idx(unsigned int freq, unsigned char *spectrum_idx, unsigned int *actual_freq)
{
    // traverse in reverse order t ensure that 470MHz~480MHz is preferentially matched with spectrum 3.
    for (int i = sizeof(FREQ_RANGES) / sizeof(FREQ_RANGES[0]) - 1; i >= 0; i--)
    {
        if (freq >= FREQ_RANGES[i][0] && freq <= FREQ_RANGES[i][1])
        {
            float bw = at_get_bw_value();
            unsigned int base_freq = FREQ_RANGES[i][0];
            int diff_freq_hz = (int)(freq - base_freq);
            int bw_hz = (int)(bw * 1000); // kHz -> Hz

            if (diff_freq_hz % bw_hz != 0)
            {
                // 不为带宽的整数倍，向下取整修正
                *actual_freq = base_freq + (diff_freq_hz / bw_hz) * bw_hz;
            }
            else
            {
                *actual_freq = freq;
            }
            *spectrum_idx = i;
            return base_freq; // 返回base freq
        }
    }

    return 1; // 不在范围
}

static unsigned short at_get_max_awaken_id(void)
{
    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode];
    unsigned char symbol_length = rate_mode->symbol_length;
    unsigned short id_max_array[SYMBOL_LENGTH_1024 + 1] = {1023, 4095, 16383, 65535};

    return id_max_array[symbol_length];
}

static unsigned int at_get_address(void)
{
    unsigned int addr = 0;
    unsigned char len;

    uc_wiota_get_userid(&addr, &len);
    return addr;
}

static int at_set_address(unsigned int addr)
{
    if (at_simplified_local.addr != addr)
    {
        if (uc_wiota_set_userid(&addr, 4) == 0)
        {
            at_simplified_local.addr = addr;
            return 0;
        }
        return 1;
    }
    return 0;
}

static int at_set_subsyetemid(unsigned int subsyetemid)
{
    if (at_simplified_local.subsystemid != subsyetemid)
    {
        sub_system_config_t config = {0};
        uc_wiota_get_system_config(&config);
        config.subsystemid = subsyetemid;
        if (uc_wiota_set_system_config(&config))
        {
            at_simplified_local.subsystemid = subsyetemid;
            return 0;
        }
        return 1;
    }
    return 0;
}

static int at_set_power(unsigned char power)
{
    if (at_simplified_local.power != power)
    {
        if (uc_wiota_set_cur_power(power))
        {
            at_simplified_local.power = power;
            return 0;
        }
        return 1;
    }
    return 0;
}

void at_wiota_recv_callback(uc_recv_back_p data)
{
    if (data->data != RT_NULL && (UC_OP_SUCC == data->result ||
                                  UC_OP_PART_SUCC == data->result ||
                                  UC_OP_CRC_FAIL == data->result))
    {
        int data_len = data->data_len;
        unsigned char *recv_data = data->data;
        unsigned int addr = 0;
        int max_len = 1024;
        unsigned char *decompress_data = RT_NULL;

        if (data->type <= UC_RECV_BC)
        {
            if (uc_wiota_get_data_compress_of_recv())
            {
                // 解压数据
                decompress_data = (unsigned char *)rt_malloc(max_len);
                if (decompress_data == RT_NULL)
                {
                    rt_kprintf("decompress data malloc failed\n");
                    rt_free(data->data);
                    return;
                }
                rt_memset(decompress_data, 0, max_len);

                int decompress_len = fastlz_decompress(recv_data, data_len, decompress_data, max_len);
                if (decompress_len > 0)
                {
                    recv_data = decompress_data;
                    data_len = decompress_len;
                }
                else
                {
                    rt_kprintf("decompress data failed\n");
                    rt_free(data->data);
                    rt_free(decompress_data);
                    return;
                }
            }

            // 前四字节为对端地址
            addr = recv_data[0] | (recv_data[1] << 8) | (recv_data[2] << 16) | (recv_data[3] << 24);
            recv_data += 4;
            data_len -= 4;

            at_rx_notity();
            at_server_printf("+RECV:%u,%d,-%d,%d,", addr, data->type, data->rssi, data_len);
            at_send_data(recv_data, data_len);
            at_server_printfln("");
            at_rx_complete();
            rt_kprintf("head data %d %d dfe %u\n", data->head_data[0], data->head_data[1], data->cur_rf_cnt);
        }

        if (decompress_data != RT_NULL)
        {
            rt_free(decompress_data);
        }
        rt_free(data->data);
        // if (data->type == UC_RECV_PG_TX_DONE)
        // {
        //     at_server_printfln("+PGTX:DONE");
        // }
    }
}

static void at_wiota_run(void)
{
    // 退出wiota协议栈，且不保存静态数据
    uc_wiota_set_is_exit_static(0);
    uc_wiota_exit();

    // 启动wiota协议栈
    uc_wiota_init();

    rate_mode_t *rate_mode = &at_simplified_local.rate_mode_map[at_simplified_local.rate_mode - 1]; // -1，模式从1开始，下标从0开始
    sub_system_config_t config = {0};

    uc_wiota_get_system_config(&config);
    config.symbol_length = rate_mode->symbol_length;
    config.spectrum_idx = at_simplified_local.spectrum_idx;
    config.freq_idx = WIOTA_FREQUENCE_INDEX(at_simplified_local.cur_freq, at_simplified_local.base_freq, (int)(at_get_bw_value() * 1000));
    config.bandwidth = at_simplified_local.bw_level;
    config.subsystemid = at_simplified_local.subsystemid;
    rt_kprintf("sym_len %d, spec_idx %d, freq_idx %d, bw_l %d, subsystemid\n",
               config.symbol_length, config.spectrum_idx, config.freq_idx, config.bandwidth, config.subsystemid);
    uc_wiota_set_system_config(&config);
    uc_wiota_set_userid(&at_simplified_local.addr, 4);

    uc_wiota_run();
    uc_wiota_set_cur_power(at_simplified_local.power);
    uc_wiota_set_data_rate(0, rate_mode->mcs); // mcs
    uc_wiota_set_data_rate(4, 0);              // 无gap模式
    uc_wiota_set_data_rate(5, 0);              // 新单播模式
    uc_wiota_set_crc(1);                       // crc校验常开
    uc_wiota_set_unisend_fail_cnt(3);          // send 3 cnt
    rt_kprintf("addr %d(0x%x), pwr %d, mcs %d\n", at_simplified_local.addr, at_simplified_local.addr, at_simplified_local.power, rate_mode->mcs);
    uc_wiota_register_recv_data_callback(at_wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    uc_wiota_register_recv_data_callback(at_wiota_recv_callback, UC_CALLBACK_STATE_INFO);
    rt_kprintf("at_wiota_run ok\n");
}

void at_simplified_init(void)
{
    at_state_pin_init();

    // 初始化默认配置
    // -1，模式从1开始，下标从0开始
    const rate_mode_t rate_mode_map[RATE_MODE_MAX - 1] = RATE_MODE_MAP;

    at_simplified_local.addr = at_get_address();
    at_simplified_local.subsystemid = DEFAULT_SUBSYSTMEID;
    // default_bw 100KHz, bw_level 1
    at_simplified_local.bw_level = at_get_bw_level(DEFAULE_BW);
    // cur_freq 5020000, base_freq 470000000, spectrum_idx 3
    at_simplified_local.base_freq = at_get_base_freq_and_spectrum_idx(DEFAULE_FREQ, &at_simplified_local.spectrum_idx, &at_simplified_local.cur_freq);
    RT_ASSERT(at_simplified_local.base_freq >= BASE_FREQ);
    at_simplified_local.power = DEFAULE_POWER;
    // rate_mode 4, symbol_length 256, mcs 2
    at_simplified_local.rate_mode = DEFAULE_RATE_MODE;
    rt_memcpy(at_simplified_local.rate_mode_map, rate_mode_map, sizeof(rate_mode_map));
    rt_kprintf("at_simplified_init, addr %u(0x%x), freq %uHz-%uHz, bw %d-%uKHz, power %ddBm, rate mode %d(sym_len 1, mcs 2)\n",
               at_simplified_local.addr, at_simplified_local.addr,
               at_simplified_local.base_freq, at_simplified_local.cur_freq,
               at_simplified_local.bw_level, (unsigned int)at_get_bw_value(),
               at_simplified_local.power, at_simplified_local.rate_mode);

    // 启动wiota协议栈
    at_wiota_run();
}

static at_result_t at_wiota_sn_query(void)
{
    unsigned char serial[SERIAL_NUM_LEN] = {0};

    uc_wiota_get_module_id(serial);
    at_server_printfln("+SN:%s", serial);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_version_query(void)
{
    unsigned char version[SOFTWARE_VER_LEN] = {0};

    uc_wiota_get_version(version, RT_NULL, RT_NULL, RT_NULL);
    at_server_printfln("+VER:%s", version);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_rssi_query(void)
{
    sub_system_config_t config = {0};
    uc_recv_back_t scan_result = {0};

    uc_wiota_get_system_config(&config);
    uc_wiota_scan_freq((unsigned char *)&config.freq_idx, (unsigned short)(1 * sizeof(uc_freq_scan_req_t)), 10, 5000, RT_NULL, &scan_result);
    if (UC_OP_SUCC == scan_result.result && scan_result.data != RT_NULL)
    {
        uc_freq_scan_result_t *freq_scan_result = (uc_freq_scan_result_t *)scan_result.data;
        at_server_printfln("+RSSI:%d", freq_scan_result->rssi);
        rt_free(scan_result.data);
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_test(void)
{
    at_server_printfln("AT+SYNC=<sync[1-4294967295]>");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_query(void)
{
    at_server_printfln("+SYNC:%u", at_simplified_local.subsystemid);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sync_setup(const char *args)
{
    unsigned int subsystemid = 0;

    args = parse((char *)(++args), "d", &subsystemid);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == at_set_subsyetemid(subsystemid))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_address_test(void)
{
    at_server_printfln("AT+ADDR=<addr[1-4294967295]>");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_address_query(void)
{
    at_server_printfln("+ADDR:%u", at_get_address());
    return AT_RESULT_OK;
}

static at_result_t at_wiota_address_setup(const char *args)
{
    unsigned int addr = 0;

    args = parse((char *)(++args), "d", &addr);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == at_set_address(addr))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_power_test(void)
{
    at_server_printfln("AT+POWER=<power[0-22]>");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_query(void)
{
    at_server_printfln("+POWER:%d", at_simplified_local.power);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_power_setup(const char *args)
{
    int power = 0;

    args = parse((char *)(++args), "d", &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (power >= 0 && power <= 22)
    {
        if (0 == at_set_power(power))
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_freq_test(void)
{
    for (int i = 0; i < sizeof(FREQ_RANGES) / sizeof(FREQ_RANGES[0]); i++)
    {
        at_server_printfln("AT+FREQ=<freq[%u-%u]>", FREQ_RANGES[i][0], FREQ_RANGES[i][1]);
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_query(void)
{
    at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_freq_setup(const char *args)
{
    unsigned int freq = 0;

    args = parse((char *)(++args), "d", &freq);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    unsigned char spectrum_idx = 0xff;
    unsigned int actual_freq = 0;
    unsigned int base_freq = at_get_base_freq_and_spectrum_idx(freq, &spectrum_idx, &actual_freq);
    if (base_freq >= BASE_FREQ)
    {
        if (at_simplified_local.base_freq != base_freq ||
            at_simplified_local.spectrum_idx != spectrum_idx ||
            at_simplified_local.cur_freq != actual_freq)
        {
            if (at_simplified_local.base_freq == base_freq &&
                at_simplified_local.spectrum_idx == spectrum_idx &&
                at_simplified_local.cur_freq != actual_freq)
            {
                unsigned short freq_idx = WIOTA_FREQUENCE_INDEX(actual_freq, base_freq, (int)(at_get_bw_value() * 1000));
                if (uc_wiota_set_freq_info(freq_idx))
                {
                    at_simplified_local.cur_freq = actual_freq;
                    at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
                    return AT_RESULT_OK;
                }
                return AT_RESULT_FAILE;
            }
            else
            {
                // 更新本地全局
                at_simplified_local.base_freq = base_freq;
                at_simplified_local.spectrum_idx = spectrum_idx;
                at_simplified_local.cur_freq = actual_freq;
                at_server_printfln("+FREQ:%u", at_simplified_local.cur_freq);
                at_wiota_run();
            }
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_bandwidth_test(void)
{
    at_server_printfln("AT+BW=<bw[25/50/100/200]>");
    return AT_RESULT_OK;
}

static at_result_t at_wiota_bandwidth_query(void)
{
    at_server_printfln("+BW:%s", at_get_bw_str());
    return AT_RESULT_OK;
}

static at_result_t at_wiota_bandwidth_setup(const char *args)
{
    char bw_str[16] = {0};
    float bw = 0.0f;
    char *endptr = RT_NULL;
    unsigned char level = 0;
    int args_len = rt_strlen(args);

    if (args_len >= sizeof(bw_str))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    args = parse((char *)(++args), "s", args_len, bw_str);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    bw = at_strtof(bw_str, &endptr);
    level = at_get_bw_level(bw);
    if (level != 0xFF)
    {
        if (at_simplified_local.bw_level != level)
        {
            at_simplified_local.bw_level = level;
            at_wiota_run();
        }

        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_rate_test(void)
{
    at_server_printfln("AT+RATE=<rate[%d-%d]>", RATE_MODE_1, RATE_MODE_MAX - 1);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_query(void)
{
    at_server_printfln("+RATE:%d", at_simplified_local.rate_mode);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_rate_setup(const char *args)
{
    unsigned int rate = 0;

    args = parse((char *)(++args), "d", &rate);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (rate <= RATE_MODE_8 && rate >= RATE_MODE_1)
    {
        if (at_simplified_local.rate_mode != rate)
        {
            at_simplified_local.rate_mode = rate;
            at_wiota_run();
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_send_test(void)
{
    at_server_printfln("AT+SEND=<addr[1-4294967295]>,<len[1-%d]>", UC_DATA_LENGTH_LIMIT - 4);
    at_server_printfln("AT+SEND=<addr[0]>,<len[1-%d]>", UC_BC_LENGTH_LIMIT - 4);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_send_setup(const char *args)
{
    unsigned char *recv_buf = RT_NULL;
    unsigned int addr = 0, recv_len = 0;

    args = parse((char *)(++args), "d,d", &addr, &recv_len);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    int max_len = addr ? (UC_DATA_LENGTH_LIMIT - 4) : UC_BC_LENGTH_LIMIT - 4;
    if (recv_len < 1 || recv_len > max_len)
    {
        return AT_RESULT_FAILE;
    }

    int malloc_len = 4 + recv_len + CRC16_LEN; // 总长，self_addr + data + crc16
    recv_buf = rt_malloc(malloc_len);
    if (recv_buf == RT_NULL)
    {
        return AT_RESULT_FAILE;
    }
    rt_memset(recv_buf, 0, malloc_len);

    unsigned int self_addr = at_get_address();
    rt_memcpy(recv_buf, &self_addr, 4); // 前4字节为自身地址

    at_server_printfln(">");
    if (at_server_recv(at_get_server(), (char *)&recv_buf[4], recv_len, rt_tick_from_millisecond(10000)) != recv_len)
    {
        rt_kprintf("at_server_recv tiemout\n");
        rt_free(recv_buf);
        return AT_RESULT_FAILE;
    }

    // 数据压缩
    int data_len = 4 + recv_len; // 只压缩self_addr+data
    unsigned char *send_data = recv_buf;
    int comp_data_len = data_len * 2;
    unsigned char *comp_data = rt_malloc(comp_data_len);
    if (comp_data == RT_NULL)
    {
        rt_free(recv_buf);
        return AT_RESULT_FAILE;
    }
    rt_memset(comp_data, 0, comp_data_len);

    comp_data_len = fastlz_compress(recv_buf, data_len, comp_data);
    if (comp_data_len < data_len)
    {
        send_data = comp_data;
        data_len = comp_data_len;
        // 发送前设置压缩标识，每次都要设置
        uc_wiota_set_data_compress_for_send(1);
    }
    else
    {
        uc_wiota_set_data_compress_for_send(0);
    }

    uc_op_result_e result = UC_OP_FAIL;
    unsigned char subframe_num = wiota_app_subframe(addr == 0 ? 0 : 1, data_len);
    if (subframe_num > 0)
    {
        uc_wiota_set_subframe_num(subframe_num);
        at_tx_start();
        result = uc_wiota_send_data(addr, send_data, data_len, RT_NULL, 0, AT_SEND_TIMEOUT, RT_NULL);
        at_tx_end();
    }

    rt_free(comp_data);
    rt_free(recv_buf);

    if (result == UC_OP_SUCC)
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_sleep_test(void)
{
    at_server_printfln("AT+SLEEP=<mode[0-2]>,<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_sleep_setup(const char *args)
{
    unsigned int mode = 0, time = 0; // ms

    args = parse((char *)(++args), "d,d", &mode, &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (mode < SLEEP_MODE_MAX)
    {
        // 模式DEEP_SLEEP_MODE不用检查time
        if (mode >= ALARM_WAKEUP_MODE && (time < MIN_SLEEP_TIME || time > MAX_SLEEP_TIME))
        {
            return AT_RESULT_FAILE;
        }

        at_server_printfln("OK"); // 睡眠之后无法在输出，需提前输出OK
        uart_wait_tx_done();
        if (mode == DEEP_SLEEP_MODE)
        {
            uc_wiota_sleep_enter(0, 0);
        }
        else if (mode == ALARM_WAKEUP_MODE)
        {
            // 向上取整秒
            int sleep_time = (time + 999) / 1000; // ms -> s
            uc_wiota_set_alarm_time(sleep_time);
            uc_wiota_sleep_enter(0, 0);
        }
        else if (mode == RF_WAKEUP_MODE)
        {
            uc_lpm_rx_cfg_t pr_cfg = {0};
            sub_system_config_t config = {0};
            unsigned short max_awaken_id = at_get_max_awaken_id();

            uc_wiota_get_system_config(&config);
            uc_wiota_get_paging_rx_cfg(&pr_cfg);

            pr_cfg.mode = 1;
            // sleep and communication use the same sensitivity config
            pr_cfg.spectrum_idx = config.spectrum_idx;
            pr_cfg.bandwidth = config.bandwidth;
            pr_cfg.symbol_length = config.symbol_length;
            pr_cfg.freq = config.freq_idx;
            pr_cfg.detect_period = time;
            pr_cfg.period_multiple = 1;
            pr_cfg.awaken_id = at_get_address() % (max_awaken_id - 1); // 单播唤醒ID
            pr_cfg.awaken_id_another = max_awaken_id;                  // 取最后一个为广播唤醒ID

            uc_wiota_set_paging_rx_cfg(&pr_cfg);
            uc_wiota_paging_rx_enter(0, 0);
        }
        return AT_RESULT_OK;
    }

    return AT_RESULT_FAILE;
}

static at_result_t at_wiota_wakeup_test(void)
{
    at_server_printfln("AT+WAKEUP=<addr[0-4294967295]>,<time[%d-%d]>", MIN_SLEEP_TIME, MAX_SLEEP_TIME);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_wakeup_setup(const char *args)
{
    unsigned int addr = 0, time = 0; // ms

    args = parse((char *)(++args), "d,d", &addr, &time);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (time >= MIN_SLEEP_TIME && time <= MAX_SLEEP_TIME)
    {
        uc_lpm_tx_cfg_t pt_cfg = {0};
        sub_system_config_t config = {0};
        unsigned short max_awaken_id = at_get_max_awaken_id();

        uc_wiota_get_system_config(&config);
        uc_wiota_get_paging_tx_cfg(&pt_cfg);

        pt_cfg.mode = 1;
        // sleep and communication use the same sensitivity config
        pt_cfg.spectrum_idx = config.spectrum_idx;
        pt_cfg.bandwidth = config.bandwidth;
        pt_cfg.symbol_length = config.symbol_length;
        pt_cfg.freq = config.freq_idx;
        pt_cfg.send_time = time + 100;
        if (addr == 0)
        {
            // 广播唤醒
            pt_cfg.awaken_id = max_awaken_id;
        }
        else
        {
            // 单播唤醒
            pt_cfg.awaken_id = addr % (max_awaken_id - 1);
        }
        uc_wiota_set_paging_tx_cfg(&pt_cfg);
        if (uc_wiota_paging_tx_start())
        {
            return AT_RESULT_OK;
        }
    }

    return AT_RESULT_FAILE;
}

AT_CMD_EXPORT("AT+SN", RT_NULL, RT_NULL, at_wiota_sn_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+VER", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+RSSI", RT_NULL, RT_NULL, at_wiota_rssi_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+SYNC", "=<sync>", at_wiota_sync_test, at_wiota_sync_query, at_wiota_sync_setup, RT_NULL);
AT_CMD_EXPORT("AT+ADDR", "=<addr>", at_wiota_address_test, at_wiota_address_query, at_wiota_address_setup, RT_NULL);
AT_CMD_EXPORT("AT+POWER", "=<power>", at_wiota_power_test, at_wiota_power_query, at_wiota_power_setup, RT_NULL);
AT_CMD_EXPORT("AT+FREQ", "=<freq>", at_wiota_freq_test, at_wiota_freq_query, at_wiota_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+BW", "=<bw>", at_wiota_bandwidth_test, at_wiota_bandwidth_query, at_wiota_bandwidth_setup, RT_NULL);
AT_CMD_EXPORT("AT+RATE", "=<rate>", at_wiota_rate_test, at_wiota_rate_query, at_wiota_rate_setup, RT_NULL);
AT_CMD_EXPORT("AT+SEND", "=<addr>,<len>", at_wiota_send_test, RT_NULL, at_wiota_send_setup, RT_NULL);
AT_CMD_EXPORT("AT+SLEEP", "=<mode>,<time>", at_wiota_sleep_test, RT_NULL, at_wiota_sleep_setup, RT_NULL);
AT_CMD_EXPORT("AT+WAKEUP", "=<addr>,<time>", at_wiota_wakeup_test, RT_NULL, at_wiota_wakeup_setup, RT_NULL);
