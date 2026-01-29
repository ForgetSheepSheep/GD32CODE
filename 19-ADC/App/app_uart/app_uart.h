/**
 ******************************************************************************
 * @file    app_uart.h
 * @brief   UART 应用层头文件
 * @details 基于 UART 驱动的应用层协议解析和 LED 控制
 ******************************************************************************
 */

#ifndef __APP_UART_H
#define __APP_UART_H

#include "config.h"

/**
 * @brief  UART APP 任务处理函数
 * @note
 *   从队列中取出数据，进行协议解析、校验、处理命令
 * @param  无
 * @retval 无
 */
void app_uart_task(void);

/**
 * @brief  UART APP 初始化函数
 * @note
 *   注册 UART 接收回调函数
 *   初始化接收队列
 * @param  无
 * @retval 无
 */
void app_uart_init(void);

#endif /* __APP_UART_H */
