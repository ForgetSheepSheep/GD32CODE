#ifndef  __DRV_IR_H
#define  __DRV_IR_H
#include "config.h"

#define KEY1_CODE   0X45
#define KEY2_CODE   0X46

/**
***********************************************************
* @brief 红外接收硬件初始化函数
* @param 
* @return 
***********************************************************
*/
void drv_ir_init(void);
	
/**
***********************************************************
* @brief 获取遥控按键码值
* @param ircode，输出，按键码值
* @return 返回是否成功获取到按键码值
***********************************************************
*/
bool drv_ir_get_code(uint8_t *ircode);

#endif /* __DRV_IR_H */
