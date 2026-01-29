/**
 ******************************************************************************
 * @file    drv_dgt.h
 * @brief   DGT (看门狗) 驱动头文件
 * @details 用于 GD32F303 的独立看门狗驱动，防止程序跑飞
 ******************************************************************************
 */

#ifndef __DRV_DGT_H
#define __DRV_DGT_H

#include "config.h"

/* 看门狗超时时间定义 */
#define DGT_TIMEOUT_MS    2000     /* 看门狗超时时间 2000ms */

/**
 * @brief  独立看门狗模块初始化函数
 * @param  无
 * @retval 无
 * @note
 *   配置看门狗超时时间为 2000ms，之后必须定期喂狗
 */
void drv_dgt_init(void);

/**
 * @brief  喂狗函数
 * @param  无
 * @retval 无
 * @note
 *   必须在看门狗超时之前（2000ms）调用此函数，否则系统会复位
 *   建议在主循环中定期调用
 */
void feed_dog(void);

#endif /* __DRV_DGT_H */
