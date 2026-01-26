#include "drv_pwm/drv_pwm.h"
#include "drv_led/drv_led.h"
#include "sys_dwt_delay/delay.h"
/* ======================== 静态函数声明 ======================== */
static void timerinit(void);
static void GPIO_Init(void);

void drv_pwm_init(void)
{
    GPIO_Init();
    timerinit();
}

static void GPIO_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    /* PA8 复用推挽输出（确认 PA8 对应 TIMER0_CH0 的 AF 映射正确） */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_MAX, GPIO_PIN_8);
}

static void timerinit(void)
{
    /* 使能 TIMER0 外设时钟 */
    rcu_periph_clock_enable(RCU_TIMER0);

    /* 复位 TIMER0 */
    timer_deinit(TIMER0);

    /* ------------------------ 基本计数参数 ------------------------ */
    timer_parameter_struct timerInitPara;
    timer_struct_para_init(&timerInitPara);

    /* 120MHz / (120) = 1MHz -> 1us/tick */
    timerInitPara.prescaler = 120 - 1;

    /* 周期 500us -> 2kHz */
    timerInitPara.period = 500 - 1;

    timer_init(TIMER0, &timerInitPara);

    /* ------------------------ PWM 通道配置 ------------------------ */
    timer_oc_parameter_struct ocPara;
    timer_channel_output_struct_para_init(&ocPara);

    /* 使能通道输出 + 极性配置（字段名以你库为准） */
    ocPara.outputstate  = TIMER_CCX_ENABLE;            /* 通道输出使能 */
    ocPara.ocpolarity   = TIMER_OC_POLARITY_HIGH;      /* 高电平有效 */
    ocPara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;     /* 空闲电平（可选） */

    /* 把结构体写入通道（关键） */
    timer_channel_output_config(TIMER0, TIMER_CH_0, &ocPara);

    /* PWM 模式 */
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);

    /* CCR：占空比 = CCR/(ARR+1) */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 250);

    /* 可选：使能影子寄存器，避免更新占空比时毛刺 */
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_ENABLE);

    /* 高级定时器需要主输出使能（TIMER0/TIMER7 常见需要） */
    timer_primary_output_config(TIMER0, ENABLE);

    /* ------------------------ 更新中断配置 ------------------------ */
//    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);
//    timer_interrupt_enable(TIMER0, TIMER_INT_UP);
//    nvic_irq_enable(TIMER0_UP_IRQn, 0, 0);

    /* 启动定时器 */
    timer_enable(TIMER0);
}

void led_pwm_test(void)
{
	for(uint32_t i = 0; i< 500; i+=1)
	{
		timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
		delay_ms(1);
	
		
	}
		for(uint32_t i = 500; i> 0; i-=1)
	{
		timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
		delay_ms(1);
		
	}
}
