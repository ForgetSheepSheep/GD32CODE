/**
 ******************************************************************************
 * @file    drv_led.h
 * @brief   LED 驱动头文件
 * @details 用于 GD32F303 的 LED 模块驱动
 ******************************************************************************
 */

#ifndef __DRV_LED_H
#define __DRV_LED_H

#include "config.h"

/* LED 编号定义 */
#define LED1    0
#define LED2    1
#define LED3    2

/**
 * @brief  LED 模块初始化函数
 * @param  无
 * @retval 无
 */
void drv_led_init(void);

/**
 * @brief  点亮指定 LED
 * @param  led_id LED 编号 (0-2)
 * @retval 无
 */
void drv_led_on(uint8_t led_id);

/**
 * @brief  熄灭指定 LED
 * @param  led_id LED 编号 (0-2)
 * @retval 无
 */
void drv_led_off(uint8_t led_id);

/**
 * @brief  翻转指定 LED 状态
 * @param  led_id LED 编号 (0-2)
 * @retval 无
 */
void drv_led_toggle(uint8_t led_id);

#endif /* __DRV_LED_H */
