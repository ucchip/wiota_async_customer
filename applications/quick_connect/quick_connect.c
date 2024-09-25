#include <rtthread.h>
#ifdef _QUICK_CONNECT_

#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include "uc_wiota_api.h"
#include "uc_wiota_static.h"
#ifdef RT_USING_AT
#include <at.h>
#include "ati_prs.h"
#endif
#include "uc_string_lib.h"
#include "uc_adda.h"
#include "uc_uart.h"
#include "at_wiota.h"
#include "quick_connect.h"

static rt_mutex_t p_mutex_qc_state = NULL;
static rt_mutex_t p_mutex_qc_run = NULL;

void quick_connect_init(void)
{
    p_mutex_qc_state = rt_mutex_create("qs", RT_IPC_FLAG_FIFO);

    if (p_mutex_qc_state == NULL)
    {
        return;
    }
    p_mutex_qc_run = rt_mutex_create("qr", RT_IPC_FLAG_FIFO);

    if (p_mutex_qc_run == NULL)
    {
        return;
    }
}

static e_qc_state qc_wiota_state = QC_EXIT;

void set_qc_wiota_state(e_qc_state state)
{
    rt_mutex_take(p_mutex_qc_state, RT_WAITING_FOREVER);

    qc_wiota_state = state;

    rt_mutex_release(p_mutex_qc_state);
}

e_qc_state get_qc_wiota_state(void)
{
    return qc_wiota_state;
}

static unsigned char qc_auto_run = 0; //默认不启动自动运行

void set_qc_auto_run(void)
{
    rt_mutex_take(p_mutex_qc_run, RT_WAITING_FOREVER);

    qc_auto_run = 1;

    rt_mutex_release(p_mutex_qc_run);
}

void clr_qc_auto_run(void)
{
    rt_mutex_take(p_mutex_qc_run, RT_WAITING_FOREVER);

    qc_auto_run = 0;

    rt_mutex_release(p_mutex_qc_run);
}

unsigned char get_qc_auto_run(void)
{
    return qc_auto_run;
}

//symbol_length：帧配置，取值0,1,2,3代表128,256,512,1024
/*band：带宽，
        1: 200K // 默认带宽
        2: 100K 
        3: 50K 
        4: 25K
*/
//subnum，设置每帧的子帧数
static s_qc_cfg qc_cfg[QC_MODE_MAX] =
    {
        //symbol,   band,   subnum,  msc,                up_pow,     ,tips
        {0, 1, 8, UC_MCS_LEVEL_3, (22 + 20)},
        {0, 4, 8, UC_MCS_LEVEL_0, (22 + 20)},
        {1, 1, 8, UC_MCS_LEVEL_2, (22 + 20)},
        {1, 4, 8, UC_MCS_LEVEL_0, (22 + 20)},
        {2, 4, 8, UC_MCS_LEVEL_0, (22 + 20)},
        {3, 4, 8, UC_MCS_LEVEL_0, (22 + 20)},

};

static e_qc_mode cur_mode = QC_MODE_MID_DIS_THROUGH;

int wiota_quick_connect_start(unsigned short freq, e_qc_mode mode)
{
    int ret = -1;
    sub_system_config_t config;

    if (mode >= QC_MODE_MAX)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    cur_mode = mode;

    clr_qc_auto_run();

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);

        rt_thread_mdelay(100);
    }

    //save parameter
    uc_wiota_get_system_config(&config);

    config.freq_idx = freq;

    config.symbol_length = qc_cfg[mode].symbol_len;

    config.bandwidth = qc_cfg[mode].band;

    uc_wiota_set_system_config(&config);

    uc_wiota_save_static_info();

    //use this api check freq range,when in api mode,
    //when set bandwidth,then check freq,freq is limited in range 200 when bandwith is 200k,in range 1600 when bandwidth is 25k
    if (uc_wiota_set_freq_info(freq) == FALSE)
    {
        return AT_RESULT_REPETITIVE_FAILE;
    }

    rt_kprintf("\r\nqs f=%d,m=%d,sl=%d,bw=%d,sn=%d,msc=%d,p=%d\r\n",
               freq, mode, config.symbol_length, config.bandwidth, qc_cfg[mode].subnum, qc_cfg[mode].mcs, qc_cfg[mode].up_pow-20);

#ifdef RT_USING_AT
    at_server_printfln("+QCSTART:%d,%d,%d,%d,%d,%d",
                       freq, config.symbol_length, config.bandwidth, qc_cfg[mode].subnum, qc_cfg[mode].mcs, qc_cfg[mode].up_pow-20);
#endif

    rt_thread_mdelay(50);

    set_qc_auto_run();

    ret = 0;

    return ret;
}

int wiota_quick_connect_stop(void)
{
    clr_qc_auto_run();

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);

        rt_thread_mdelay(100);
    }

    return 0;
}

static int quick_open_wiota(void)
{
    sub_system_config_t config;

    if (get_qc_wiota_state() != QC_EXIT)
    {
        uc_wiota_exit();
#ifdef RT_USING_AT
        at_wiota_set_state(AT_WIOTA_EXIT);
#endif

        set_qc_wiota_state(QC_EXIT);
    }

    rt_thread_mdelay(200);

    uc_wiota_init();

    set_qc_wiota_state(QC_INIT);

    uc_wiota_get_system_config(&config);

    uc_wiota_set_freq_info(config.freq_idx);

#ifdef RT_USING_AT
    at_server_printfln("+QCCONN:%d", config.freq_idx);
#endif

    uc_wiota_run();

#ifdef RT_USING_AT
    uc_wiota_register_recv_data_callback(wiota_recv_callback, UC_CALLBACK_NORAMAL_MSG);
    at_wiota_set_state(AT_WIOTA_RUN);
#endif

    uc_wiota_set_cur_power(qc_cfg[cur_mode].up_pow - 20);

    uc_wiota_set_data_rate(UC_RATE_NORMAL, qc_cfg[cur_mode].mcs);

    uc_wiota_set_subframe_num(qc_cfg[cur_mode].subnum);

    set_qc_wiota_state(QC_RUN);

    return 0;
}

static void quick_connect_task(void *argument)
{
    uc_wiota_status_e wiota_state = UC_STATUS_NULL;

    unsigned int cnt = 0;

    quick_connect_init();

    while (1)
    {
        cnt++;

        if (cnt % 10 == 0)
        {
            wiota_state = uc_wiota_get_state();

            if (((wiota_state == UC_STATUS_NULL) || (wiota_state == UC_STATUS_ERROR)) && (get_qc_auto_run() == 1))
            {
                quick_open_wiota();
            }

            cnt++; //防止重入
        }

        rt_thread_mdelay(100);
    }
}

int quick_connect_task_init(void)
{
    rt_thread_t tid = NULL;

    tid = rt_thread_create("qc",
                           quick_connect_task,
                           NULL,
                           1024,
                           RT_THREAD_PRIORITY_MAX - 1,
                           5);
    if (tid != NULL)
    {
        rt_thread_startup(tid);
    }
    return 0;
}

#endif