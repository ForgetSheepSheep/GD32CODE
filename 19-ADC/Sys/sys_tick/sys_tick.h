/**
 ******************************************************************************
 * @file    sys_tick.h
 * @brief   SysTick 系统滴答定时器驱动头文件
 * @details 基于 Cortex-M SysTick 的 1ms 定时器，提供系统运行时间
 ******************************************************************************
 */

#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include "config.h"

/**
 * @brief  SysTick 系统滴答定时器初始化函数
 * @note
 *   配置为 1ms 中断一次，用于记录系统运行时间
 * @param  无
 * @retval 无
 */
void sys_tick_init(void);

/**
 * @brief  获取系统运行时间
 * @param  无
 * @retval 系统运行时间（单位：ms）
 * @note
 *   该函数返回从系统启动以来经过的毫秒数
 *   使用 uint64_t 存储，可支持约 584 年不溢出
 */
uint64_t sys_tick_get_runtime(void);

/**
 * @brief  注册 SysTick 中断回调函数
 * @param  func : 回调函数指针
 * @retval 无
 * @note
 *   该函数会在每次 SysTick 中断（1ms）中被调用
 *   注意不要在回调函数中执行耗时操作
 */
void sys_callback(void (*func)(void));

#endif /* __SYS_TICK_H */
