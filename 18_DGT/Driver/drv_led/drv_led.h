#ifndef __DRV_LED_H
#define __DRV_LED_H
#include "config.h"

#define	LED1	0
#define	LED2	1
#define	LED3	2

/**
***********************************************************
* @brief  led初始化
* @param  无
* @return 无
***********************************************************
*/
void drv_led_init(void);

/**
***********************************************************
* @brief  点亮LED
* @param  led_id LED_ID  (0-2)
* @return 无
***********************************************************
*/
void drv_led_on(uint8_t led_id);

/**
***********************************************************
* @brief  熄灭LED
* @param  led_id LED_ID  (0-2)
* @return 无
***********************************************************
*/
void drv_led_off(uint8_t led_id);

/**
***********************************************************
* @brief  翻转LED
* @param  led_id LED_ID  (0-2)
* @return 无
***********************************************************
*/
void drv_led_toggle(uint8_t led_id);
#endif /* __DRV_LED_H */

