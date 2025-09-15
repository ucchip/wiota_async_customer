/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date                 Author                Notes
 * 20201-8-17     ucchip-wz          v0.00
 */
#include <rtthread.h>
#ifdef RT_USING_AT
#ifdef AT_USING_SERVER
#ifdef UC8288_MODULE
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#include "at.h"
#include "ati_prs.h"
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_uart.h"
//#include "uc_boot_download.h"
#include "at_wiota.h"
#include "at_wiota_gpio_report.h"
#ifdef _QUICK_CONNECT_
#include "quick_connect.h"
#endif

const unsigned short symLen_mcs_byte[4][8] = {{7, 9, 52, 66, 80, 0, 0, 0},
                                              {7, 15, 22, 52, 108, 157, 192, 0},
                                              {7, 15, 31, 42, 73, 136, 255, 297},
                                              {7, 15, 31, 63, 108, 220, 451, 619}};

enum at_wiota_lpm
{
    AT_WIOTA_SLEEP = 0,
    AT_WIOTA_PAGING_TX,     // 1, send tx task, system is noral
    AT_WIOTA_PAGING_RX,     // 2, enter paging mode(system is sleep), only phy is working
    AT_WIOTA_GATING,        // 3
    AT_WIOTA_CLOCK,         // 4
    AT_WIOTA_LPM,           // 5
    AT_WIOTA_FREQ_DIV,      // 6
    AT_WIOTA_VOL_MODE,      // 7
    AT_WIOTA_EX_WK,         // 8
    AT_WIOTA_GATING_CONFIG, // 9
    AT_WIOTA_EXTRA_RF,      // 10
    AT_WIOTA_GATING_PRINT,  // 11
    AT_WIOTA_LPM_MAX,
};

enum at_wiota_log
{
    AT_LOG_CLOSE = 0,
    AT_LOG_OPEN,
    AT_LOG_UART0,
    AT_LOG_UART1,
    AT_LOG_SPI_CLOSE,
    AT_LOG_SPI_OPEN,
};

#define ADC_DEV_NAME "adc"
#define WIOTA_TRANS_END_STRING "EOF"

#define WIOTA_SCAN_FREQ_TIMEOUT 120000
#define WIOTA_SEND_TIMEOUT 60000
#define WIOTA_WAIT_DATA_TIMEOUT 10000
#define WIOTA_TRANS_AUTO_SEND 1000
#define WIOTA_SEND_DATA_MUX_LEN 1024
#define WIOTA_DATA_END 0x1A
#define WIOTA_TRANS_MAX_LEN 310
#define WIOTA_TRANS_END_STRING_MAX 8
#define WIOTA_TRANS_BUFF (WIOTA_TRANS_MAX_LEN + WIOTA_TRANS_END_STRING_MAX + CRC16_LEN + 1)

#define WIOTA_MUST_INIT(state)             \
    if (state != AT_WIOTA_INIT)            \
    {                                      \
        return AT_RESULT_REPETITIVE_FAILE; \
    }

#define WIOTA_MUST_RUN(state)              \
    if (state != AT_WIOTA_RUN)             \
    {                                      \
        return AT_RESULT_REPETITIVE_FAILE; \
    }

#define WIOTA_MUST_ALREADY_INIT(state)                   \
    if (state != AT_WIOTA_INIT && state != AT_WIOTA_RUN) \
    {                                                    \
        return AT_RESULT_REPETITIVE_FAILE;               \
    }

enum at_test_mode_data_type
{
    AT_TEST_MODE_RECVDATA = 0,
    AT_TEST_MODE_QUEUE_EXIT,
};

typedef struct at_test_queue_data
{
    enum at_test_mode_data_type type;
    void *data;
    void *paramenter;
} t_at_test_queue_data;

typedef struct at_test_statistical_data
{
    int type;
    int dev;

    int upcurrentrate;
    int upaverate;
    int upminirate;
    int upmaxrate;

    int downcurrentrate;
    int downavgrate;
    int downminirate;
    int downmaxrate;

    int send_fail;
    int recv_fail;
    int max_mcs;
    int msc;
    int power;
    int rssi;
    int snr;
} t_at_test_statistical_data;

// extern dtu_send_t g_dtu_send;
extern at_server_t at_get_server(void);
extern char *parse(char *b, char *f, ...);

static int wiota_state = AT_WIOTA_DEFAULT;

void at_wiota_set_state(int state)
{
    wiota_state = state;
}

int at_wiota_get_state(void)
{
    return wiota_state;
}

static rt_err_t get_char_timeout(rt_tick_t timeout, char *chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}

static at_result_t at_wiota_version_query(void)
{
    unsigned char version[15] = {0};
    unsigned char git_info[36] = {0};
    unsigned char time[36] = {0};
    unsigned int cce_version = 0;

    uc_wiota_get_version(version, git_info, time, &cce_version);

    at_server_printfln("+WIOTAVERSION:%s", version);
    at_server_printfln("+GITINFO:%s", git_info);
    at_server_printfln("+TIME:%s", time);
    at_server_printfln("+CCEVERSION:%x", cce_version);

    return AT_RESULT_OK;
}

static at_result_t at_freq_query(void)
{
    at_server_printfln("+WIOTAFREQ=%d", uc_wiota_get_freq_info());

    return AT_RESULT_OK;
}

static at_result_t at_freq_setup(const char *args)
{
    int freq = 0;

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d", &freq);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_freq_info(freq))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_dcxo_query(void)
{
    at_server_printfln("+WIOTADCXO=0x%x", uc_wiota_get_dcxo());

    return AT_RESULT_OK;
}

static at_result_t at_dcxo_setup(const char *args)
{
    int dcxo = 0;
    int dcxo_idx = 0;

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "y", &dcxo);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("dcxo=0x%x\n", dcxo);

    dcxo_idx = (dcxo >> 12) & 0xFF;

    if (dcxo_idx >= 0 && dcxo_idx <= 64)
    {
        uc_wiota_set_dcxo(dcxo);
    }
    else if (65 == dcxo_idx)
    {
        uc_wiota_set_dcxo_by_temp_curve();
    }

    return AT_RESULT_OK;
}

static at_result_t at_userid_query(void)
{
    unsigned int id[2] = {0};
    unsigned char len = 0;

    uc_wiota_get_userid(&(id[0]), &len);
    at_server_printfln("+WIOTAUSERID=0x%x", id[0]);

    return AT_RESULT_OK;
}

static at_result_t at_userid_setup(const char *args)
{
    unsigned int userid[2] = {0};

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "y", &userid[0]);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("userid:%x\n", userid[0]);

    if (uc_wiota_set_userid(userid, 4))
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_radio_query(void)
{
    rt_uint32_t temp = 0;
    radio_info_t radio;
    rt_device_t adc_dev;

    if (AT_WIOTA_RUN != wiota_state)
    {
        rt_kprintf("radio state %d\n", wiota_state);
        return AT_RESULT_FAILE;
    }

    adc_dev = rt_device_find(ADC_DEV_NAME);
    if (RT_NULL == adc_dev)
    {
        rt_kprintf("adc find fail\n");
    }
    else
    {
        rt_adc_enable((rt_adc_device_t)adc_dev, ADC_CONFIG_CHANNEL_CHIP_TEMP);
        temp = rt_adc_read((rt_adc_device_t)adc_dev, ADC_CONFIG_CHANNEL_CHIP_TEMP);
    }

    uc_wiota_get_radio_info(&radio);
    //temp,rssi,ber,snr,cur_power,max_pow,cur_mcs,max_mcs
    at_server_printfln("+WIOTARADIO=%d,-%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       temp, radio.rssi, radio.ber, radio.snr, radio.cur_power,
                       radio.min_power, radio.max_power, radio.cur_mcs, radio.max_mcs, radio.frac_offset);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_query(void)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);

    at_server_printfln("+WIOTASYSTEMCONFIG=%d,%d,%d,%d,%d,%d,0x%x,0x%x",
                       config.id_len, config.symbol_length, config.bandwidth, config.pz,
                       config.btvalue, config.spectrum_idx, 0, config.subsystemid);

    return AT_RESULT_OK;
}

