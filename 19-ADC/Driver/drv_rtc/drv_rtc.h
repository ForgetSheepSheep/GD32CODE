/**
 ******************************************************************************
 * @file    drv_rtc.h
 * @brief   RTC 驱动头文件
 * @details 用于 GD32F303 的 RTC 实时时钟驱动，支持日期和时间的读写
 ******************************************************************************
 */

#ifndef __DRV_RTC_H
#define __DRV_RTC_H

#include "config.h"

/* BKP 备份寄存器标志码 */
#define MAGIC_CODE  0x5A5A

/**
 * @brief  RTC 时间结构体
 * @note
 *   年月日时分秒，BCD 格式或二进制格式均可
 */
typedef struct
{
    uint16_t year;    /* 年份，如 2023 */
    uint16_t month;   /* 月份，1-12 */
    uint8_t  day;     /* 日期，1-31 */
    uint8_t  hour;    /* 小时，0-23 */
    uint8_t  minute;  /* 分钟，0-59 */
    uint8_t  second;  /* 秒，0-59 */
} rtc_time_t;

/**
 * @brief  RTC 硬件初始化函数
 * @param  无
 * @retval 无
 * @note
 *   该函数在上电后若备份寄存器数据丢失时才初始化 RTC
 *   初始化完成后写入 MAGIC_CODE，保证下次复位不再重新初始化
 */
void drv_rtc_init(void);

/**
 * @brief  获取 RTC 时间
 * @param  time : 指向时间结构体的指针，用于存储读取到的时间
 * @retval 无
 * @note
 *   从硬件读取 UTC 时间，自动转换为北京时间
 */
void drv_get_rtc_time(rtc_time_t *time);

/**
 * @brief  设置 RTC 时间
 * @param  time : 指向时间结构体的指针，包含要设置的时间
 * @retval 无
 * @note
 *   设置时间为北京时间，自动转换为 UTC 时间存储
 */
void drv_set_rtc_time(rtc_time_t *time);

#endif /* __DRV_RTC_H */
