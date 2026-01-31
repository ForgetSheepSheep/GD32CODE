/******************************************************************************
 * @file    drv_temp.c
 * @brief   NTC 温度传感器驱动模块（ADC 采样 + 查表法）
 *
 * @note
 *  - 使用 ADC 采集 NTC 分压电压
 *  - 通过 ADC 值在查表中匹配得到对应温度
 *  - 温度数据经过滤波处理
 *
 * 硬件说明：
 *  - NTC 接 ADC 通道 PC3 / ADC_CHANNEL_13
 *  - ADC 参考电压为 MCU Vref
 *
 * 软件说明：
 *  - ADC 使用连续转换模式
 *  - 数据经过中位值平均滤波处理
 *  - 温度最终以 °C 输出
 ******************************************************************************/

#include "drv_temp/drv_temp.h"
#include "sys_dwt_delay/delay.h"
#include <stdlib.h>

/* ========================= 宏定义 ========================= */
#define NTC_TABLE_SIZE        (sizeof(g_ntcAdcTable) / sizeof(g_ntcAdcTable[0]))  // 查表大小
#define INDEX_TO_TEMP(index)  ((int32_t)(index))  // 查表索引对应温度
#define DROP_OR_LITRE         1   /* DROP = 1  LITRE = 0, 查表方式选择 */

/* ========================= NTC 查表数据 ========================= */
/*
 * g_ntcAdcTable[]
 *  - ADC 值从高到低排列，对应温度从 0 ~ 99°C
 *  - 元素值为 ADC 采样值
 */
static const uint16_t g_ntcAdcTable[] = {
    3123, 3089, 3051, 3013, 2973, 2933, 2893, 2852, 2810, 2767,
    2720, 2681, 2637, 2593, 2548, 2503, 2458, 2412, 2367, 2321,
    2275, 2230, 2184, 2138, 2093, 2048, 2002, 1958, 1913, 1869,
    1825, 1782, 1739, 1697, 1655, 1614, 1573, 1533, 1494, 1455,
    1417, 1380, 1343, 1307, 1272, 1237, 1203, 1170, 1138, 1106,
    1081, 1045, 1016, 987,  959,  932,  905,  879,  854,  829,
    806,  782,  760,  738,  716,  696,  675,  656,  637,  618,
    600,  583,  566,  550,  534,  518,  503,  489,  475,  461,
    448,  435,  422,  410,  398,  387,  376,  365,  355,  345,
    335,  326,  316,  308,  299,  290,  283,  274,  267,  259
};

/* ========================= 全局变量 ========================= */
static float g_temp_data = 0.0f;  // 当前温度值（°C）

/* ========================= 内部函数声明 ========================= */
static void GpioInit(void);
static void AdcInit(void);
static uint16_t get_adcval(void);
static uint16_t adc_temp_10mpl(uint16_t adcval);
static int32_t DescBinarySearch(const uint16_t *arr, int32_t size, uint16_t key);
static uint16_t ArithAvgFltr(uint16_t *arr, uint32_t len);
static uint16_t MedianAvgFltr(uint16_t *arr, uint32_t len);
static void push_data_buff(uint16_t temp10mpl);

/* ========================= 温度缓存与滤波 ========================= */
#define MAX_BUF_SIZE  10
static uint16_t g_Temp_10mpl_buf[MAX_BUF_SIZE];

/**
***********************************************************
* @brief  往环形缓存推入新温度值
* @param  temp10mpl: 温度值（单位：0.1°C）
* @return
* @note
*  使用环形缓冲区存储历史温度值，用于滤波处理
***********************************************************
*/
static void push_data_buff(uint16_t temp10mpl)
{
    static uint16_t s_index = 0;
    g_Temp_10mpl_buf[s_index] = temp10mpl;
    s_index++;
    s_index %= MAX_BUF_SIZE;
}

/**
***********************************************************
* @brief  GPIO 初始化
* @param
* @return
* @note
*  PC3 配置为模拟输入模式（AIN），连接 NTC 温度传感器
***********************************************************
*/
static void GpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);                       // 使能 GPIOC 时钟
    gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_3); // PC3 配置为模拟输入
}

/**
***********************************************************
* @brief  ADC 初始化
* @param
* @return
* @note
*  配置 ADC0 通道13，使用连续转换模式和过采样功能
*  过采样配置：16次过采样，右移4位，提高测量精度
***********************************************************
*/
static void AdcInit(void)
{
    rcu_periph_clock_enable(RCU_ADC0);  // 使能 ADC0 时钟

    /* ADC 时钟配置 */
    rcu_adc_clock_config(RCU_CKADC_CKAHB_DIV6);

    /* ADC 模式 */
    adc_mode_config(ADC_MODE_FREE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE); // 连续转换模式
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);           // 数据右对齐

    /* 配置 ADC 通道 */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_13, ADC_SAMPLETIME_239POINT5);

    /* 软件触发转换 */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

    /* 过采样配置 */
    adc_oversample_mode_config(ADC0, ADC_OVERSAMPLING_ALL_CONVERT,
                               ADC_OVERSAMPLING_SHIFT_4B, ADC_OVERSAMPLING_RATIO_MUL16);
    adc_oversample_mode_enable(ADC0);

    /* 启动 ADC */
    adc_enable(ADC0);
    delay_us(50);
    adc_calibration_enable(ADC0);

    /* 软件触发 ADC 转换 */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/**
***********************************************************
* @brief  获取 ADC 值
* @param
* @return  ADC 采样值（0~4095）
* @note
*  等待转换完成标志 EOC，然后读取数据寄存器
***********************************************************
*/
static uint16_t get_adcval(void)
{
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC)); // 等待转换完成
    return adc_regular_data_read(ADC0);        // 读取 ADC 数据
}

