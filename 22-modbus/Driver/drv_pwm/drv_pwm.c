#include "drv_pwm/drv_pwm.h"
#include "drv_led/drv_led.h"
#include "sys_dwt_delay/delay.h"
/* ======================== ��̬�������� ======================== */
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

    /* 注意：PA8 被LED1占用，这里跳过PA8的PWM配置
     * PA8 用于LED1控制，不要配置为PWM复用功能
     */

    /* 配置其他PWM引脚，跳过PA8 */
    // PA8 配置为普通GPIO，用于LED1
    // 如果需要PWM输出，请使用其他GPIO引脚
}

static void timerinit(void)
{
    /* ʹ�� TIMER0 ����ʱ�� */
    rcu_periph_clock_enable(RCU_TIMER0);

    /* ��λ TIMER0 */
    timer_deinit(TIMER0);

    /* ------------------------ ������������ ------------------------ */
    timer_parameter_struct timerInitPara;
    timer_struct_para_init(&timerInitPara);

    /* 120MHz / (120) = 1MHz -> 1us/tick */
    timerInitPara.prescaler = 120 - 1;

    /* ���� 500us -> 2kHz */
    timerInitPara.period = 500 - 1;

    timer_init(TIMER0, &timerInitPara);

    /* ------------------------ PWM ͨ������ ------------------------ */
    timer_oc_parameter_struct ocPara;
    timer_channel_output_struct_para_init(&ocPara);

    /* ʹ��ͨ����� + �������ã��ֶ��������Ϊ׼�� */
    ocPara.outputstate  = TIMER_CCX_ENABLE;            /* ͨ�����ʹ�� */
    ocPara.ocpolarity   = TIMER_OC_POLARITY_HIGH;      /* �ߵ�ƽ��Ч */
    ocPara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;     /* ���е�ƽ����ѡ�� */

    /* �ѽṹ��д��ͨ�����ؼ��� */
    timer_channel_output_config(TIMER0, TIMER_CH_0, &ocPara);

    /* PWM ģʽ */
    timer_channel_output_mode_config(TIMER0, TIMER_CH_0, TIMER_OC_MODE_PWM0);

    /* CCR��ռ�ձ� = CCR/(ARR+1) */
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, 250);

    /* ��ѡ��ʹ��Ӱ�ӼĴ������������ռ�ձ�ʱë�� */
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_0, TIMER_OC_SHADOW_ENABLE);

    /* �߼���ʱ����Ҫ�����ʹ�ܣ�TIMER0/TIMER7 ������Ҫ�� */
    timer_primary_output_config(TIMER0, ENABLE);

    /* ------------------------ �����ж����� ------------------------ */
//    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);
//    timer_interrupt_enable(TIMER0, TIMER_INT_UP);
//    nvic_irq_enable(TIMER0_UP_IRQn, 0, 0);

    /* ������ʱ�� */
    timer_enable(TIMER0);
}

void led_pwm_test(void)
{
	for(uint32_t i = 0; i< 500; i+=10)
	{
		timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
		delay_ms(1);
	
		
	}
		for(uint32_t i = 500; i> 0; i-=10)
	{
		timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, i);
		delay_ms(1);
		
	}
}
