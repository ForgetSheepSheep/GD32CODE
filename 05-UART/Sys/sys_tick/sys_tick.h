#ifndef __SYS_TICK
#define __SYS_TICK
#include "config.h"

/**
***********************************************************
* @brief  systick滴答定时器初始化
* @param  无
* @return 无
***********************************************************
*/
void sys_tick_init(void);
/**
***********************************************************
* @brief  获取系统时间
* @param  无
* @return 系统运行时间
***********************************************************
*/
uint64_t sys_tick_get_runtime(void);

#endif /* __SYS_TICK */
