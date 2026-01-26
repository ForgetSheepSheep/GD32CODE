#include "config.h"

/********************************系统头文件********************************/
#include "sys_dwt_delay/delay.h"

/********************************驱动头文件********************************/
#include "drv_led/drv_led.h"
#include "drv_key/drv_key.h"

static void sys_init(void)
{
	delay_init();
}

static void drv_init(void)
{
	drv_led_init();
	drv_key_init();
}

int main(void)
{
	
//	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
	
	sys_init();
	
	drv_init();
	
	while(1)
	{
//		for(uint8_t i = 0; i < 3 ; i++)
//		{
//			drv_led_on(i);
//			delay_ms(500);
//			drv_led_off(i);
//			delay_ms(500);
//		}
	}
}


