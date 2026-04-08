/**
 ******************************************************************************
 * @file    drv_adc.c
 * @brief   ADC 驱动源文件
 * @details 用于 GD32F303 的 ADC 模块驱动，支持滑动电阻电压采集
 ******************************************************************************
 */

#include "drv_adc_dma/drv_adc_dma.h"
#include "sys_dwt_delay/delay.h"

/* 私有函数声明 */
static void GpioInit(void);
static void AdcInit(void);
static void DmaInit(void);
/**
 * @brief  ADC 模块初始化
 * @note   调用 GPIO 初始化和 ADC 初始化
 * @param  无
 * @retval 无
 */
void drv_adc_dma_init(void)
{
    GpioInit();
    AdcInit();
	DmaInit();
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
	 gpio_init(ADC_GPIO_PORT, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_3);
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
	/* 设置连续模式；*/ 
	adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
	/* 设置扫描模式；*/ 
	adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);

    /* 设置数据对齐方式：右对齐 */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* 设置规则组通道数量：1 个通道 */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 2);

    /* 配置规则组通道：通道12，采样时间239.5周期，序列位置0 */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL, ADC_SAMPLETIME_239POINT5);
	adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_13, ADC_SAMPLETIME_239POINT5);
    /* 设置外部触发源：无（使用软件触发） */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);

    /* 使能外部触发（配合软件触发使用） */
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);


	adc_dma_mode_enable(ADC0);
	adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT, ADC_OVERSAMPLING_SHIFT_4B,ADC_OVERSAMPLING_RATIO_MUL16);
	adc_oversample_mode_enable(ADC0);
    /* 使能 ADC 模块 */
    adc_enable(ADC0);

    /* 延时等待 ADC 稳定 */
    delay_us(50);

    /* 执行 ADC 校准 */
    adc_calibration_enable(ADC0);

    /* 启动软件触发（连续模式下只需触发一次） */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}
#define ADC0_RDATA_ADDR		(ADC0 + 0x4C)
static uint16_t g_adcVal[2];
static void DmaInit(void)
{
	/* 使能DMA时钟；*/
	rcu_periph_clock_enable(RCU_DMA0);
	/* 复位DMA通道；*/
	dma_deinit(DMA0, DMA_CH0);
	
	dma_parameter_struct dmaStruct;
	dma_struct_para_init(&dmaStruct);
	/* 配置传输方向；*/ 
	dmaStruct.direction = DMA_PERIPHERAL_TO_MEMORY;
	/* 配置数据源地址；*/ 
	dmaStruct.periph_addr = ADC0_RDATA_ADDR;
	/* 配置源地址是固定的还是增长的；*/ 
	dmaStruct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	/* 配置源数据传输位宽；*/ 
	dmaStruct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
	
	/* 配置数据目的地址；*/
	dmaStruct.memory_addr = (uint32_t)g_adcVal;
	/* 配置目的地址是固定的还是增长的；*/ 
	dmaStruct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	/* 配置目的数据传输位宽；*/ 
	dmaStruct.memory_width = DMA_MEMORY_WIDTH_16BIT;
	/* 配置数据传输最大次数；*/ 
	dmaStruct.number = 2;
	/* 配置DMA通道优先级；*/ 
	dmaStruct.priority = DMA_PRIORITY_HIGH;
	dma_init(DMA0, DMA_CH0, &dmaStruct);
	
	/* 使能DMA循环模式搬移数据；*/ 
	dma_circulation_enable(DMA0, DMA_CH0);
	/* 使能DMA通道；*/ 
	dma_channel_enable(DMA0, DMA_CH0);
}


/**
 * @brief  ADC 测试函数
 * @note   读取 ADC 值并计算对应的电压值，通过串口输出
 * @param  无
 * @retval 无
 */
void adcdma_test(void)
{


    /* 串口输出 ADC 值和电压值 */
    printf("ch1 = %d, ch2 = %d\r\n", g_adcVal[0],  g_adcVal[1]);

    /* 延时 1 秒 */
    delay_us(1000000);
}
