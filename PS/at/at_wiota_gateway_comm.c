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
#include "at.h"
#include "ati_prs.h"
#ifdef WIOTA_ASYNC_GATEWAY_AT
#include "uc_wiota_gateway_api.h"

#define COMM_WAIT_DATA_TIMEOUT   10000
#define COMM_SEND_TIMEOUT        60000

extern at_server_t at_get_server(void);
static rt_err_t at_comm_get_char_timeout(rt_tick_t timeout, char *chr)
{
    at_server_t at_server = at_get_server();
    return at_server->get_char(at_server, chr, timeout);
}

static void at_comm_data_recv(unsigned int id, unsigned char type, void* data, int len)
{
    at_server_printf("+COMMRECV,0x%08x,%d,%d:", id, type, len);
    if (data != RT_NULL)
    {
        if (len > 0)
        {
            at_send_data(data, len);
        }
    }
    at_server_printfln("");
}

static at_result_t at_comm_init_setup(const char *args)
{
    int crc_flag = 0;
    int power = 0; 
    int order_scanf_freq = 0;
    int default_freq = 0;
    int broadcast_send_counter = 0; 
    int continue_mode = 0; 
    int send_detect_time = 0;
    int unicast_send_counter = 0;

    args = parse((char *)(++args), "d,d,d,d,d,d,d,d", &crc_flag, &power, 
                    &order_scanf_freq, &default_freq, 
                    &broadcast_send_counter, 
                    &continue_mode, 
                    &send_detect_time, 
                    &unicast_send_counter);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    if (UC_GATEWAY_OK != uc_wiota_gateway_init((unsigned char)crc_flag, 
        (unsigned char)power, 
        (unsigned char)order_scanf_freq,
        (unsigned char)default_freq,
        (unsigned char)broadcast_send_counter,  
        (UC_GATEWAY_CONTINUE_MODE_TYPE)continue_mode, 
        (unsigned char)send_detect_time, 
        (unsigned char)unicast_send_counter))
    {
        return AT_RESULT_FAILE;
    }

    uc_wiota_gateway_set_recv_register(at_comm_data_recv);

    return AT_RESULT_OK;
}

static at_result_t at_comm_exit_exec(void)
{
    if (uc_wiota_gateway_exit() == 0)
    {
        uc_wiota_gateway_set_recv_register(RT_NULL);
        return AT_RESULT_OK;
    }
    else
    {
        return AT_RESULT_FAILE;
    }
}

static at_result_t at_comm_freq_query(void)
{
    at_server_printfln("+COMMFREQ:%d", uc_wiota_get_freq_num());

    return AT_RESULT_OK;
}

static at_result_t at_comm_gwid_query(void)
{
    at_server_printfln("+COMMGWID:0x%08x", uc_wiota_get_gateway_id());

    return AT_RESULT_OK;
}

static at_result_t at_comm_state_query(void)
{
    at_server_printfln("+COMMSTATE:%d", uc_wiota_gateway_get_state());

    return AT_RESULT_OK;
}

static at_result_t at_comm_send_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;
    unsigned int userId = 0;
    unsigned int timeout_u = 0;

    args = parse((char *)(++args), "y,d,d", &userId, &length, &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    timeout_u = (unsigned int)timeout;

    if (length > 0)
    {
        sendbuffer = (unsigned char *)rt_malloc(length + 4); // reserve CRC16_LEN for low mac
        if (sendbuffer == NULL)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_FAILE;
        }
        psendbuffer = sendbuffer;
        //at_server_printfln("SUCC");
        at_server_printf(">");

        while (length)
        {
            if (at_comm_get_char_timeout(rt_tick_from_millisecond(COMM_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
            {
                at_server_printfln("SEND FAIL");
                rt_free(sendbuffer);
                return AT_RESULT_FAILE;
            }
            length--;
            psendbuffer++;
        }

        if (0 == uc_wiota_gateway_send(userId, sendbuffer, psendbuffer - sendbuffer, timeout_u > 0 ? timeout_u :COMM_SEND_TIMEOUT, RT_NULL))
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;

            at_server_printfln("SEND SUCC");
            // at_server_printfln("+dcxo,%d,temp,%d", state_get_dcxo_idx(),l1_adc_temperature_read());
            return AT_RESULT_OK;
        }
        else
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;
            at_server_printfln("SEND FAIL");
            // at_server_printfln("+dcxo,%d,temp,%d", state_get_dcxo_idx(),l1_adc_temperature_read());
            return AT_RESULT_FAILE;
        }
    }

    return AT_RESULT_OK;
}

static at_result_t at_comm_send_gw_setup(const char *args)
{
    int length = 0, timeout = 0;
    unsigned char *sendbuffer = NULL;
    unsigned char *psendbuffer;
    unsigned int timeout_u = 0;

    args = parse((char *)(++args), "d,d", &length, &timeout);
    if (!args)
    {
        return AT_RESULT_PARSE_FAILE;
    }

    timeout_u = (unsigned int)timeout;

    if (length > 0)
    {
        sendbuffer = (unsigned char *)rt_malloc(length + 4); // reserve CRC16_LEN for low mac
        if (sendbuffer == NULL)
        {
            at_server_printfln("SEND FAIL");
            return AT_RESULT_FAILE;
        }
        psendbuffer = sendbuffer;
        //at_server_printfln("SUCC");
        at_server_printf(">");

        while (length)
        {
            if (at_comm_get_char_timeout(rt_tick_from_millisecond(COMM_WAIT_DATA_TIMEOUT), (char *)psendbuffer) != RT_EOK)
            {
                at_server_printfln("SEND FAIL");
                rt_free(sendbuffer);
                return AT_RESULT_FAILE;
            }
            length--;
            psendbuffer++;
        }

        if (0 == uc_wiota_gateway_send(uc_wiota_get_gateway_id(), sendbuffer, psendbuffer - sendbuffer, timeout_u > 0 ? timeout_u :COMM_SEND_TIMEOUT, RT_NULL))
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;

            at_server_printfln("SEND SUCC");
            // at_server_printfln("+dcxo,%d,temp,%d", state_get_dcxo_idx(),l1_adc_temperature_read());
            return AT_RESULT_OK;
        }
        else
        {
            rt_free(sendbuffer);
            sendbuffer = NULL;
            at_server_printfln("SEND FAIL");
            // at_server_printfln("+dcxo,%d,temp,%d", state_get_dcxo_idx(),l1_adc_temperature_read());
            return AT_RESULT_FAILE;
        }
    }

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+COMMINIT", "=<crc>,<power>,<scan_freq>,<default_freq>,<broadcast_send_counter>,<continue_mode>,<send_detect_time>,<unicast_send_counter>", RT_NULL, RT_NULL, at_comm_init_setup, RT_NULL);
AT_CMD_EXPORT("AT+COMMEXIT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_comm_exit_exec);
AT_CMD_EXPORT("AT+COMMFREQ", "=<freq_num>", RT_NULL, at_comm_freq_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+COMMGWID", "=<gw_id>", RT_NULL, at_comm_gwid_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+COMMSTATE", "=<state>", RT_NULL, at_comm_state_query, RT_NULL, RT_NULL);
AT_CMD_EXPORT("AT+COMMSEND", "=<id>,<len>,<timeout>", RT_NULL, RT_NULL, at_comm_send_setup, RT_NULL);
AT_CMD_EXPORT("AT+COMMSENDGW", "=<len>,<timeout>", RT_NULL, RT_NULL, at_comm_send_gw_setup, RT_NULL);

#endif
#endif
#endif
#endif
