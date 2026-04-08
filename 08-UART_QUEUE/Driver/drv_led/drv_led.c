#include "drv_led/drv_led.h"

typedef struct
{
	rcu_periph_enum rcu;
	uint32_t gpio;
	uint32_t gpio_pin;
}Led_GPIO_t;

static const Led_GPIO_t g_gpio_list[] = 
{
	{RCU_GPIOA,GPIOA,GPIO_PIN_8},
	{RCU_GPIOE,GPIOE,GPIO_PIN_6},
	{RCU_GPIOF,GPIOF,GPIO_PIN_6},
};

#define LED_NUM_MAX  (sizeof(g_gpio_list) / sizeof(g_gpio_list[0]))

/**
***********************************************************
* @brief  led初始化
* @param  无
* @return 无
***********************************************************
*/
void drv_led_init()
{

	for (uint8_t i = 0; i < LED_NUM_MAX; i++)
	{
		rcu_periph_clock_enable(g_gpio_list[i].rcu);
		gpio_init(g_gpio_list[i].gpio, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ,
				  g_gpio_list[i].gpio_pin);
		gpio_bit_reset(g_gpio_list[i].gpio, g_gpio_list[i].gpio_pin);
	}
	
}

/**
***********************************************************
* @brief  点亮LED
* @param  led_id LED_ID  (0-2)
* @return 无
***********************************************************
*/
void drv_led_on(uint8_t led_id)
{
	if(led_id > LED_NUM_MAX) 
	{
		return;
	}
	gpio_bit_set(g_gpio_list[led_id].gpio, g_gpio_list[led_id].gpio_pin);
}

/**
***********************************************************
* @brief  熄灭LED
* @param  led_id LED_ID  (0-2)
* @return 无
***********************************************************
*/
void drv_led_off(uint8_t led_id)
{
	if(led_id > LED_NUM_MAX) 
	{
		return;
	}
	gpio_bit_reset(g_gpio_list[led_id].gpio, g_gpio_list[led_id].gpio_pin);
}
