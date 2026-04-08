#include "app_uart/app_uart.h"
#include "drv_uart/drv_uart.h"
#include "drv_led/drv_led.h"

/*
 * ========================= UART 应用层协议说明 =========================
 *
 * 数据帧格式（共 7 字节）：
 *
 * ┌────────┬────────┬────────┬──────────┬──────────┬──────────┬────────┐
 * │  HEAD1 │  HEAD2 │  LEN   │  FUNC    │  DATA0   │  DATA1   │  XOR   │
 * ├────────┼────────┼────────┼──────────┼──────────┼──────────┼────────┤
 * │ 0x55   │ 0xAA   │   0x03 │  FUNC_ID │ payload  │ payload  │ 校验   │
 * └────────┴────────┴────────┴──────────┴──────────┴──────────┴────────┘
 *	 例：55 AA 03 06 02（led_num） 01 （led_state））
 * 说明：
 *  - HEAD1/HEAD2      : 帧头，用于同步
 *  - LEN              : 控制数据长度（本协议固定为 3）
 *  - FUNC             : 功能码（如 LED_CTRL_CODE）
 *  - DATA             : 功能数据（如 led_num / led_state）
 *  - XOR              : 前面所有字节异或校验（串口自己加）
 *
 * 接收流程：
 *  USART 中断 -> pro_uart_data() 字节组帧
 *              -> g_rc_succeed_flag = true
 *              -> app_uart_task() 校验 + 执行业务
 */

#define RX_BUF_SIZE        20      /* 接收缓冲区大小（大于最大帧长即可） */

#define DATA_HEAD_ONE      0x55    /* 帧头1 */
#define DATA_HEAD_TOW      0xAA    /* 帧头2 */

#define CTRL_DATA_LEN      3       /* 控制数据长度（FUNC + 2字节参数） */
#define PACKET_DATA_LEN    (CTRL_DATA_LEN + 4)
/* 4 = HEAD1 + HEAD2 + LEN + XOR */

#define FUNC_DATA_IDX      3       /* 功能码在帧中的索引位置 */
#define LED_CTRL_CODE      0x06    /* LED 控制功能码 */

/*
 * LED 控制数据结构
 * 对应 DATA 区内容：
 *   DATA0 -> led_num   : LED 编号
 *   DATA1 -> led_state : LED 状态（0=关，1=开）
 */
typedef struct 
{
    uint8_t led_num;     /* LED 编号 */
    uint8_t led_state;   /* LED 状态 */
} led_ctrl_inof_t;

static bool g_rc_succeed_flag = false; /* 一帧完整数据接收完成标志 */
static uint8_t g_rc_buf[RX_BUF_SIZE];  /* 接收数据缓冲区 */

/**
***********************************************************
* @brief  串口字节接收处理函数（组帧状态机）
* @param  dat : USART 中断接收到的 1 字节数据
* @return 无
* @note
*  1. 本函数在 USART 中断回调中被调用
*  2. 使用静态 index 实现简单的帧同步
*  3. 仅负责判断“是否接收到完整一帧”
***********************************************************
*/
static void pro_uart_data(uint8_t dat)
{
    static uint8_t index = 0;   /* 当前接收字节索引 */

    g_rc_buf[index++] = dat;    /* 按顺序存入缓冲区 */

    switch (index)
    {
        case 1:
            /* 第 1 字节：必须是帧头 0x55 */
            if (g_rc_buf[0] != DATA_HEAD_ONE)
            {
                index = 0;      /* 帧头错误，重新同步 */
            }
            break;

        case 2:
            /* 第 2 字节：必须是帧头 0xAA */
            if (g_rc_buf[1] != DATA_HEAD_TOW)
            {
                index = 0;      /* 帧头错误，重新同步 */
            }
            break;

        case 7:
            /* 已接收到完整一帧（固定 7 字节） */
            g_rc_succeed_flag = true; /* 通知 APP 层处理 */
            index = 0;                /* 为下一帧做准备 */
            break;

        default:
            /* 中间字节：不做处理，继续接收 */
            break;
    }
}

/**
***********************************************************
* @brief  异或校验计算
* @param  data : 数据指针
* @param  len  : 参与校验的字节长度
* @return 异或校验值
***********************************************************
*/
static uint8_t cal_xor_sum(const uint8_t *data, uint8_t len)
{
    uint8_t xor_sum = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        xor_sum ^= data[i];
    }

    return xor_sum;
}

/**
***********************************************************
* @brief  LED 控制执行函数
* @param  ctrldata : 指向 LED 控制数据区
* @return 无
* @note
*  led_state != 0 -> 开灯
*  led_state == 0 -> 关灯
***********************************************************
*/
static void clrt_led(led_ctrl_inof_t *ctrldata)
{
    (ctrldata->led_state != 0) ?
        drv_led_on(ctrldata->led_num) :
        drv_led_off(ctrldata->led_num);
}

/**
***********************************************************
* @brief  串口 APP 层任务处理函数
* @param  无
* @return 无
* @note
*  1. 必须放在主循环中周期性调用
*  2. 只在 g_rc_succeed_flag 置位后处理一帧数据
***********************************************************
*/
void app_uart_task(void)
{
    /* 没有完整帧，直接返回 */
    if (!g_rc_succeed_flag)
    {
        return;
    }

    /* 清除标志，避免重复处理 */
    g_rc_succeed_flag = false;

    /* 校验失败，丢弃该帧 */
    if (cal_xor_sum(g_rc_buf, PACKET_DATA_LEN - 1)
        != g_rc_buf[PACKET_DATA_LEN - 1])
    {
        return;
    }

    /* 根据功能码分发业务 */
    if (g_rc_buf[FUNC_DATA_IDX] == LED_CTRL_CODE)
    {
        clrt_led((led_ctrl_inof_t *)(&g_rc_buf[FUNC_DATA_IDX + 1]));
    }
}

/**
***********************************************************
* @brief  串口 APP 初始化函数
* @param  无
* @return 无
* @note
*  将 APP 层的字节处理函数注册给 UART 驱动
***********************************************************
*/
void app_uart_init(void)
{
    uart_callback(pro_uart_data);
}
