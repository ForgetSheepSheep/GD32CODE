#ifndef __DRV_KEY_H
#define __DRV_KEY_H
#include "config.h"

/* 返回值约定
 * 0x00               : 无按键事件
 * 0x01 ~ 0x04        : 按键1~4 单击
 * 0x51 ~ 0x54        : 按键1~4 双击
 * 0x81 ~ 0x84        : 按键1~4 长按
 * 0xFF               : 参数错误
 */


/* ---------- 单击 ---------- */
#define KEY1_SHORT_PRESS   0x01
#define KEY2_SHORT_PRESS   0x02
#define KEY3_SHORT_PRESS   0x03
#define KEY4_SHORT_PRESS   0x04

/* ---------- 双击 ---------- */
#define KEY1_DOUBLE_PRESS  0x51
#define KEY2_DOUBLE_PRESS  0x52
#define KEY3_DOUBLE_PRESS  0x53
#define KEY4_DOUBLE_PRESS  0x54

/* ---------- 长按 ---------- */
#define KEY1_LONG_PRESS    0x81
#define KEY2_LONG_PRESS    0x82
#define KEY3_LONG_PRESS    0x83
#define KEY4_LONG_PRESS    0x84

/* ---------- 其他 ---------- */
#define KEY_NULL_PRESS     0x00
#define KEY_ERROR_PRESS    0xFF
void drv_key_init(void);
/**
***********************************************************
* @brief  获取按键码值
* @param  无
* @return res 按键码值
***********************************************************
*/
uint8_t drv_get_key_val(void);
#endif /* __DRV_KEY_H */
