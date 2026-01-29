/**
 ******************************************************************************
 * @file    drv_adc.c
 * @brief   ADC 驱动源文件
 * @details 用于 GD32F303 的 ADC 模块驱动，支持滑动电阻电压采集
 ******************************************************************************
 */

#include "drv_adc/drv_adc.h"
#include "sys_dwt_delay/delay.h"

/* 私有函数声明 */
static void GpioInit(void);
static void AdcInit(void);

/**
 * @brief  ADC 模块初始化
 * @note   调用 GPIO 初始化和 ADC 初始化
 * @param  无
 * @retval 无
 */
void drv_adc_init(void)
{
    GpioInit();
    AdcInit();
}

/**
 * @brief  GPIO 初始化函数
 * @note   配置 PC2 为模拟输入模式
 * @param  无
 * @retval 无
 */
static void GpioInit(void)
{
    /* 使能 GPIOC 时钟 */
    rcu_periph_clock_enable(RCU_GPIOC);
    /* 配置 PC2 为模拟输入模式，速度 10MHz */
    gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, ADC_PIN);
}

/**
 * @brief  ADC 模块初始化函数
 * @note   配置 ADC0 为连续转换模式，通道12，软件触发
 * @param  无
 * @retval 无
 */
static void AdcInit(void)
{
    /* 使能 ADC0 时钟 */
    rcu_periph_clock_enable(RCU_ADC0);

    /* 设置 ADC 时钟分频系数：AHB/6 = 120MHz/6 = 20MHz */
    rcu_adc_clock_config(RCU_CKADC_CKAHB_DIV6);

    /* 设置 ADC 工作模式：独立模式 */
    adc_mode_config(ADC_MODE_FREE);

    /* 使能连续转换模式：一次触发后持续转换 */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);

    /* 设置数据对齐方式：右对齐 */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* 设置规则组通道数量：1 个通道 */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);

    /* 配置规则组通道：通道12，采样时间239.5周期，序列位置0 */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL, ADC_SAMPLETIME_239POINT5);

    /* 设置外部触发源：无（使用软件触发） */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);

    /* 使能外部触发（配合软件触发使用） */
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

    /* 使能 ADC 模块 */
    adc_enable(ADC0);

    /* 延时等待 ADC 稳定 */
    delay_us(50);

    /* 执行 ADC 校准 */
    adc_calibration_enable(ADC0);

    /* 启动软件触发（连续模式下只需触发一次） */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/**
 * @brief  获取 ADC 转换值
 * @note   等待 EOC 标志并返回最新的 ADC 转换结果
 * @param  无
 * @retval ADC 转换值 (0-4095)
 */
uint16_t drv_get_adcval(void)
{
    /* 等待 ADC 转换完成标志 (EOC) */
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC));

    /* 读取 ADC 规则组数据 */
    return adc_regular_data_read(ADC0);
}

/**
 * @brief  ADC 测试函数
 * @note   读取 ADC 值并计算对应的电压值，通过串口输出
 * @param  无
 * @retval 无
 */
void adc_test(void)
{
    uint16_t adc_val;
    float voltage;

    /* 获取 ADC 转换值 */
    adc_val = drv_get_adcval();

    /* 计算对应的电压值：Voltage = ADC_VAL / 4095 * 3.3V */
    voltage = (float)adc_val / ADC_RESOLUTION * ADC_VREF;

    /* 串口输出 ADC 值和电压值 */
    printf("AdcVal = %d, Voltage = %.3fV\r\n", adc_val, voltage);

    /* 延时 1 秒 */
    delay_us(1000000);
}
