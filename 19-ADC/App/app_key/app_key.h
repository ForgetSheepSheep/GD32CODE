/**
 ******************************************************************************
 * @file    app_key.h
 * @brief   KEY 应用层头文件
 * @details 基于 KEY 驱动的应用层，处理按键事件和 RTC 时间显示
 ******************************************************************************
 */

#ifndef __APP_KEY_H
#define __APP_KEY_H

#include "config.h"

/**
 * @brief  KEY 任务处理函数
 * @param  无
 * @retval 无
 */
void app_key_task(void);

/**
 * @brief  RTC 任务处理函数
 * @note
 *   读取 RTC 时间并通过串口打印
 * @param  无
 * @retval 无
 */
void rtc_task(void);

#endif /* __APP_KEY_H */
