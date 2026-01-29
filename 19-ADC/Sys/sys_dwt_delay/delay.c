/**
 ******************************************************************************
 * @file    delay.c
 * @brief   DWT 延时驱动源文件
 * @details 基于 Cortex-M4 DWT CYCCNT 的高精度延时函数
 ******************************************************************************
 */

#include "sys_dwt_delay/delay.h"

/**
 * @brief  初始化 DWT 计数器
 * @param  无
 * @retval 无
 * @note
 *   1) DWT (Data Watchpoint and Trace) 为 Cortex-M 内核自带的调试/性能监控单元
 *   2) CYCCNT 为 32 位循环计数器，用于统计 CPU 运行周期数，上电后从 0 开始
 *   3) 使能 delay_us()/delay_ms() 前必须调用此函数使能 TRC 和 CYCCNT
 * @attention
 *   - 若未使能 TRC 和 CYCCNT，DWT->CYCCNT 读取会一直为 0，延时失效
 */
void delay_init(void)
{
    /* 失能 TRC */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;

    /* 使能 TRC */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 失能循环计数器 */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;

    /* 使能循环计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 清零计数器 */
    DWT->CYCCNT = 0;
}

/**
 * @brief  输入延时周期计数，通过 DWT->CYCCNT 计数实现
 * @param  us : 延时时间（单位：us）
 * @retval 无
 * @note
 *   1) 将 us 转换为需要的 CPU 时钟周期数 ticks
 *       ticks = us * (CPU_Freq / 1,000,000)
 *     其中 CPU_Freq 可以通过 rcu_clock_freq_get(CK_AHB) 获取（通过 HCLK=CPU时钟）
 *   2) 采用无符号偶数差分 (DWT->CYCCNT - start) 判断，充分利用 CYCCNT 32 位循环计数
 * @attention
 *   - 此时为忙等待方式，CPU 在延时期间不能执行其他任务
 *   - 延时期间计数器不会清零，实际延时可以超出定时器中断，适合定时器中断上下文
 *   - 最大延时时间需注意 32 位循环计数，最大可计为 (2^32 / CPU_Freq) 秒
 */
void delay_us(uint32_t us)
{
    /* 计算为 nUs 对应的 CPU 周期数 */
    us *= (rcu_clock_freq_get(CK_AHB) / 1000000);

    uint32_t tick_start = DWT->CYCCNT;

    /* 忙等待循环 */
    while ((DWT->CYCCNT - tick_start) < us);
}

/**
 * @brief  毫秒级延时函数，通过多次调用 delay_us 实现
 * @param  ms : 延时时间（单位：ms）
 * @retval 无
 * @note
 *   通过多次调用 delay_us(1000) 实现毫秒级延时，提高代码复用性
 * @attention
 *   - 此类为忙等待方式，延时期间 CPU 无法做其他事情
 *   - 若需要长时间延时/任务切换/RTOS 场景，应使用 RTOS 的延时机制或定时器中断
 */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        delay_us(1000);
    }
}
