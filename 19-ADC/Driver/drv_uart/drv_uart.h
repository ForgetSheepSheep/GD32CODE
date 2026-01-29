/**
 ******************************************************************************
 * @file    drv_uart.h
 * @brief   UART 驱动头文件
 * @details 用于 GD32F303 的 UART 模块驱动，支持 printf 重定向和接收回调
 ******************************************************************************
 */

#ifndef __DRV_UART_H
#define __DRV_UART_H

#include "config.h"

/**
 * @brief  串口模块初始化函数
 * @param  baud_rate : 串口波特率，如 9600 / 115200
 * @retval 无
 * @note
 *   该函数完成 UART 相关的 GPIO 初始化和 UART 参数初始化
 */
void drv_uart_init(uint32_t baud_rate);

/**
 * @brief  发送单个字符，阻塞式
 * @param  ch : 要发送的字符
 * @retval 无
 * @note
 *   使用发送缓冲区空标志 TBE 判断是否可以发送下一个字节
 */
void uart_send_char(const char ch);

/**
 * @brief  发送字符串，以 '\0' 为结尾
 * @param  str : 字符串首地址
 * @retval 无
 * @note
 *   逐字节调用 uart_send_char 函数
 */
void uart_send_string(const char *str);

/**
 * @brief  注册/注销串口接收回调函数
 * @param  func : 回调函数指针
 *               func != NULL : 注册
 *               func == NULL : 注销
 * @retval 无
 * @note
 *   回调函数在 USART 中断服务程序中执行：
 *   - 注意不要在中断中执行耗时操作，例如 printf 等函数
 *   - 推荐只设置标志位，在主循环 task 中再处理
 */
void uart_callback(void (*func)(uint8_t));

#endif /* __DRV_UART_H */
