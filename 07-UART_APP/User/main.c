#include "config.h"
/********************************系统头文件********************************/
#include "sys_dwt_delay/delay.h"
#include "sys_tick/sys_tick.h"
/********************************驱动头文件********************************/
#include "app_uart/app_uart.h"
/********************************驱动头文件********************************/
#include "drv_led/drv_led.h"
#include "drv_key/drv_key.h"
#include "drv_uart/drv_uart.h"


static void sys_init(void)
{
    delay_init();
    sys_tick_init();
}
static void app_init(void)
{
	app_uart_init();
}
static void drv_init(void)
{
    drv_led_init();
    drv_key_init();
	drv_uart_init(115200);
}

/**
***********************************************************
* @brief  主函数：按键功能测试
* @note
*  - 单击：松开后约300ms才返回（单击延迟确认，为了与双击区分）
*  - 双击：第二次松开时立即返回
*  - 长按：按住超过阈值后松开返回
***********************************************************
*/
int main(void)
{
    /*******系统初始化********/
    sys_init();
    /*******驱动初始化********/
    drv_init();
	 /*******APP初始化********/
	app_init();

	printf("uart_test_start\r\n");
    while (1)
    {
        
		app_uart_task();
        
    }
}
