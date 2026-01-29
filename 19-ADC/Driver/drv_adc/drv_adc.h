/**
 ******************************************************************************
 * @file    drv_adc.h
 * @brief   ADC 驱动头文件
 * @details 用于 GD32F303 的 ADC 模块驱动，支持滑动电阻电压采集
 ******************************************************************************
 */

#ifndef __DRV_ADC_H
#define __DRV_ADC_H

#include "config.h"

/* ADC 参考电压和分辨率定义 */
#define ADC_VREF         3.3f      /* ADC 参考电压 3.3V */
#define ADC_RESOLUTION   4095.0f   /* ADC 分辨率 12-bit (2^12 - 1) */

/* ADC 引脚和通道配置 */
#define ADC_PIN          GPIO_PIN_2      /* ADC 引脚 PC2 */
#define ADC_GPIO_PORT    GPIOC           /* ADC GPIO 端口 */
#define ADC_CHANNEL      ADC_CHANNEL_12  /* ADC 通道 12 */

/**
 * @brief  ADC 初始化函数
 * @note   初始化 GPIO 和 ADC 模块，配置为连续转换模式
 * @param  无
 * @retval 无
 */
void drv_adc_init(void);

/**
 * @brief  获取 ADC 转换值
 * @note   等待 EOC 标志并返回最新的 ADC 转换结果
 * @param  无
 * @retval ADC 转换值 (0-4095)
 */
uint16_t drv_get_adcval(void);

/**
 * @brief  ADC 测试函数
 * @note   读取 ADC 值并计算对应的电压值，通过串口输出
 * @param  无
 * @retval 无
 */
void adc_test(void);

#endif /* __DRV_ADC_H */
