/**
 ******************************************************************************
 * @file    sys_tick.c
 * @brief   SysTick 系统滴答定时器驱动源文件
 * @details 基于 Cortex-M SysTick 的 1ms 定时器，提供系统运行时间
 ******************************************************************************
 */

#include "sys_tick/sys_tick.h"

/* ======================== 全局变量 ======================== */
/**
 * @brief  系统滴答计数器（单位：ms）
 * @note
 *   使用 volatile 修饰，确保中断和主函数之间访问的可见性
 */
static volatile uint64_t g_sys_tick_count = 0;

/**
 * @brief  SysTick 中断回调函数指针
 * @note
 *   用户可通过 sys_callback() 注册自定义的回调函数
 */
static void (*p_sys_callback)(void) = NULL;

/**
 * @brief  SysTick 系统滴答定时器初始化函数
 * @param  无
 * @retval 无
 * @note
 *   配置 SysTick 为 1ms 中断一次，用于记录系统运行时间
 *   CPU 频率：120MHz，因此重装载值 = 120MHz / 1000 = 120000
 */
void sys_tick_init(void)
{
    /* 1s = 120M，1ms = 120K -> 重装载值，产生 1ms 中断 */
    if (SysTick_Config(rcu_clock_freq_get(CK_AHB) / 1000U))
    {
        while (1);  /* 配置失败，死循环 */
    }
}

/**
 * @brief  注册 SysTick 中断回调函数
 * @param  func : 回调函数指针
 * @retval 无
 * @note
 *   该函数会在每次 SysTick 中断（1ms）中被调用
 *   注意不要在回调函数中执行耗时操作
 */
void sys_callback(void (*func)(void))
{
    p_sys_callback = func;
}

/**
 * @brief  获取系统运行时间
 * @param  无
 * @retval 系统运行时间（单位：ms）
 */
uint64_t sys_tick_get_runtime(void)
{
    return g_sys_tick_count;
}

/**
 * @brief  SysTick_Handler 定时器中断服务函数，1ms 执行一次并递增
 * @param  无
 * @retval 无
 * @note
 *   每次进入中断时，系统滴答计数器加 1
 *   如果注册了回调函数，会调用回调函数
 */
void SysTick_Handler(void)
{
    /* 系统滴答计数器加 1 */
    g_sys_tick_count++;

    /* 调用回调函数，如果存在 */
    void (*cb)(void) = p_sys_callback;
    if (cb != NULL)
    {
        cb();
    }
}
