
#ifdef _QUICK_CONNECT_

#ifndef __QUICK_CONNECT_H__
#define __QUICK_CONNECT_H__

typedef enum
{
    QC_MODE_SHORT_DIS_HIGH_RATE     =0, //短距高速率/UCM200 / UCM200T
    QC_MODE_SHORT_DIS_THROUGH       =1, //短距强穿透/UCM200 / UCM200T
    QC_MODE_MID_DIS_HIGH_RATE       =2, //中距高速率/UCM200 / UCM200T
    QC_MODE_MID_DIS_THROUGH         =3, //中距强穿透/UCM200 / UCM200T
    QC_MODE_LONG_DIS                =4, //远距离通信//室内多层楼覆盖，室外公里级覆盖/UCM200 /UCM200T
    QC_MODE_GREAT_DIS               =5, //超远距离通信//最远通信距离，传输时间较长/UCM200 /UCM200T
    QC_MODE_MAX,
} e_qc_mode;

typedef struct
{
    unsigned char symbol_len;   //符号长度
    unsigned char band;         //带宽
    unsigned char subnum;       //子帧数
    unsigned char mcs;          //速率级别
    signed char up_pow;     //上行发射功率

}s_qc_cfg;


typedef enum
{
    QC_DEF = 0,
    QC_INIT,
    QC_RUN,
    QC_EXIT,
} e_qc_state;


int wiota_quick_connect_start(unsigned short freq,e_qc_mode mode);

int wiota_quick_connect_stop(void);

void clr_qc_auto_run(void);


int quick_connect_task_init(void);

#endif
#endif
