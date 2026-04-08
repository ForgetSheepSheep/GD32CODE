#include "app_sensor/app_sensor.h"
#include "drv_sensor/drv_sensor.h"
/**
***********************************************************
* @brief ����������������
* @param 
* @return 
***********************************************************
*/
void app_sensor_task(void)
{
	drv_sensor_proc();
}
void app_sensor_printf_task(void)
{
	Sensor_Data_t sensordata;
	drv_sensor_get_data(&sensordata);
	printf("temp: %.1fC, humi: %d%%RH\n", sensordata.temp, sensordata.humi);
}