static at_result_t at_system_config_setup(const char *args)
{
    sub_system_config_t config;
    unsigned int temp[7];

    WIOTA_MUST_INIT(wiota_state)

    uc_wiota_get_system_config(&config);

    args = parse((char *)(++args), "d,d,d,d,d,d,y,y",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5],
                 &(temp[6]), &config.subsystemid);

    config.id_len = (unsigned char)temp[0];
    config.symbol_length = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.pz = (unsigned char)temp[3];
    config.btvalue = (unsigned char)temp[4];
    config.spectrum_idx = (unsigned char)temp[5];

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // default config
    config.pp = 1;

    // rt_kprintf("id_len=%d,symbol_len=%d,dlul=%d,bt=%d,group_num=%d,ap_max_pow=%d,spec_idx=%d,freq_idx=%d,subsystemid=0x%x\n",
    //            config.id_len, config.symbol_length, config.dlul_ratio,
    //            config.btvalue, config.group_number, config.ap_max_pow,
    //            config.spectrum_idx, config.freq_idx, config.subsystemid);

    if (uc_wiota_set_system_config(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_freq_spacing_query(void)
{
    sub_system_config_t config;
    uc_wiota_get_system_config(&config);
    at_server_printfln("+WIOTAFSPACING=%d", config.freqSpacing);
    return AT_RESULT_OK;
}

static at_result_t at_freq_spacing_setup(const char *args)
{
    sub_system_config_t config;
    unsigned int temp;

    WIOTA_MUST_INIT(wiota_state)

    uc_wiota_get_system_config(&config);

    args = parse((char *)(++args), "d", &temp);

    config.freqSpacing = (unsigned char)temp;

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_system_config(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_subsys_id_query(void)
{
    at_server_printfln("+WIOTASUBSYSID=0x%x", uc_wiota_get_subsystem_id());
    return AT_RESULT_OK;
}

static at_result_t at_subsys_id_setup(const char *args)
{
    unsigned int subsysid = 0;

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "y", &subsysid);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_subsystem_id((unsigned int)subsysid))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiota_init_exec(void)
{
    if (wiota_state == AT_WIOTA_DEFAULT || wiota_state == AT_WIOTA_EXIT)
    {
        uc_wiota_init();
        wiota_state = AT_WIOTA_INIT;
        return AT_RESULT_OK;
    }

    return AT_RESULT_REPETITIVE_FAILE;
}

// extern unsigned char state_get_dcxo_idx(void);
// extern signed char l1_adc_temperature_read(void);

void wiota_recv_callback(uc_recv_back_p data)
{
    // rt_kprintf("wiota_recv_callback result %d\n", data->result);

    if (data->data != RT_NULL && (UC_OP_SUCC == data->result ||
                                  UC_OP_PART_SUCC == data->result ||
                                  UC_OP_CRC_FAIL == data->result))
    {
        if (WIOTA_MODE_OUT_UART == wiota_gpio_mode_get())
        {
            // if (g_dtu_send->flag && (!g_dtu_send->at_show))
            // {
            //     at_send_data(data->data, data->data_len);
            //     rt_free(data->data);
            //     return;
            // }
            if (data->type < UC_RECV_SCAN_FREQ || UC_RECV_SUBF_DATA == data->type)
            {
                at_server_printf("+WIOTARECV,-%d,%d,%d,%d,%d,", data->rssi, data->snr, data->type,
                                 data->result, data->data_len);
                at_send_data(data->data, data->data_len);
                at_server_printfln("");
                rt_kprintf("head data %d %d dfe %u\n", data->head_data[0], data->head_data[1], data->cur_rf_cnt);
            }
            rt_free(data->data);
            if (data->type == UC_RECV_PG_TX_DONE)
            {
                at_server_printf("PG TX DONE");
            }
        }
        else
        {
            wiota_data_insert(data);
        }
    }
}

static at_result_t at_wiota_cfun_setup(const char *args)
{
    int state = 0;

    args = parse((char *)(++args), "d", &state);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // rt_kprintf("state = %d\n", state);

    if (1 == state && wiota_state == AT_WIOTA_INIT)
    {
        uc_wiota_run();
        uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
        // uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_STATE_INFO);
        wiota_state = AT_WIOTA_RUN;
    }
    else if (0 == state && wiota_state == AT_WIOTA_RUN)
    {
        uc_wiota_exit();
        wiota_state = AT_WIOTA_EXIT;
    }
    else
        return AT_RESULT_REPETITIVE_FAILE;

    return AT_RESULT_OK;
}

at_result_t at_wiotasend_handle_data(int timeout, int length, unsigned int userId, uc_send callback)
{
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;
    unsigned int timeout_u = (unsigned int)timeout;
    int send_result = 0;

    sendbuffer = (unsigned char *)rt_malloc(length + CRC16_LEN); // reserve CRC16_LEN for low mac
    if (sendbuffer == NULL || length < 2)
    {
        rt_free(sendbuffer);
        at_server_printfln("SEND FAIL");
        return AT_RESULT_NULL;
    }
    psendbuffer = sendbuffer;
    //at_server_printfln("SUCC");
    at_server_printf(">");

    while (length)
    {
        if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            at_server_printfln("SEND FAIL");
            rt_free(sendbuffer);
            return AT_RESULT_NULL;
        }
        length--;
        psendbuffer++;
    }

    send_result = uc_wiota_send_data(userId, sendbuffer, psendbuffer - sendbuffer, RT_NULL,
                                     0, timeout_u > 0 ? timeout_u : WIOTA_SEND_TIMEOUT, callback);

    if (NULL == callback)
    {
        rt_free(sendbuffer);
        sendbuffer = NULL;
        if (UC_OP_SUCC == send_result)
        {
            at_server_printfln("SEND SUCC");
            return AT_RESULT_OK;
        }
        else if (UC_OP_TIMEOUT == send_result)
        {
            at_server_printfln("SEND TOUT");
            return AT_RESULT_NULL;
        }
        else
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_NULL;
        }
    }
    else
    {
        return AT_RESULT_OK;
    }
}

static at_result_t at_wiotasend_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned int userId = 0;

    WIOTA_MUST_RUN(wiota_state)

    args = parse((char *)(++args), "d,d,y", &timeout, &length, &userId);
    if (!args || length <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiotasend_handle_data(timeout, length, userId, RT_NULL);
}

void at_send_callback(uc_send_back_p sendResult)
{
    if (UC_OP_SUCC == sendResult->result)
    {
        rt_free(sendResult->oriPtr); // if timeout, will assert!
        at_server_printfln("SEND SUCC");
    }
    else if (UC_OP_TIMEOUT == sendResult->result)
    {
        at_server_printfln("SEND TIMEOUT");
    }
    else
    {
        rt_free(sendResult->oriPtr); // if timeout, will assert!
        at_server_printfln("SEND FAIL");
    }
}

static at_result_t at_wiotasend_noblock_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned int userId = 0;

    WIOTA_MUST_RUN(wiota_state)

    args = parse((char *)(++args), "d,d,y", &timeout, &length, &userId);
    if (!args || length < 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return at_wiotasend_handle_data(timeout, length, userId, at_send_callback);
}

static at_result_t at_wiotatrans_process(unsigned short timeout, char *strEnd)
{
    uint8_t *pBuff = RT_NULL;
    int result = 0;
    int send_result = 0;
    timeout = (timeout == 0) ? WIOTA_SEND_TIMEOUT : timeout;
    if ((RT_NULL == strEnd) || ('\0' == strEnd[0]))
    {
        strEnd = WIOTA_TRANS_END_STRING;
    }
    uint8_t nLenEnd = strlen(strEnd);
    //    uint8_t nStrEndCount = 0;
    int16_t nSeekRx = 0;
    char nRun = 1;
    //    char nCatchEnd = 0;
    char nSendFlag = 0;

    pBuff = (uint8_t *)rt_malloc(WIOTA_TRANS_BUFF);
    if (pBuff == RT_NULL)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
    at_server_printfln("\r\nEnter transmission mode >");

    while (nRun)
    {
        get_char_timeout(rt_tick_from_millisecond(-1), (char *)&pBuff[nSeekRx]);
        ++nSeekRx;
        if ((nSeekRx > 2) && ('\n' == pBuff[nSeekRx - 1]) && ('\r' == pBuff[nSeekRx - 2]))
        {
            nSendFlag = 1;
            nSeekRx -= 2;
            if ((nSeekRx >= nLenEnd) && pBuff[nSeekRx - 1] == strEnd[nLenEnd - 1])
            {
                int i = 0;
                for (i = 0; i < nLenEnd; ++i)
                {
                    if (pBuff[nSeekRx - nLenEnd + i] != strEnd[i])
                    {
                        break;
                    }
                }
                if (i >= nLenEnd)
                {
                    nSeekRx -= nLenEnd;
                    nRun = 0;
                }
            }
        }

        if ((nSeekRx > (WIOTA_TRANS_MAX_LEN + nLenEnd + 2)) || (nSendFlag && (nSeekRx > WIOTA_TRANS_MAX_LEN)))
        {
            at_server_printfln("\r\nThe message's length can not over 310 characters.");
            do
            {
                // discard any characters after the end string
                result = get_char_timeout(rt_tick_from_millisecond(200), (char *)&pBuff[0]);
            } while (RT_EOK == result);
            nSendFlag = 0;
            nSeekRx = 0;
            nRun = 1;
            rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
            continue;
        }

        if (nSendFlag)
        {
            nSeekRx = (nSeekRx > WIOTA_TRANS_MAX_LEN) ? WIOTA_TRANS_MAX_LEN : nSeekRx;
            if (nSeekRx > 0)
            {
                send_result = uc_wiota_send_data(0, pBuff, nSeekRx, NULL, 0, timeout, RT_NULL);
                if (UC_OP_SUCC == send_result)
                {
                    at_server_printfln("SEND SUCC");
                }
                else if (UC_OP_TIMEOUT == send_result)
                {
                    at_server_printfln("SEND TOUT");
                }
                else
                {
                    at_server_printfln("SEND FAIL");
                }
            }
            nSeekRx = 0;
            nSendFlag = 0;
            rt_memset(pBuff, 0, WIOTA_TRANS_BUFF);
        }
    }

    do
    {
        // discard any characters after the end string
        result = get_char_timeout(rt_tick_from_millisecond(200), (char *)&pBuff[0]);
    } while (RT_EOK == result);

    at_server_printfln("\r\nLeave transmission mode");
    if (RT_NULL != pBuff)
    {
        rt_free(pBuff);
        pBuff = RT_NULL;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiotatrans_setup(const char *args)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    int timeout = 0;
    char strEnd[WIOTA_TRANS_END_STRING_MAX + 1] = {0};

    args = parse((char *)(++args), "d,s", &timeout, (signed long)8, strEnd);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    int i = 0;
    for (i = 0; i < WIOTA_TRANS_END_STRING_MAX; i++)
    {
        if (('\r' == strEnd[i]) || ('\n' == strEnd[i]))
        {
            strEnd[i] = '\0';
            break;
        }
    }

    if (i <= 0)
    {
        strcpy(strEnd, WIOTA_TRANS_END_STRING);
    }
    return at_wiotatrans_process(timeout & 0xFFFF, strEnd);
}

static at_result_t at_wiotatrans_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    return at_wiotatrans_process(0, RT_NULL);
}

#if 0
void dtu_send_process(void)
{
    int16_t nSeekRx = 0;
    uint8_t buff[WIOTA_TRANS_BUFF] = {0};
    rt_err_t result;

    int i = 0;
    for (i = 0; i < WIOTA_TRANS_END_STRING_MAX; i++)
    {
        if (('\0' == g_dtu_send->exit_flag[i]) ||
            ('\r' == g_dtu_send->exit_flag[i]) ||
            ('\n' == g_dtu_send->exit_flag[i]))
        {
            g_dtu_send->exit_flag[i] = '\0';
            break;
        }
    }
    g_dtu_send->flag_len = i;
    if (g_dtu_send->flag_len <= 0)
    {
        strcpy(g_dtu_send->exit_flag, WIOTA_TRANS_END_STRING);
        g_dtu_send->flag_len = strlen(WIOTA_TRANS_END_STRING);
    }
    g_dtu_send->timeout = g_dtu_send->timeout ? g_dtu_send->timeout : 5000;
    g_dtu_send->wait = g_dtu_send->wait ? g_dtu_send->wait : 200;

    while (g_dtu_send->flag)
    {
        result = get_char_timeout(rt_tick_from_millisecond(g_dtu_send->wait), (char *)&buff[nSeekRx]);
        if (RT_EOK == result)
        {
            nSeekRx++;
            if ((nSeekRx >= g_dtu_send->flag_len) && (buff[nSeekRx - 1] == g_dtu_send->exit_flag[g_dtu_send->flag_len - 1]))
            {

                int i = 0;
                for (i = 0; i < g_dtu_send->flag_len; ++i)
                {
                    if (buff[nSeekRx - g_dtu_send->flag_len + i] != g_dtu_send->exit_flag[i])
                    {
                        break;
                    }
                }
                if (i >= g_dtu_send->flag_len)
                {
                    nSeekRx -= g_dtu_send->flag_len;
                    g_dtu_send->flag = 0;
                }
            }
            if (g_dtu_send->flag && (nSeekRx > (WIOTA_TRANS_MAX_LEN + g_dtu_send->flag_len)))
            {
                // too long to send
                result = RT_ETIMEOUT;
            }
        }
        if ((nSeekRx > 0) && ((RT_EOK != result) || (0 == g_dtu_send->flag)))
        {
            // timeout to send
            if (nSeekRx > WIOTA_TRANS_MAX_LEN)
            {
                nSeekRx = WIOTA_TRANS_MAX_LEN;
                do
                {
                    // discard any characters after the end string
                    char ch;
                    result = get_char_timeout(rt_tick_from_millisecond(100), &ch);
                } while (RT_EOK == result);
            }

            if ((AT_WIOTA_RUN == wiota_state) &&
                (UC_OP_SUCC == uc_wiota_send_data(0, buff, nSeekRx, NULL, 0, g_dtu_send->timeout, RT_NULL)))
            {
                if (g_dtu_send->at_show)
                {
                    at_server_printfln("SEND:%4d.", nSeekRx);
                }
            }
            else
            {
                at_server_printfln("SEND FAIL");
            }
            nSeekRx = 0;
            rt_memset(buff, 0, WIOTA_TRANS_BUFF);
        }
    }
    do
    {
        // discard any characters after the end string
        result = get_char_timeout(rt_tick_from_millisecond(100), (char *)&buff[0]);
    } while (RT_EOK == result);
    if (0 == g_dtu_send->flag)
    {
        at_server_printfln("OK");
    }
}

static at_result_t at_wiota_dtu_send_setup(const char *args)
{
    if ((AT_WIOTA_RUN != wiota_state) || (RT_NULL == g_dtu_send))
    {
        return AT_RESULT_FAILE;
    }
    rt_memset(g_dtu_send->exit_flag, 0, WIOTA_TRANS_END_STRING_MAX);
    int timeout = 0;
    int wait = 0;
    args = parse((char *)(++args), "d,d,s", &timeout, &wait, (signed long)WIOTA_TRANS_END_STRING_MAX, g_dtu_send->exit_flag);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    g_dtu_send->timeout = timeout & 0xFFFF;
    g_dtu_send->wait = wait & 0xFFFF;
    g_dtu_send->flag = 1;
    return AT_RESULT_OK;
}

static at_result_t at_wiota_dtu_send_exec(void)
{
    if (AT_WIOTA_RUN != wiota_state)
    {
        return AT_RESULT_FAILE;
    }
    g_dtu_send->flag = 1;
    return AT_RESULT_OK;
}
#endif

static at_result_t at_wiotarecv_setup(const char *args)
{
    unsigned short timeout = 0;
    unsigned int userId = 0;
    uc_recv_back_t result;

    if (AT_WIOTA_DEFAULT == wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    args = parse((char *)(++args), "d,y", &timeout, &userId);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (timeout < 1)
        timeout = WIOTA_WAIT_DATA_TIMEOUT;

    // rt_kprintf("timeout = %d\n", timeout);

    uc_wiota_recv_data(&result, timeout, userId, RT_NULL);
    if (!result.result)
    {
        if (result.type < UC_RECV_MAX_TYPE)
        {
            at_server_printf("+WIOTARECV,-%d,%d,%d,%d,%d,", result.rssi, result.snr, result.type,
                             result.result, result.data_len);
        }
        at_send_data(result.data, result.data_len);
        at_server_printfln("");
        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiota_recv_exec(void)
{
    uc_recv_back_t result;

    if (AT_WIOTA_DEFAULT == wiota_state)
    {
        return AT_RESULT_FAILE;
    }

    uc_wiota_recv_data(&result, WIOTA_WAIT_DATA_TIMEOUT, 0, RT_NULL);
    if (!result.result)
    {
        if (result.type < UC_RECV_MAX_TYPE)
        {
            at_server_printf("+WIOTARECV,-%d,%d,%d,%d,%d,", result.rssi, result.snr, result.type,
                             result.result, result.data_len);
        }
        at_send_data(result.data, result.data_len);
        at_server_printfln("");
        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_wiotalpm_setup(const char *args)
{
    int mode = 0, value = 0, value2 = 0;

    args = parse((char *)(++args), "d,d,d", &mode, &value, &value2);

    switch (mode)
    {
    case AT_WIOTA_SLEEP:
    {
        at_server_printfln("OK");
        uart_wait_tx_done();
        uc_wiota_sleep_enter((unsigned char)value, (unsigned char)value2);
        break;
    }
#ifdef _LPM_PAGING_
    case AT_WIOTA_PAGING_TX:
    {
        if (!uc_wiota_paging_tx_start())
        {
            return AT_RESULT_FAILE;
        }
        break;
    }
    case AT_WIOTA_PAGING_RX:
    {
        // WIOTA_MUST_RUN(wiota_state)
        at_server_printfln("OK");
        uart_wait_tx_done();
        uc_wiota_paging_rx_enter((unsigned char)value, (unsigned int)value2);
        break;
    }
#endif // _LPM_PAGING_
    case AT_WIOTA_CLOCK:
    {
        uc_wiota_set_alarm_time((unsigned int)value);
        break;
    }
#ifdef _CLK_GATING_
    case AT_WIOTA_GATING:
    {
        uc_wiota_set_is_gating((unsigned char)value, (unsigned char)value2);
        break;
    }
#endif
    case AT_WIOTA_LPM:
    {
        uc_wiota_set_lpm_mode((unsigned char)value);
        break;
    }
    case AT_WIOTA_FREQ_DIV:
    {
        uc_wiota_set_freq_div((unsigned char)value);
        break;
    }
    case AT_WIOTA_VOL_MODE:
    {
        uc_wiota_set_vol_mode((unsigned char)value);
        break;
    }
    case AT_WIOTA_EX_WK:
    {
        uc_wiota_set_is_ex_wk((unsigned char)value);
        break;
    }
#ifdef _CLK_GATING_
    case AT_WIOTA_GATING_CONFIG:
    {
        uc_wiota_set_gating_config((unsigned int)value, (unsigned int)value2);
        break;
    }
    case AT_WIOTA_EXTRA_RF:
    {
        uc_wiota_set_extra_rf_before_send((unsigned int)value);
        break;
    }
    case AT_WIOTA_GATING_PRINT:
    {
        uc_wiota_set_gating_print((unsigned int)value);
        break;
    }
#endif
    default:
        return AT_RESULT_FAILE;
    }
    return AT_RESULT_OK;
}

static at_result_t at_wiotarate_setup(const char *args)
{
    int rate_mode = 0xFF;
    int rate_value = 0xFF;

    args = parse((char *)(++args), "d,d", &rate_mode, &rate_value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // at_server_printfln("+WIOTARATE: %d, %d", (unsigned char)rate_mode, (unsigned short)rate_value);
    if (uc_wiota_set_data_rate((unsigned char)rate_mode, (unsigned short)rate_value))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiotarate_query(void)
{
    radio_info_t radio;
    unsigned char cur_mcs = 0;
    unsigned short first_bc_data;
    unsigned short first_uni_data;
    unsigned short second_uni_data;
    unsigned short normal_bc_data;
    unsigned short normal_uni_data;

    uc_wiota_get_radio_info(&radio);
    cur_mcs = radio.cur_mcs;
    normal_bc_data = uc_wiota_get_subframe_data_len(cur_mcs, 1, 0, 0);
    normal_uni_data = uc_wiota_get_subframe_data_len(cur_mcs, 0, 0, 0);
    first_bc_data = uc_wiota_get_subframe_data_len(0, 1, 1, 0);
    first_uni_data = uc_wiota_get_subframe_data_len(0, 0, 1, 1);
    second_uni_data = uc_wiota_get_subframe_data_len(0, 0, 1, 0);

    at_server_printfln("+WIOTARATE:%d,%d,%d,%d,%d,%d", cur_mcs, first_bc_data,
                       normal_bc_data, first_uni_data, second_uni_data, normal_uni_data);

    return AT_RESULT_OK;
}

static at_result_t at_wiotapow_setup(const char *args)
{
    int mode = 0;
    int power = 0x7F;

    args = parse((char *)(++args), "d,d", &mode, &power);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    // at can't parse minus value for now
    if (mode == 0)
    {
        uc_wiota_set_cur_power((signed char)(power - 20));
    }
    else if (mode == 1)
    {
        uc_wiota_set_max_power((signed char)(power - 20));
    }
    else if (mode == 2)
    {
        uc_wiota_set_auto_ack_pow((unsigned char)power);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)

void at_handle_log_uart(int uart_number)
{
    rt_device_t device = NULL;
    //    rt_device_t old_device = NULL;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /*init default parment*/

    device = rt_device_find(AT_SERVER_DEVICE);

    if (device)
    {
        rt_device_close(device);
    }

    if (0 == uart_number)
    {
        config.baud_rate = BAUD_RATE_460800;
        rt_console_set_device(AT_SERVER_DEVICE);
        //boot_set_uart0_baud_rate(BAUD_RATE_460800);
    }
    else if (1 == uart_number)
    {
        config.baud_rate = BAUD_RATE_115200;
        rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
        //boot_set_uart0_baud_rate(BAUD_RATE_115200);
    }

    if (device)
    {
        rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
        rt_device_open(device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    }
}

#endif

static at_result_t at_wiotalog_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    switch (mode)
    {
    case AT_LOG_CLOSE:
    case AT_LOG_OPEN:
        uc_wiota_log_switch(UC_LOG_UART, mode - AT_LOG_CLOSE);
        break;

    case AT_LOG_UART0:
    case AT_LOG_UART1:
#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
        at_handle_log_uart(mode - AT_LOG_UART0);
#endif
        break;

    case AT_LOG_SPI_CLOSE:
    case AT_LOG_SPI_OPEN:
        uc_wiota_log_switch(UC_LOG_SPI, mode - AT_LOG_SPI_CLOSE);
        break;

    default:
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotastats_query(void)
{
    uc_stats_info_t local_stats_t = {0};

    uc_wiota_get_all_stats(&local_stats_t);

    at_server_printfln("+WIOTASTATS=0,%d,%d,%d,%d,%d,%d,%d,%d", local_stats_t.uni_send_total, local_stats_t.uni_send_succ,
                       local_stats_t.bc_send_total, local_stats_t.bc_send_succ,
                       local_stats_t.uni_recv_fail, local_stats_t.uni_recv_succ,
                       local_stats_t.bc_recv_fail, local_stats_t.bc_recv_succ);

    return AT_RESULT_OK;
}

static at_result_t at_wiotastats_setup(const char *args)
{
    int mode = 0;
    int type = 0;
    unsigned int back_stats;

    args = parse((char *)(++args), "d,d", &mode, &type);
    if (!args || type >= UC_STATS_TYPE_MAX)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (UC_STATS_READ == mode)
    {
        if (UC_STATS_TYPE_ALL == type)
        {
            at_wiotastats_query();
        }
        else
        {
            back_stats = uc_wiota_get_stats((unsigned char)type);
            at_server_printfln("+WIOTASTATS=%d,%d", type, back_stats);
        }
    }
    else if (UC_STATS_WRITE == mode)
    {
        uc_wiota_reset_stats((unsigned char)type);
    }
    else
    {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_query(void)
{
    at_server_printfln("+WIOTACRC=%d", uc_wiota_get_crc());

    return AT_RESULT_OK;
}

static at_result_t at_wiotacrc_setup(const char *args)
{
    int crc_limit = 0;

    args = parse((char *)(++args), "d", &crc_limit);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_crc((unsigned short)crc_limit);

    return AT_RESULT_OK;
}

static at_result_t at_wiotaosc_query(void)
{
    at_server_printfln("+WIOTAOSC=%d", uc_wiota_get_is_osc());

    return AT_RESULT_OK;
}

static at_result_t at_wiotaosc_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_is_osc((unsigned char)mode);

    return AT_RESULT_OK;
}

#ifdef _LIGHT_PILOT_
static at_result_t at_wiotalight_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_light_func_enable((unsigned char)mode);

    return AT_RESULT_OK;
}
#endif

static at_result_t at_wiota_save_static_exec(void)
{
    uc_wiota_save_static_info();

    return AT_RESULT_OK;
}

static at_result_t at_wiota_subframe_num_query(void)
{
    unsigned char subframe_num = uc_wiota_get_subframe_num();

    at_server_printfln("+WIOTASUBFNUM:%d", subframe_num);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_subframe_num_setup(const char *args)
{
    int number = 0;

    args = parse((char *)(++args), "d", &number);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_subframe_num((unsigned char)number))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_wiota_bc_round_setup(const char *args)
{
    int number = 0;

    args = parse((char *)(++args), "d", &number);

    if (!args || number <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_bc_round((unsigned char)number))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiota_detect_time_setup(const char *args)
{
    int number = 0;

    args = parse((char *)(++args), "d", &number);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_detect_time((unsigned int)number);

    return AT_RESULT_OK;
}

// static at_result_t at_wiota_bandwidth_setup(const char *args)
// {
//     int number = 0;

//     args = parse((char *)(++args), "d", &number);

//     if (!args || number < 0)
//     {
//         return AT_RESULT_PARSE_FAILE;
//     }

//     uc_wiota_set_bandwidth((unsigned char)number);

//     return AT_RESULT_OK;
// }

static at_result_t at_wiota_continue_send_setup(const char *args)
{
    int flag = 0;

    args = parse((char *)(++args), "d", &flag);

    if (!args || flag < 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_continue_send((unsigned char)flag);

    return AT_RESULT_OK;
}

#ifdef _SUBFRAME_MODE_
static at_result_t at_wiota_subframe_mode_setup(const char *args)
{
    int mode = 0;
    int flag = 0;

    args = parse((char *)(++args), "d, d", &mode, &flag);

    if (!args || flag < 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == mode)
    {
        uc_wiota_set_subframe_send((unsigned char)flag);
    }
    else if (1 == mode)
    {
        uc_wiota_set_subframe_recv((unsigned char)flag);
    }
    else if (2 == mode || 3 == mode) // 2 or 3
    {
        uc_wiota_set_two_way_mode((mode - 2), (unsigned char)flag);
    }
    else if (4 == mode && flag > 0)
    {
        uc_wiota_set_subframe_data_limit((unsigned int)flag);
    }
    else if (5 == mode)
    {
        uc_wiota_set_two_mode_recv((unsigned char)flag);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}
#endif

static at_result_t at_wiota_incomplete_recv_setup(const char *args)
{
    int flag = -1;

    args = parse((char *)(++args), "d", &flag);

    if (!args || flag < 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_incomplete_recv((unsigned char)flag);

    return AT_RESULT_OK;
}

static at_result_t at_wiotatxmode_setup(const char *args)
{
    int mode = 0;
    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (0 <= mode && mode <= 3)
    {
        if (uc_wiota_set_tx_mode((unsigned char)mode))
        {
            return AT_RESULT_OK;
        }
    }
    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiotarecvmode_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (uc_wiota_set_recv_mode((unsigned char)mode))
    {
        return AT_RESULT_OK;
    }

    return AT_RESULT_PARSE_FAILE;
}

static at_result_t at_wiotaframeinfo_query(void)
{
    unsigned int frame_len_bc = uc_wiota_get_frame_len(0);
    unsigned int frame_len = uc_wiota_get_frame_len(1);
    unsigned int subframe_len = uc_wiota_get_subframe_len();

    at_server_printfln("+WIOTAFRAMEINFO:%d,%d,%d", frame_len_bc, frame_len, subframe_len);

    return AT_RESULT_OK;
}

static at_result_t at_wiotastate_query(void)
{
    at_server_printfln("+WIOTASTATE:%d", uc_wiota_get_state());

    return AT_RESULT_OK;
}

static at_result_t at_wiotaunifailcnt_setup(const char *args)
{
    int cnt = 0;

    args = parse((char *)(++args), "d", &cnt);

    if (!args || cnt <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_unisend_fail_cnt((unsigned char)cnt);

    return AT_RESULT_OK;
}

static unsigned int nth_power(unsigned int num, unsigned int n)
{
    unsigned int s = 1;

    for (unsigned int i = 0; i < n; i++)
    {
        s *= num;
    }
    return s;
}

static void convert_string_to_int(unsigned char numLen, unsigned short num, const unsigned char *pStart, uc_freq_scan_req_p array)
{
    unsigned char *temp = NULL;
    unsigned char len = 0;
    unsigned char nth = numLen;
    unsigned short tempNum = 0;

    temp = (unsigned char *)rt_malloc(numLen);
    if (temp == NULL)
    {
        rt_kprintf("convert_string_to_int malloc failed\n");
        return;
    }

    for (len = 0; len < numLen; len++)
    {
        temp[len] = pStart[len] - '0';
        tempNum += nth_power(10, nth - 1) * temp[len];
        nth--;
    }
    rt_memcpy((array + num), &tempNum, sizeof(unsigned short));
    rt_free(temp);
    temp = NULL;
}

static unsigned short convert_string_to_array(unsigned char *string, uc_freq_scan_req_p array)
{
    unsigned char *pStart = string;
    unsigned char *pEnd = string;
    unsigned short num = 0;
    unsigned char numLen = 0;

    while (*pStart != '\0')
    {
        while (*pEnd != '\0')
        {
            if (*pEnd == ',')
            {
                convert_string_to_int(numLen, num, pStart, array);
                num++;
                pEnd++;
                pStart = pEnd;
                numLen = 0;
            }
            numLen++;
            pEnd++;
        }

        convert_string_to_int(numLen, num, pStart, array);
        num++;
        pStart = pEnd;
    }
    return num;
}

static at_result_t at_scan_freq_setup(const char *args)
{
    unsigned int freqNum = 0;
    unsigned int timeout = 0;
    unsigned char *freqString = RT_NULL;
    unsigned char *tempFreq = RT_NULL;
    uc_recv_back_t result;
    unsigned int convertNum = 0;
    uc_freq_scan_req_p freqArry = NULL;
    unsigned int dataLen = 0;
    unsigned int strLen = 0;
    unsigned int round = 0;

    if (wiota_state != AT_WIOTA_RUN)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    args = parse((char *)(++args), "d,d,d,d", &timeout, &round, &dataLen, &freqNum);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    // strLen = dataLen;

    if (freqNum > 0)
    {
        // calc max len, freq "65535," is 6bytes
        freqString = (unsigned char *)rt_malloc(freqNum * 6 + 1);
        if (freqString == RT_NULL)
        {
            at_server_printfln("MEM FAIL");
            return AT_RESULT_NULL;
        }
        tempFreq = freqString;
        at_server_printfln("OK");
        at_server_printf(">");
        while (1)
        {
            if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)tempFreq) != RT_EOK)
            {
                rt_free(freqString);
                at_server_printfln("GET FAIL");
                return AT_RESULT_NULL;
            }
            // dataLen--;
            strLen++;
            if (strLen > 1 && *tempFreq == '\n' && *(tempFreq - 1) == '\r')
            {
                break;
            }
            tempFreq++;
        }

        freqArry = (uc_freq_scan_req_p)rt_malloc(freqNum * sizeof(uc_freq_scan_req_t));
        if (freqArry == NULL)
        {
            rt_free(freqString);
            return AT_RESULT_NULL;
        }
        rt_memset(freqArry, 0, freqNum * sizeof(uc_freq_scan_req_t));

        freqString[strLen - 2] = '\0'; // add string end flag

        convertNum = convert_string_to_array(freqString, freqArry);
        if (convertNum != freqNum)
        {
            rt_free(freqString);
            rt_free(freqArry);
            at_server_printfln("CONVERT FAIL");
            return AT_RESULT_NULL;
        }
        rt_free(freqString);
        uc_wiota_scan_freq((unsigned char *)freqArry, (unsigned short)(freqNum * sizeof(uc_freq_scan_req_t)),
                           (unsigned char)round, timeout, RT_NULL, &result);
        rt_free(freqArry);
    }
    else
    {
        // uc_wiota_scan_freq(RT_NULL, 0, WIOTA_SCAN_FREQ_TIMEOUT, RT_NULL, &result);
        uc_wiota_scan_freq(RT_NULL, 0, (unsigned char)round, 0, RT_NULL, &result); // scan all wait for ever
    }

    if (UC_OP_SUCC == result.result)
    {
        uc_freq_scan_result_p freqlinst = (uc_freq_scan_result_p)(result.data);
        int freq_num = result.data_len / sizeof(uc_freq_scan_result_t);

        at_server_printfln("+WIOTASCANFREQ:");

        for (int i = 0; i < freq_num; i++)
        {
            at_server_printfln("%d,%d", freqlinst->freq_idx, freqlinst->rssi);
            freqlinst++;
        }

        rt_free(result.data);
        return AT_RESULT_OK;
    }
    else if (UC_OP_TIMEOUT == result.result)
    {
        at_server_printfln("SCAN TOUT");
    }
    else if (UC_OP_FAIL == result.result)
    {
        at_server_printfln("SCAN FAIL");
    }

    return AT_RESULT_NULL;
}
static at_result_t at_scan_sort_setup(const char *args)
{
    unsigned int mode;

    args = parse((char *)(++args), "d", &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == mode || 1 == mode)
    {
        uc_wiota_set_scan_sorted((unsigned char)mode);
    }
    else if (2 == mode || 3 == mode)
    {
        uc_wiota_set_scan_max((unsigned char)mode - 2);
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }

    return AT_RESULT_OK;
}

#ifdef _LPM_PAGING_
static at_result_t at_paging_tx_config_query(void)
{
    uc_lpm_tx_cfg_t config;
    uc_wiota_get_paging_tx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGTX=%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth,
                       config.symbol_length, config.awaken_id, config.send_time);

    return AT_RESULT_OK;
}

static at_result_t at_paging_tx_config_setup(const char *args)
{
    uc_lpm_tx_cfg_t config = {0};
    unsigned int temp[6];

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_tx_cfg(&config);

    config.freq = (unsigned short)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.send_time = (unsigned int)temp[5];

    if (uc_wiota_set_paging_tx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_rx_config_query(void)
{
    uc_lpm_rx_cfg_t config;
    uc_wiota_get_paging_rx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGRX=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       config.freq, config.spectrum_idx, config.bandwidth,
                       config.symbol_length, config.awaken_id, config.detect_period,
                       config.lpm_nlen, config.lpm_utimes, config.threshold,
                       config.extra_flag, config.extra_period);

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_setup(const char *args)
{
    uc_lpm_rx_cfg_t config = {0};
    unsigned int temp[11];

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d,d,d,d,d,d,d,d,d,d",
                 &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &temp[6], &temp[7], &temp[8], &temp[9], &temp[10]);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config);

    config.freq = (unsigned short)temp[0];
    config.spectrum_idx = (unsigned char)temp[1];
    config.bandwidth = (unsigned char)temp[2];
    config.symbol_length = (unsigned char)temp[3];
    config.awaken_id = (unsigned short)temp[4];
    config.detect_period = (unsigned int)temp[5];
    config.lpm_nlen = (unsigned char)temp[6];
    config.lpm_utimes = (unsigned char)temp[7];
    config.threshold = (unsigned short)temp[8];
    config.extra_flag = (unsigned short)temp[9];
    config.extra_period = (unsigned int)temp[10];

    if (uc_wiota_set_paging_rx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_rx_config_another_query(void)
{
    uc_lpm_rx_cfg_t config;
    uc_wiota_get_paging_rx_cfg(&config);
    at_server_printfln("+WIOTAPAGINGRXANO=%d,%d",
                       config.period_multiple, config.awaken_id_another);

    return AT_RESULT_OK;
}

static at_result_t at_paging_rx_config_another_setup(const char *args)
{
    uc_lpm_rx_cfg_t config = {0};
    unsigned int temp[3];

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d", &temp[0], &temp[1]);

    if (!args || temp[0] > 255)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config);

    config.period_multiple = (unsigned char)temp[0];
    config.awaken_id_another = (unsigned short)temp[1];

    if (uc_wiota_set_paging_rx_cfg(&config))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}

static at_result_t at_paging_config_mode_query(void)
{
    uc_lpm_rx_cfg_t config_rx;
    uc_lpm_tx_cfg_t config_tx;
    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);
    at_server_printfln("+WIOTAPAGINGMODE=%d,%d", config_rx.mode, config_tx.mode);
    return AT_RESULT_OK;
}

static at_result_t at_paging_config_mode_setup(const char *args)
{
    uc_lpm_rx_cfg_t config_rx = {0};
    uc_lpm_tx_cfg_t config_tx = {0};
    unsigned int rx_mode, tx_mode;

    // WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d,d", &rx_mode, &tx_mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_get_paging_rx_cfg(&config_rx);
    uc_wiota_get_paging_tx_cfg(&config_tx);

    config_rx.mode = rx_mode;
    config_tx.mode = tx_mode;

    if (uc_wiota_set_paging_rx_cfg(&config_rx) && uc_wiota_set_paging_tx_cfg(&config_tx))
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_PARSE_FAILE;
}
#endif // _LPM_PAGING_
static at_result_t at_wiota_symbol_mode_setup(const char *args)
{
    int symbol_mode = 0;

    args = parse((char *)(++args), "d", &symbol_mode);

    if (!args || symbol_mode > 3)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_symbol_mode((unsigned char)(symbol_mode & 0x3));

    return AT_RESULT_OK;
}

static at_result_t at_memory_query(void)
{
    unsigned int total = 0;
    unsigned int used = 0;
    unsigned int max_used = 0;
#ifndef RT_USING_MEMHEAP_AS_HEAP
#if defined(RT_USING_HEAP) && defined(RT_USING_SMALL_MEM)
    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total %d used %d maxused %d\n", total, used, max_used);
#endif
#endif

    at_server_printfln("+MEMORY=%d,%d,%d", total, used, max_used);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_control_mode_setup(const char *args)
{
    int ctl_mode = 0;

    args = parse((char *)(++args), "d", &ctl_mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (0 == ctl_mode)
    {
        uc_wiota_suspend();
    }
    else if (1 == ctl_mode)
    {
        uc_wiota_recover();
    }

    return AT_RESULT_OK;
}
#ifdef _LPM_PAGING_
static at_result_t at_wiotawaken_query(void)
{
    unsigned char awaken_cause = 0;
    unsigned char is_cs_awawen = 0;
    unsigned char pg_awaken_cause = 0;
    unsigned char detect_idx = 0;
    unsigned int detected_times = 0;

    awaken_cause = uc_wiota_get_awakened_cause(&is_cs_awawen);

    if (AWAKENED_CAUSE_PAGING == awaken_cause)
    {
        pg_awaken_cause = uc_wiota_get_paging_awaken_cause(&detected_times, &detect_idx);
    }

    at_server_printfln("+WIOTAWAKEN:%d,%d,%d,%u,%d", awaken_cause, is_cs_awawen, pg_awaken_cause, detected_times, detect_idx);

    return AT_RESULT_OK;
}
#endif

#ifdef _SUBFRAME_MODE_
static at_result_t at_wiota_two_way_subfnum_setup(const char *args)
{
    int number1, number2 = 0;

    args = parse((char *)(++args), "d,d", &number1, &number2);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (uc_wiota_set_two_way_subframe_num((unsigned char)number1, (unsigned char)number2))
    {
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_PARSE_FAILE;
    }
}
#endif

static at_result_t at_wiota_uni_ack_info_setup(const char *args)
{
    unsigned int is_ack, ack_info = 0;
    uc_uni_ack_info_t uni_ack_info = {0};

    args = parse((char *)(++args), "d,d", &is_ack, &ack_info);

    if (!args || ack_info > 1 || is_ack > 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    uc_wiota_get_uni_ack_info(&uni_ack_info);
    uni_ack_info.is_ack_info = is_ack;
    uni_ack_info.is_same_info = is_ack;
    uni_ack_info.same_info = ack_info;
    uc_wiota_set_uni_ack_info(&uni_ack_info);
    return AT_RESULT_OK;
}

static at_result_t at_wiota32k_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_outer_32K((unsigned char)mode);

    return AT_RESULT_OK;
}

static at_result_t at_wiota_reset_rf_exec(void)
{
    if (wiota_state == AT_WIOTA_RUN)
    {
        uc_wiota_reset_rf();
        return AT_RESULT_OK;
    }

    return AT_RESULT_REPETITIVE_FAILE;
}

static at_result_t at_module_id_query(void)
{
    unsigned char module_id[19] = {0};
    unsigned char valid;

    valid = uc_wiota_get_module_id(module_id);

    at_server_printfln("+MODULEID=%s,%d", module_id, valid);

    return AT_RESULT_OK;
}

#ifdef _LPM_PAGING_
static at_result_t at_wiota_embed_pgtx_setup(const char *args)
{
    int flag = 0;

    args = parse((char *)(++args), "d", &flag);

    if (!args || flag < 0 || flag > 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_embed_pgtx((unsigned char)flag);

    return AT_RESULT_OK;
}
#endif
static at_result_t at_wiota_adc_adj_query(void)
{
    uc_adc_adj_t adc_adj = {0};
    uc_wiota_get_adc_adj_info(&adc_adj);
    at_server_printfln("+WIOTADCDJ=%d,%d,%d,%d,%d,%d,%d", adc_adj.is_close,
                       adc_adj.is_valid, adc_adj.adc_trm, adc_adj.adc_ka, adc_adj.adc_mida,
                       adc_adj.adc_kb, adc_adj.adc_midb);
    return AT_RESULT_OK;
}

static at_result_t at_wiota_adc_adj_setup(const char *args)
{
    int mode = 0;

    args = parse((char *)(++args), "d", &mode);

    if (!args || mode < 0 || mode >1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_adc_adj_close(mode); // 1 means close!
    return AT_RESULT_OK;
}

#ifdef _QUICK_CONNECT_
static at_result_t at_wiota_quick_start(const char *args)
{
    int ret = AT_RESULT_FAILE;
    unsigned char onoff = 0;
    e_qc_mode mode = 0;
    unsigned short freq = 0;

    args = parse((char *)(++args), "d,d,d", &onoff, &freq, &mode);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (onoff == 1)
    {
        ret = wiota_quick_connect_start((unsigned short)freq, (e_qc_mode)mode);
    }
    else if (onoff == 0)
    {
        ret = wiota_quick_connect_stop();
    }

    return ret;
}
#endif

static at_result_t at_search_list_setup(const char *args)
{
    int bandwidth = 0;
    unsigned int value = 0;

    args = parse((char *)(++args), "d,y", &bandwidth, &value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (uc_wiota_set_search_list((unsigned char)bandwidth, value))
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_FAILE;
}

static at_result_t at_new_ldo_setup(const char *args)
{
    unsigned int value = 0;

    args = parse((char *)(++args), "d", &value);
    if (!args || value < 0 || value > 7)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_new_ldo((unsigned char)value);

    return AT_RESULT_OK;
}

#ifdef _SUBFRAME_MODE_
static at_result_t at_wiotasubfdata_setup(const char *args)
{
    int length = 0;
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;
    uc_subf_data_t subf_data = {0};

    WIOTA_MUST_RUN(wiota_state)

    args = parse((char *)(++args), "d", &length);
    if (!args || length <= 0)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    sendbuffer = (unsigned char *)rt_malloc(length);
    if (sendbuffer == NULL || length < 2)
    {
        rt_free(sendbuffer);
        return AT_RESULT_NULL;
    }
    psendbuffer = sendbuffer;
    at_server_printf(">");

    while (length)
    {
        if (get_char_timeout(rt_tick_from_millisecond(WIOTA_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
        {
            rt_free(sendbuffer);
            return AT_RESULT_NULL;
        }
        length--;
        psendbuffer++;
    }

    subf_data.data = sendbuffer;
    subf_data.data_len = psendbuffer - sendbuffer;

    if (uc_wiota_add_subframe_data(&subf_data))
    {
        rt_free(sendbuffer);
        return AT_RESULT_OK;
    }

    rt_free(sendbuffer);
    return AT_RESULT_NULL;
}

static at_result_t at_wiotasubfreport_setup(const char *args)
{
    int mode = 0;

    WIOTA_MUST_ALREADY_INIT(wiota_state)

    args = parse((char *)(++args), "d", &mode);
    if (!args || mode < 0 || mode > 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if(uc_wiota_set_is_report_subf_data(mode))
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_PARSE_FAILE;
}

#endif


static at_result_t at_wiota_rf_ctrl_setup(const char *args)
{
    int ctrl = 0;

    args = parse((char *)(++args), "d", &ctrl);

    if (!args || ctrl < 0 || ctrl > 1)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_rf_ctrl_type(ctrl);

    return AT_RESULT_OK;
}

static at_result_t at_agc_adjust_set_setup(const char *args)
{
    unsigned int adjust = 0;
    unsigned int agc = 0;

    args = parse((char *)(++args), "d,d", &adjust, &agc);
    if (!args || adjust > 1 || agc > 15)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    uc_wiota_set_en_aagc_ajust((unsigned char)adjust);
    uc_wiota_set_init_agc((unsigned char)agc);

    return AT_RESULT_OK;
}

static at_result_t at_mem_addr_setup(const char *args)
{
    unsigned int mem_addr = 0;
    unsigned int value = 0;

    args = parse((char *)(++args), "y,y", &mem_addr, &value);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }
    if (uc_wiota_mem_addr_value(mem_addr, value))
    {
        return AT_RESULT_OK;
    }
    return AT_RESULT_FAILE;
}


AT_CMD_EXPORT("AT+WIOTAVERSION", RT_NULL, RT_NULL, at_wiota_version_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAINIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_init_exec);
AT_CMD_EXPORT("AT+WIOTALPM", "=<mode>,<v1>,<v2>", RT_NULL, RT_NULL, at_wiotalpm_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARATE", "=<mode>,<v>", RT_NULL, at_wiotarate_query, at_wiotarate_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPOW", "=<mode>,<pow>", RT_NULL, RT_NULL, at_wiotapow_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFREQ", "=<freq>", RT_NULL, at_freq_query, at_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADCXO", "=<dcxo>", RT_NULL, at_dcxo_query, at_dcxo_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAUSERID", "=<id0>", RT_NULL, at_userid_query, at_userid_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARADIO", RT_NULL, RT_NULL, at_radio_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONFIG", "=<idlen>,<symbol>,<band>,<pz>,<bt>,<speci>,<sysid>,<subsysid>",
              RT_NULL, at_system_config_query, at_system_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFSPACING", "=<fspac>", RT_NULL, at_freq_spacing_query, at_freq_spacing_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBSYSID", "=<subsysid>", RT_NULL, at_subsys_id_query, at_subsys_id_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARUN", "=<state>", RT_NULL, RT_NULL, at_wiota_cfun_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASEND", "=<timeout>,<len>,<userId>", RT_NULL, RT_NULL, at_wiotasend_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASENDNOBK", "=<timeout>,<len>,<userId>", RT_NULL, RT_NULL, at_wiotasend_noblock_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATRANS", "=<timeout>,<end>", RT_NULL, RT_NULL, at_wiotatrans_setup, at_wiotatrans_exec);
// AT_CMD_EXPORT("AT+WIOTADTUSEND", "=<timeout>,<wait>,<end>", RT_NULL, RT_NULL, at_wiota_dtu_send_setup, at_wiota_dtu_send_exec);
AT_CMD_EXPORT("AT+WIOTARECV", "=<timeout>,<userId>", RT_NULL, RT_NULL, at_wiotarecv_setup, at_wiota_recv_exec);
AT_CMD_EXPORT("AT+WIOTALOG", "=<mode>", RT_NULL, RT_NULL, at_wiotalog_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASTATS", "=<mode>,<type>", RT_NULL, at_wiotastats_query, at_wiotastats_setup, RT_NULL);
// AT_CMD_EXPORT("AT+WIOTATHROUGHT", "=<mode>,<type>", RT_NULL, at_wiotathrought_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACRC", "=<climit>", RT_NULL, at_wiotacrc_query, at_wiotacrc_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAOSC", "=<mode>", RT_NULL, at_wiotaosc_query, at_wiotaosc_setup, RT_NULL);
#ifdef _LIGHT_PILOT_
AT_CMD_EXPORT("AT+WIOTALIGHT", "=<mode>", RT_NULL, RT_NULL, at_wiotalight_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTASAVESTATIC", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_save_static_exec);
AT_CMD_EXPORT("AT+WIOTASUBNUM", "=<number>", RT_NULL, at_wiota_subframe_num_query, at_wiota_subframe_num_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTABCROUND", "=<round>", RT_NULL, RT_NULL, at_wiota_bc_round_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTADETECTIME", "=<time>", RT_NULL, RT_NULL, at_wiota_detect_time_setup, RT_NULL);
// AT_CMD_EXPORT("AT+WIOTABAND", "=<band>", RT_NULL, RT_NULL, at_wiota_bandwidth_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACONTINUESEND", "=<flag>", RT_NULL, RT_NULL, at_wiota_continue_send_setup, RT_NULL);
#ifdef _SUBFRAME_MODE_
AT_CMD_EXPORT("AT+WIOTASUBFRAMEMODE", "=<mode>, <flag>", RT_NULL, RT_NULL, at_wiota_subframe_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBFDATA", "=<len>", RT_NULL, RT_NULL, at_wiotasubfdata_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASUBFREP", "=<mode>", RT_NULL, RT_NULL, at_wiotasubfreport_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTAINCRECV", "=<flag>", RT_NULL, RT_NULL, at_wiota_incomplete_recv_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTATXMODE", "=<mode>", RT_NULL, RT_NULL, at_wiotatxmode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARECVMODE", "=<mode>", RT_NULL, RT_NULL, at_wiotarecvmode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAFRAMEINFO", RT_NULL, RT_NULL, at_wiotaframeinfo_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASTATE", RT_NULL, RT_NULL, at_wiotastate_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAUNIFAIL", "=<cnt>", RT_NULL, RT_NULL, at_wiotaunifailcnt_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANFREQ", "=<timeout>,<round>,<dataLen>,<freqnum>", RT_NULL, RT_NULL, at_scan_freq_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTASCANSORT", "=<mode>", RT_NULL, RT_NULL, at_scan_sort_setup, RT_NULL);
#ifdef _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAPAGINGTX", "=<freq>,<spec_idx>,<band>,<symbol>,<awaken_id>,<send_time>",
              RT_NULL, at_paging_tx_config_query, at_paging_tx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGRX", "=<freq>,<spec_idx>,<band>,<symbol>,<awaken_id>,<detect_period>,<nlen>,<utimes>,<thres>,<extra_flag>,<extra_period>",
              RT_NULL, at_paging_rx_config_query, at_paging_rx_config_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGRXANO", "=<period_multiple>,<awaken_id_another>",
              RT_NULL, at_paging_rx_config_another_query, at_paging_rx_config_another_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAPAGINGMODE", "=<rx_mode>,<tx_mode>",
              RT_NULL, at_paging_config_mode_query, at_paging_config_mode_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTASYMBMODE", "=<symbol_mode>", RT_NULL, RT_NULL, at_wiota_symbol_mode_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACKMEM", "=<total>,<used>,<maxused>", RT_NULL, at_memory_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+WIOTACTL", "=<mode>", RT_NULL, RT_NULL, at_wiota_control_mode_setup, RT_NULL);
#ifdef _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAWAKEN", RT_NULL, RT_NULL, at_wiotawaken_query, RT_NULL, RT_NULL);
#endif
#ifdef _SUBFRAME_MODE_
AT_CMD_EXPORT("AT+WIOTATWNUM", "=<number1>, <number2>", RT_NULL, RT_NULL, at_wiota_two_way_subfnum_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTACKINFO", "=<is_info>, <info>", RT_NULL, RT_NULL, at_wiota_uni_ack_info_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAOUTERK", "=<mode>", RT_NULL, RT_NULL, at_wiota32k_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTARESETRF", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_wiota_reset_rf_exec);
AT_CMD_EXPORT("AT+WIOTAMODULEID", "=<modulid>", RT_NULL, at_module_id_query, RT_NULL, RT_NULL);
#ifdef _LPM_PAGING_
AT_CMD_EXPORT("AT+WIOTAEMPT", "=<flag>", RT_NULL, RT_NULL, at_wiota_embed_pgtx_setup, RT_NULL);
#endif
AT_CMD_EXPORT("AT+WIOTADCDJ", "=<mode>", RT_NULL, at_wiota_adc_adj_query, at_wiota_adc_adj_setup, RT_NULL);

#ifdef _QUICK_CONNECT_
AT_CMD_EXPORT("AT+WIOTAQC", "=<onoff>,<freq>,<mode>", RT_NULL, RT_NULL, at_wiota_quick_start, RT_NULL);
#endif

AT_CMD_EXPORT("AT+WIOTASLIST", "=<band>,<value>", RT_NULL, RT_NULL, at_search_list_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTALDO", "=<value>", RT_NULL, RT_NULL, at_new_ldo_setup, RT_NULL);

AT_CMD_EXPORT("AT+WIOTARFCTRL", "=<ctrl>", RT_NULL, RT_NULL, at_wiota_rf_ctrl_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAAGC", "=<adjust>,<agc>", RT_NULL, RT_NULL, at_agc_adjust_set_setup, RT_NULL);
AT_CMD_EXPORT("AT+WIOTAMEMADDR", "=<addr>,<v>", RT_NULL, RT_NULL, at_mem_addr_setup, RT_NULL);

#endif
#endif
#endif