/**
***********************************************************
* @brief  ADC 值转换为 0.1°C 温度
* @param  adcval: ADC 采样值
* @return  温度值（单位：0.1°C）
* @note
*  通过二分查找在 NTC 表中找到对应位置，然后进行线性插值
***********************************************************
*/
static uint16_t adc_temp_10mpl(uint16_t adcval)
{
    int32_t index = DescBinarySearch(g_ntcAdcTable, NTC_TABLE_SIZE, adcval); // 查表获取索引

    if (index == 0) {
        return 0;  // 温度最小值
    }

    // 插值计算温度（0.1°C 单位）
    uint16_t temp10mpl = INDEX_TO_TEMP(index - 1) * 10 +
        (g_ntcAdcTable[index - 1] - adcval) * 10 /
        (g_ntcAdcTable[index - 1] - g_ntcAdcTable[index]);

    return temp10mpl;
}

/**
***********************************************************
* @brief  NTC 温度传感器驱动初始化
* @param
* @return
* @note
*  初始化 GPIO 和 ADC，启动连续转换模式
*
*  调用示例：
*   drv_temp_init();  // 初始化后自动开始采集
***********************************************************
*/
void drv_temp_init(void)
{
    GpioInit();  // 初始化 GPIO
    AdcInit();   // 初始化 ADC
}

/**
***********************************************************
* @brief  温度传感器数据处理
* @param
* @return
* @note
*  该函数在主循环中调用，用于更新温度数据：
*   1) 获取 ADC 值并转换为温度
*   2) 推入缓存缓冲区
*   3) 使用中位值平均滤波处理
*
*  调用示例：
*   drv_tempsensor_proc();  // 定期调用更新温度
***********************************************************
*/
void drv_tempsensor_proc(void)
{
    static uint16_t s_counverNUm = 0;
    uint16_t adcval = get_adcval();                   // 获取 ADC 值
    uint16_t temp_10Mpl = adc_temp_10mpl(adcval);    // 转换为 0.1°C
    push_data_buff(temp_10Mpl);                       // 推入缓存
    s_counverNUm++;

    if(s_counverNUm < 3)
    {
        g_temp_data = g_Temp_10mpl_buf[0] / 10.0f; // 初始采样，直接使用
        return;
    }

    if(s_counverNUm >= MAX_BUF_SIZE)
    {
        s_counverNUm = MAX_BUF_SIZE;
    }

    // 使用中位值平均滤波，减小突变影响
    g_temp_data = MedianAvgFltr(g_Temp_10mpl_buf, MAX_BUF_SIZE) / 10.0f;
    s_counverNUm = 0;
}

/**
***********************************************************
* @brief  获取当前温度值
* @param
* @return  温度值（单位：°C）
* @note
*  返回值范围：0.0 ~ 99.0
*  使用前需要先调用 drv_tempsensor_proc() 更新数据
*
*  调用示例：
*   float temp = drv_get_temp();
***********************************************************
*/
float drv_get_temp(void)
{
    return g_temp_data;
}

/**
***********************************************************
* @brief  算术平均滤波
* @param  arr: 数据数组
* @param  len: 数组长度
* @return  平均值
* @note
*  对数组中的所有元素求平均
***********************************************************
*/
static uint16_t ArithAvgFltr(uint16_t *arr, uint32_t len)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        sum += arr[i];
    }
    return (uint16_t)(sum / len); // 返回平均值
}

/**
***********************************************************
* @brief  qsort 比较函数（降序）
* @param  _a: 比较元素 a
* @param  _b: 比较元素 b
* @return  比较结果
* @note
*  用于中位值滤波时的排序
***********************************************************
*/
static int32_t CmpCb(const void *_a, const void *_b)
{
    uint16_t *a = (uint16_t *)_a;
    uint16_t *b = (uint16_t *)_b;
    if (*a > *b) return -1;  // 降序
    if (*a < *b) return 1;
    return 0;
}

/**
***********************************************************
* @brief  中位值平均滤波
* @param  arr: 数据数组
* @param  len: 数组长度
* @return  去除最大最小值后的平均值
* @note
*  先排序，然后去掉最大最小值，求平均值
*  可有效滤除脉冲干扰
***********************************************************
*/
static uint16_t MedianAvgFltr(uint16_t *arr, uint32_t len)
{
    qsort(arr, len, sizeof(uint16_t), CmpCb);        // 降序排序
    return ArithAvgFltr(&arr[1], len - 2);           // 去掉最大最小值，求平均
}

/**
***********************************************************
* @brief  降序二分查找
* @param  arr: 降序数组
* @param  size: 数组大小
* @param  key: 查找值
* @return  查找索引
* @note
*  在降序数组中查找第一个小于等于 key 的元素位置
***********************************************************
*/
#if DROP_OR_LITRE
static int32_t DescBinarySearch(const uint16_t *arr, int32_t size, uint16_t key)
{
    int32_t left = 0;
    int32_t right = size - 1;
    int32_t mid;
    int32_t index = size - 1;

    while (left <= right) {
        mid = left + ((right - left) / 2);
        if (key >= arr[mid]) {   // 降序表查找
            right = mid - 1;
            index = mid;
        } else {
            left = mid + 1;
        }
    }
    return index;
}
#else
// 升序表查找备用
static int32_t DescBinarySearch(const uint16_t *arr, int32_t size, uint16_t key)
{
    int32_t left = 0;
    int32_t right = size - 1;
    int32_t mid;
    int32_t index = size - 1;

    while (left <= right) {
        mid = left + ((right - left) / 2);
        if (key <= arr[mid]) {   // 升序表查找
            right = mid - 1;
            index = mid;
        } else {
            left = mid + 1;
        }
    }
    return index;
}
#endif
