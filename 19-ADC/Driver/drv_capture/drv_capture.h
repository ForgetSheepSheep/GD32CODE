/**
 ******************************************************************************
 * @file    drv_capture.h
 * @brief   CAPTURE (输入捕获) 驱动头文件
 * @details 用于 GD32F303 的 TIMER 输入捕获驱动，测量 PWM 周期和占空比
 ******************************************************************************
 */

#ifndef __DRV_CAPTURE_H
#define __DRV_CAPTURE_H

#include "config.h"

/**
 * @brief  输入捕获模块初始化函数
 * @param  无
 * @retval 无
 */
void drv_capture_init(void);

/**
 * @brief  输入捕获测试函数
 * @param  无
 * @retval 无
 * @note
 *   打印捕获到的 PWM 周期和占空比（单位：us）
 */
void capture_test(void);

#endif /* __DRV_CAPTURE_H */
