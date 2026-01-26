#include "drv_pwm/drv_pwm.h"
#include "drv_led/drv_led.h"

#if 0
/* ======================== 静态函数声明 ======================== */
/**
 * @brief  定时器初始化（内部使用）
 * @param  tim_us  定时周期，单位：微秒（us）
 * @return 无
 */
static void timerinit(uint32_t tim_us);

/* ======================== 对外接口函数 ======================== */
/**
***********************************************************
* @brief  定时器模块初始化
* @note   当前配置为 1ms 周期中断
*         定时器溢出中断中翻转 LED1
* @param  无
* @return 无
***********************************************************
*/
void drv_timer_init(void)
{
    /* 设定定时周期为 1000us，即 1ms */
    timerinit(1000);
}

/* ======================== 定时器初始化实现 ======================== */
/**
***********************************************************
* @brief  TIMER0 初始化函数
* @param  tim_us 定时周期（单位：微秒）
* @note
* 1) TIMER0 外设时钟需提前使能
* 2) 定时器计数频率 = TIMER_CLK / (prescaler + 1)
* 3) 当前 prescaler=1200-1：
*    若 TIMER 时钟为 120MHz，则计数频率为：
*       120MHz / 1200 = 100kHz → 10us / tick
* 4) 周期 period = tim_us - 1
*    实际中断周期 = tim_us × 10us（需与时钟配置匹配）
***********************************************************
*/
static void timerinit(uint32_t tim_us)
{
    /* 使能 TIMER0 外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER0);

    /* 复位 TIMER0 寄存器到默认状态 */
    timer_deinit(TIMER0);

    /* 定时器参数结构体 */
    timer_parameter_struct timerInitPara;

    /* 初始化结构体为默认值 */
    timer_struct_para_init(&timerInitPara);

    /* 预分频配置
     * prescaler = 1200 - 1
     * 若 TIMER0 输入时钟为 120MHz，则分频后：
     * 120MHz / 1200 = 100kHz
     */
    timerInitPara.prescaler = 1200 - 1;

    /* 自动重装载值
     * 计数范围：0 ~ period
     * 中断周期 ≈ (period + 1) × 定时器计数周期
     */
    timerInitPara.period = tim_us - 1;

    /* 初始化 TIMER0 */
    timer_init(TIMER0, &timerInitPara);

    /* 使能更新（溢出）中断 */
    timer_interrupt_enable(TIMER0, TIMER_INT_UP);

    /* 使能 TIMER0 更新中断的 NVIC */
    nvic_irq_enable(TIMER0_UP_IRQn, 0, 0);

    /* 启动定时器 */
    timer_enable(TIMER0);
}

/* ======================== 定时器中断服务函数 ======================== */
/**
***********************************************************
* @brief  TIMER0 更新中断服务函数
* @note
* 1) 每次定时器溢出进入该中断
* 2) 中断中翻转 LED1 状态
* 3) 使用前必须清除中断标志位
***********************************************************
*/
void TIMER0_UP_IRQHandler(void)
{
    /* 判断更新中断标志位 */
    if (timer_interrupt_flag_get(TIMER0, TIMER_INT_FLAG_UP) == SET)
    {
        /* 翻转 LED1 */
        drv_led_toggle(LED1);

        /* 清除更新中断标志位 */
        timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);
    }
}
#else
#include "drv_pwm/drv_pwm.h"
#include "drv_led/drv_led.h"

/* ======================== 静态函数声明 ======================== */
/**
 * @brief  定时器初始化（内部使用）
 * @param  tim_us  定时周期，单位：微秒（us）
 * @return 无
 */
static void timerinit(uint32_t tim_us);

/* ======================== 对外接口函数 ======================== */
/**
***********************************************************
* @brief  定时器模块初始化
* @note   当前配置为 1ms 周期中断（需与 prescaler/时钟匹配）
*         定时器溢出中断中翻转 LED1
* @param  无
* @return 无
***********************************************************
*/
void drv_timer_init(void)
{
    /* 设定定时周期为 1000us，即 1ms */
    timerinit(1000);
}

/* ======================== 定时器初始化实现 ======================== */
/**
***********************************************************
* @brief  TIMER4 初始化函数
* @param  tim_us 定时周期（单位：微秒）
* @note
* 1) TIMER4 外设时钟需提前使能
* 2) 定时器计数频率 = TIMER_CLK / (prescaler + 1)
* 3) 当前 prescaler=1200-1：
*    若 TIMER 时钟为 120MHz，则计数频率为：
*       120MHz / 1200 = 100kHz → 10us / tick
* 4) 周期 period = tim_us - 1
*    实际中断周期 = tim_us × 10us（需与时钟配置匹配）
***********************************************************
*/
static void timerinit(uint32_t tim_us)
{
    /* 使能 TIMER4 外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER4);

    /* 复位 TIMER4 寄存器到默认状态 */
    timer_deinit(TIMER4);

    /* 定时器参数结构体 */
    timer_parameter_struct timerInitPara;

    /* 初始化结构体为默认值 */
    timer_struct_para_init(&timerInitPara);

    /* 预分频配置
     * prescaler = 1200 - 1
     * 若 TIMER4 输入时钟为 120MHz，则分频后：
     * 120MHz / 1200 = 100kHz
     */
    timerInitPara.prescaler = 1200 - 1;

    /* 自动重装载值
     * 计数范围：0 ~ period
     * 中断周期 ≈ (period + 1) × 定时器计数周期
     */
    timerInitPara.period = tim_us - 1;

    /* 初始化 TIMER4 */
    timer_init(TIMER4, &timerInitPara);

    /* 清除一次更新中断标志（防止上电残留导致立刻进中断） */
    timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_UP);

    /* 使能更新（溢出）中断 */
    timer_interrupt_enable(TIMER4, TIMER_INT_UP);

    /* 使能 TIMER4 更新中断的 NVIC */
    nvic_irq_enable(TIMER4_IRQn, 0, 0);

    /* 启动定时器 */
    timer_enable(TIMER4);
}

/* ======================== 定时器中断服务函数 ======================== */
/**
***********************************************************
* @brief  TIMER4 更新中断服务函数
* @note
* 1) 每次定时器溢出进入该中断
* 2) 中断中翻转 LED1 状态
* 3) 使用前必须清除中断标志位
***********************************************************
*/
void TIMER4_IRQHandler(void)
{
    /* 判断更新中断标志位 */
    if (timer_interrupt_flag_get(TIMER4, TIMER_INT_FLAG_UP) == SET)
    {
        /* 清除更新中断标志位（建议先清再执行逻辑） */
        timer_interrupt_flag_clear(TIMER4, TIMER_INT_FLAG_UP);

        /* 翻转 LED1 */
        drv_led_toggle(LED1);
    }
}
#endif
