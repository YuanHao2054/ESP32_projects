/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：club.szlcsc.com
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-15     LCKFB-lp     first version
 */

 #ifndef _BSP_SG90_H
 #define _BSP_SG90_H
 
 #include <stdio.h>
 #include "esp_log.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "driver/uart.h"
 #include "driver/gpio.h"
 #include "driver/i2c.h"
 #include "sdkconfig.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_log.h"
 #include "freertos/queue.h"
 #include <inttypes.h>
 #include "sdkconfig.h"
 #include "driver/gpio.h"
 #include "esp_log.h"
 #include "rom/ets_sys.h"
 #include "esp_system.h"
 #include "driver/gpio.h"
 #include "driver/spi_master.h"
 #include "driver/spi_common.h"
 #include "hal/gpio_types.h"
 #include "driver/ledc.h"
 #include "driver/mcpwm.h"
 #include "string.h"
 
 #define GPIO_SIG        6
 
 #define PWMA_TIMER               LEDC_TIMER_0           //定时器0
 #define SG90_CHANNEL             LEDC_CHANNEL_0         // 使用LEDC的通道0
 #define PWM_MODE                 LEDC_LOW_SPEED_MODE    //低速模式
 #define LEDC_DUTY_RES            LEDC_TIMER_12_BIT      // LEDC分辨率设置为12位
 #define LEDC_FREQUENCY           (50)                 // 频率单位是Hz。设置频率为50Hz
 
 void SG90_Init(void);
 void Set_Servo_Angle(unsigned int angle);
 unsigned int Get_Servo_Angle(void);
 
 #endif