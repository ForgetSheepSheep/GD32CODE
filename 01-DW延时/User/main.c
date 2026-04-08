#include "config.h"
#include "sys_dwt_delay/delay.h"
int main(void)
{

	/*使能GPIO的时钟*/
	rcu_periph_clock_enable(RCU_GPIOA);
	/*配置为推挽输出模式*/
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_8);
	delay_init();
	while(1)
	{
		gpio_bit_set(GPIOA,GPIO_PIN_8);
		delay_ms(1000);
		gpio_bit_reset(GPIOA,GPIO_PIN_8);
		delay_ms(1000);
	}
}
