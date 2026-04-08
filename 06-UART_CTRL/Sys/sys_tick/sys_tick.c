#include "sys_tick/sys_tick.h"

static volatile uint64_t g_sys_tick_count = 0;
/**
***********************************************************
* @brief  systick滴答定时器初始化
* @param  无
* @return 无
***********************************************************
*/
void sys_tick_init(void)
{
	/* 1s计数120M 1ms 120 / 1000  1ms产生一次中断*/
	if(SysTick_Config(rcu_clock_freq_get(CK_AHB) / 1000U ))
	{
		while(1);
	}
}
/**
***********************************************************
* @brief  获取系统时间
* @param  无
* @return 系统运行时间
***********************************************************
*/
uint64_t sys_tick_get_runtime(void)
{
	return g_sys_tick_count;
}
/**
***********************************************************
* @brief  Systick_Handler 定时中断服务函数 1ms执行一下
* @param  无
* @return 无
***********************************************************
*/
void SysTick_Handler(void)
{
	g_sys_tick_count++;
}
