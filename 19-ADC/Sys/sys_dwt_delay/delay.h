/**
 ******************************************************************************
 * @file    delay.h
 * @brief   DWT 延时驱动头文件
 * @details 基于 Cortex-M4 DWT CYCCNT 的高精度延时函数
 ******************************************************************************
 */

#ifndef __DELAY_H
#define __DELAY_H

#include "config.h"

/**
 * @brief  DWT 延时模块初始化函数
 * @param  无
 * @retval 无
 * @note
 *   必须在使用 delay_us() / delay_ms() 之前调用此函数初始化
 */
void delay_init(void);

/**
 * @brief  微秒级延时函数
 * @param  us : 延时时间（单位：us）
 * @retval 无
 */
void delay_us(uint32_t us);

/**
 * @brief  毫秒级延时函数
 * @param  ms : 延时时间（单位：ms）
 * @retval 无
 */
void delay_ms(uint32_t ms);

#endif /* __DELAY_H */
