#include "config.h"

/********************************系统头文件********************************/
#include "sys_dwt_delay/delay.h"

/********************************驱动头文件********************************/
#include "drv_led/drv_led.h"
int main(void)
{
	drv_led_init();
	delay_init();
	while(1)
	{
		for(uint8_t i = 0; i < 3 ; i++)
		{
			drv_led_on(i);
			delay_ms(500);
			drv_led_off(i);
			delay_ms(500);
		}
	}
}
