#include "drv_key/drv_key.h"
#include "drv_led/drv_led.h"

static void key_gpio_init(void);
static void key_exti_init(void);
/**
***********************************************************
* @brief  key硬件初始化
* @param  无
* @return 无
***********************************************************
*/
void drv_key_init(void)
{
	key_gpio_init();
	key_exti_init();
}
/**************************IO_Init**************************/
static void key_gpio_init(void)
{
	//key1
	rcu_periph_clock_enable(RCU_GPIOA);
	gpio_init(GPIOA, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, GPIO_PIN_0);
	
	//key2 | key3 | key4
	rcu_periph_clock_enable(RCU_GPIOG);
	gpio_init(GPIOG, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, GPIO_PIN_13|
										   GPIO_PIN_14|GPIO_PIN_15);
}
/*************************EXTI_Init*************************/
static void key_exti_init(void)
{
    /* 使能AF时钟（EXTI线映射需要） */
    rcu_periph_clock_enable(RCU_AF);

    /* I/O连线到EXTI线：PA0 -> EXTI0 */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0);

    /* I/O连线到EXTI线：PG13/14/15 -> EXTI13/14/15（必须分别配置） */
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOG, GPIO_PIN_SOURCE_13);
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOG, GPIO_PIN_SOURCE_14);
    gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOG, GPIO_PIN_SOURCE_15);

    /* 配置EXTI：中断模式 + 下降沿触发 */
    exti_init(EXTI_0,  EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_init(EXTI_13, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_init(EXTI_14, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
	exti_init(EXTI_15, EXTI_INTERRUPT, EXTI_TRIG_FALLING);

    /* 清除挂起标志位（避免上电/配置过程产生伪中断） */
    exti_interrupt_flag_clear(EXTI_0);
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_14);
	exti_interrupt_flag_clear(EXTI_15);

    /* 使能NVIC中断（EXTI0单独一路；EXTI10~15共用一路） */
    nvic_irq_enable(EXTI0_IRQn,     1, 1);
    nvic_irq_enable(EXTI10_15_IRQn, 1, 1);
}

/*
***********************************************************
* @brief  EXTI10_15中断服务函数  对应EXTI13~EXTI15（如PG13~PG15）
* @param  无
* @return 无
***********************************************************
*/
void EXTI10_15_IRQHandler(void)
{
    /* EXTI13 */
    if (exti_interrupt_flag_get(EXTI_13) != RESET)
    {
        drv_led_on(LED2);
        exti_interrupt_flag_clear(EXTI_13);
    }

    /* EXTI14 */
    if (exti_interrupt_flag_get(EXTI_14) != RESET)
    {
        drv_led_off(LED1);
        exti_interrupt_flag_clear(EXTI_14);
    }

    /* EXTI15 */
    if (exti_interrupt_flag_get(EXTI_15) != RESET)
    {
        drv_led_off(LED2);
        exti_interrupt_flag_clear(EXTI_15);
    }
}

/*
***********************************************************
* @brief  EXTI0中断服务函数  对应EXTI0（如PA0）
* @param  无
* @return 无
***********************************************************
*/
void EXTI0_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_0) != RESET)
    {
        drv_led_on(LED1);
        exti_interrupt_flag_clear(EXTI_0);
    }
}
