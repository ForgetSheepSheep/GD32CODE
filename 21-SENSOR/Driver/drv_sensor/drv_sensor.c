/******************************************************************************
 * @file    drv_sensor.c
 * @brief   温湿度传感器统一驱动模块
 *
 * @note
 *  - 整合温度传感器（NTC）和湿度传感器（HS1101）
 *  - 提供统一的初始化和数据处理接口
 *  - 提供统一的传感器数据获取接口
 *
 * 硬件说明：
 *  - 温度传感器：PC3 / ADC0_CHANNEL13
 *  - 湿度传感器：PC4 / ADC1_CHANNEL14，PB0/PB1 交流驱动
 *
 * 软件说明：
 *  - 温湿度数据统一管理
 *  - 需要在主循环中定期调用处理函数
 ******************************************************************************/

#include "drv_sensor/drv_sensor.h"
#include "drv_hum/drv_hum.h"
#include "drv_temp/drv_temp.h"

/**
***********************************************************
* @brief  传感器驱动初始化
* @param
* @return
* @note
*  该函数完成以下初始化：
*   1) 温度传感器初始化（drv_temp_init）
*   2) 湿度传感器初始化（HumiDrvInit）
*
*  调用示例：
*   drv_sensor_init();  // 系统初始化时调用一次
***********************************************************
*/
void drv_sensor_init(void)
{
    drv_temp_init();      // 初始化温度传感器
    HumiDrvInit();       // 初始化湿度传感器
}

/**
***********************************************************
* @brief  传感器数据处理
* @param
* @return
* @note
*  该函数在主循环中调用，用于更新传感器数据：
*   1) 更新温度数据（drv_tempsensor_proc）
*   2) 更新湿度数据（drv_Humi_Sensor_Proc）
*
*  调用示例：
*   drv_sensor_proc();  // 在主循环中定期调用（如每10ms）
***********************************************************
*/
void drv_sensor_proc(void)
{
    /* 更新温度传感器数据 */
    drv_tempsensor_proc();

    /* 更新湿度传感器数据，传入当前温度进行温度补偿 */
    drv_Humi_Sensor_Proc((uint8_t)drv_get_temp());
}

/**
***********************************************************
* @brief  获取传感器数据
* @param  sensor_data: 传感器数据结构体指针
* @return
* @note
*  将当前的温度和湿度数据填充到传入的结构体中
*  使用前需要先调用 drv_sensor_proc() 更新数据
*
*  调用示例：
*   Sensor_Data_t data;
*   drv_sensor_get_data(&data);
*   printf("Temp: %.1fC, Humi: %d%%RH\n", data.temp, data.humi);
***********************************************************
*/
void drv_sensor_get_data(Sensor_Data_t *sensor_data)
{
    sensor_data->temp = drv_get_temp();              // 获取温度值
    sensor_data->humi = (uint8_t)drv_get_humiData(); // 获取湿度值
}
