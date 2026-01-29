/**
 ******************************************************************************
 * @file    drv_dgt.c
 * @brief   DGT (看门狗) 驱动源文件
 * @details 用于 GD32F303 的独立看门狗驱动，防止程序跑飞
 ******************************************************************************
 */

#include "drv_dgt/drv_dgt.h"

/**
 * @brief  独立看门狗模块初始化函数
 * @note
 *   配置看门狗超时时间为 2000ms，之后必须定期喂狗
 * @param  无
 * @retval 无
 */
void drv_dgt_init(void)
{
    /* 配置独立看门狗：预分频 = 32，重装载值 = 2500 */
    /* 内部 RC 40kHz / 32 = 1.25kHz (0.8ms/count) */
    /* 超时时间 = 2500 * 0.8ms = 2000ms */
    fwdgt_config(2500, FWDGT_PSC_DIV32);

    /* 使能看门狗 */
    fwdgt_enable();
}

/**
 * @brief  喂狗函数
 * @param  无
 * @retval 无
 * @note
 *   必须在看门狗超时之前（2000ms）调用此函数，否则系统会复位
 *   建议在主循环中定期调用
 */
void feed_dog(void)
{
    /* 重载看门狗计数器 */
    fwdgt_counter_reload();
    /* 打印喂狗日志 */
    printf("----喂狗----\r\n");
}
