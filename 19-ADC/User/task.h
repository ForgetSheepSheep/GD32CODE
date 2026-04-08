/**
 ******************************************************************************
 * @file    task.h
 * @brief   任务调度系统头文件
 * @details 基于 SysTick 的简单任务调度系统
 ******************************************************************************
 */

#ifndef __TASK_H
#define __TASK_H

#include "config.h"

/**
 * @brief  任务系统初始化函数
 * @note
 *   注册 1ms tick 中断回调，用于任务调度
 * @param  无
 * @retval 无
 */
void task_init(void);

/**
 * @brief  任务调度主循环函数
 * @note
 *   在 main while(1) 中调用，遍历任务表并执行就绪的任务
 * @param  无
 * @retval 无
 */
void task_loop(void);

#endif /* __TASK_H */
