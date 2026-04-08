/**
 ******************************************************************************
 * @file    app_key.c
 * @brief   KEY 应用层源文件
 * @details 基于 KEY 驱动的应用层，处理按键事件和 RTC 时间显示
 ******************************************************************************
 */

#include "app_key/app_key.h"
#include "drv_key/drv_key.h"
#include "drv_rtc/drv_rtc.h"

/**
 * @brief  KEY 任务处理函数
 * @param  无
 * @retval 无
 */
void app_key_task(void)
{
    uint8_t key_val;

    key_val = drv_get_key_val();

    if (key_val == KEY_NULL_PRESS)
    {
        return; // 没有按键，直接返回
    }

    switch (key_val)
    {
        /* ---------- 单击 ---------- */
        case KEY1_SHORT_PRESS:
            printf("[KEY] KEY1 SHORT PRESS\r\n");
            break;
        case KEY2_SHORT_PRESS:
            printf("[KEY] KEY2 SHORT PRESS\r\n");
            break;
        case KEY3_SHORT_PRESS:
            printf("[KEY] KEY3 SHORT PRESS\r\n");
            break;
        case KEY4_SHORT_PRESS:
            printf("[KEY] KEY4 SHORT PRESS\r\n");
            break;

        /* ---------- 双击 ---------- */
        case KEY1_DOUBLE_PRESS:
            printf("[KEY] KEY1 DOUBLE PRESS\r\n");
            break;
        case KEY2_DOUBLE_PRESS:
            printf("[KEY] KEY2 DOUBLE PRESS\r\n");
            break;
        case KEY3_DOUBLE_PRESS:
            printf("[KEY] KEY3 DOUBLE PRESS\r\n");
            break;
        case KEY4_DOUBLE_PRESS:
            printf("[KEY] KEY4 DOUBLE PRESS\r\n");
            break;

        /* ---------- 长按 ---------- */
        case KEY1_LONG_PRESS:
            printf("[KEY] KEY1 LONG PRESS\r\n");
            break;
        case KEY2_LONG_PRESS:
            printf("[KEY] KEY2 LONG PRESS\r\n");
            break;
        case KEY3_LONG_PRESS:
            printf("[KEY] KEY3 LONG PRESS\r\n");
            break;
        case KEY4_LONG_PRESS:
            printf("[KEY] KEY4 LONG PRESS\r\n");
            break;

        /* ---------- 错误 ---------- */
        case KEY_ERROR_PRESS:
            printf("[KEY] ERROR PRESS\r\n");
            break;

        default:
            printf("[KEY] UNKNOWN VALUE: 0x%02X\r\n", key_val);
            break;
    }
}

/**
 * @brief  RTC 任务处理函数
 * @note
 *   读取 RTC 时间并通过串口打印
 * @param  无
 * @retval 无
 */
void rtc_task(void)
{
    rtc_time_t rtc_time;
    drv_get_rtc_time(&rtc_time);
    printf("%04d-%02d-%02d %02d:%02d:%02d\r\n", rtc_time.year, rtc_time.month, rtc_time.day,
                                                rtc_time.hour, rtc_time.minute, rtc_time.second);
}
