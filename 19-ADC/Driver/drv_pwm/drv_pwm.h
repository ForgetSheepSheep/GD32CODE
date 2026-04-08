/**
 ******************************************************************************
 * @file    drv_pwm.h
 * @brief   PWM 驱动头文件
 * @details 用于 GD32F303 的 PWM 输出驱动，配置 TIMER0_CH0 输出 PWM 信号
 ******************************************************************************
 */

#ifndef __DRV_PWM_H
#define __DRV_PWM_H

#include "config.h"

/* PWM 参数定义 */
#define PWM_FREQUENCY     2000      /* PWM 频率 2kHz */
#define PWM_PERIOD       500        /* PWM 周期 500us */
#define PWM_PRESCALER    120        /* 预分频 120 (120MHz/120 = 1MHz) */

/**
 * @brief  PWM 模块初始化函数
 * @param  无
 * @retval 无
 */
void drv_pwm_init(void);

/**
 * @brief  LED PWM 测试函数
 * @note   逐渐增加和减少 PWM 占空比，实现呼吸灯效果
 * @param  无
 * @retval 无
 */
void led_pwm_test(void);

#endif /* __DRV_PWM_H */
