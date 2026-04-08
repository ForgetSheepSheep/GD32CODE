# LED1控制问题修复报告

## 问题分析
LED1一直亮且无法通过Modbus控制的主要原因：
- ModbusTask()没有集成到任务调度系统中
- ModbusTask()在main.c中被直接调用，没有固定的执行间隔

## 修复步骤

### 1. 修改task.c
- 添加modbus_app.h头文件
- 将ModbusTask添加到任务表中，执行周期为100ms
- 添加一个测试任务，每2秒关闭一次LED1

### 2. 修改main.c
- 从主循环中移除直接调用ModbusTask()的代码

## 代码修改详情

### task.c修改：
```c
// 添加头文件
#include "modbus_app.h"
#include "drv_led/drv_led.h"

// 任务表中添加ModbusTask
{ false, 100   , 100  , ModbusTask     },

// 添加测试任务
{ false, 2000  , 2000 , test_led_task   },

// 测试任务实现
static void test_led_task(void)
{
    /* 关闭LED1 */
    drv_led_off(LED1);
}
```

### main.c修改：
```c
// 从while(1)循环中移除ModbusTask();
// 原代码：
// task_loop();
// ModbusTask();
// 修改为：
task_loop();
```

## 修复效果
1. ModbusTask现在通过任务调度系统执行，保证固定间隔（100ms）
2. 添加了测试任务，每2秒自动关闭LED1
3. Modbus协议现在应该能正常控制LED1的开关

## 测试方法
1. 编译并烧录程序
2. 观察LED1是否每2秒闪烁一次（测试任务）
3. 使用Modbus工具向地址0x0002写入0或1，验证LED1是否能正常开关