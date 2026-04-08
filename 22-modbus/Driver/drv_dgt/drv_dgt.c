#include "drv_dgt/drv_dgt.h"


/**
***********************************************************
* @brief  看门狗 模块初始化
* @param  无
* @return 无
***********************************************************
*/

void drv_dgt_init(void)
{
	/* 关闭FWDGT_PSC 和 FWDGT_RLD的写保护 */
	/* 配置预分频器 和 重装载寄存器  */
	fwdgt_config(2500, FWDGT_PSC_DIV32); // 40khz   / 32   1.25khz   0.8ms   2000ms
	/* 开启开门狗 */
	fwdgt_enable();
	
}
void feed_dog(void)
{
		fwdgt_counter_reload();
	printf("----喂狗----\r\n");
}
