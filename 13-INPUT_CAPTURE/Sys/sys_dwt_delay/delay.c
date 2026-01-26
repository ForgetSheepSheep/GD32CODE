#include "sys_dwt_delay/delay.h"


/**
***********************************************************
* @brief  初始化DWT延时功能
* @param  无
* @return 无
* @note
*  1) DWT(Data Watchpoint and Trace)为Cortex-M内核调试/跟踪单元
*  2) CYCCNT为32位周期计数器：每个CPU时钟周期自增1
*  3) 使用delay_us()/delay_ms()前必须先调用本函数使能TRC与CYCCNT
* @attention
*  - 若未使能TRC或CYCCNT，DWT->CYCCNT可能一直不变（延时失效）
***********************************************************
*/
void delay_init(void)
{
		/* 关闭 TRC */
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
	/* 打开 TRC */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

	/* 关闭计数功能 */
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
	/* 打开计数功能 */
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	/* 计数清零 */
	DWT->CYCCNT = 0;
}
/**
***********************************************************
* @brief  微秒级延时函数（基于DWT->CYCCNT周期计数）
* @param  us：延时时间（单位：us）
* @return 无
* @note
*  1) 将us转换为需要等待的CPU周期数ticks：
*       ticks = us * (CPU_Freq / 1,000,000)
*     其中CPU_Freq这里取rcu_clock_freq_get(CK_AHB)（通常等于HCLK=CPU时钟）
*  2) 采用无符号减法(DWT->CYCCNT - start)判断，可正确处理CYCCNT 32位回绕
* @attention
*  - 本函数为忙等待（busy-wait），会一直占用CPU
*  - 延时期间若发生中断，实际延时会略大于设定值（因为CPU被中断打断）
*  - 参数过大时需注意32位计数回绕：最大延时约为(2^32 / CPU_Freq)秒
***********************************************************
*/
void delay_us(uint32_t us)
{

	/* 转换为nUs对应的时钟跳动次数*/
	us *= (rcu_clock_freq_get(CK_AHB) / 1000000);
	
	uint32_t tickStart = DWT->CYCCNT;
	
	/* 延时等待 */
	while ((DWT->CYCCNT - tickStart) < us);
}

/**
***********************************************************
* @brief  毫秒级延时函数（基于delay_us叠加实现）
* @param  ms：延时时间（单位：ms）
* @return 无
* @note
*  通过循环调用delay_us(1000)实现毫秒级延时，逻辑直观
* @attention
*  - 同样为忙等待方式，延时时间较长时不建议使用（占用CPU）
*  - 若需要长时间延时/低功耗/RTOS场景，建议用定时器或系统延时函数
***********************************************************
*/
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
    {
        delay_us(1000);
    }
}
