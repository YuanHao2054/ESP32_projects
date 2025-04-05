#include <stdio.h>
#include "bsp_sg90.h"
#include "string.h"
#include "esp_private/esp_task_wdt.h"
#include "esp_private/esp_task_wdt_impl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// 功能引脚宏定义
#define LED 3  
#define KEY1 18
#define KEY2 19

// 控制舵机和LED的任务
static void control_servo_and_led(void* arg)
{
    TickType_t last_trigger_time = 0; // 记录上次触发时间
    const TickType_t min_interval = 2000 / portTICK_PERIOD_MS; // 最小间隔2秒
    bool key1_pressed = false;
    bool key2_pressed = false;
    
    for(;;) {
        // 读取按键状态
        bool current_key1 = (gpio_get_level(KEY1) == 0);
        bool current_key2 = (gpio_get_level(KEY2) == 0);
        
        // 检测按键按下事件（下降沿）
        bool trigger = false;
        if(current_key1 && !key1_pressed) {
            printf("KEY1 pressed\n");
            trigger = true;
        }
        if(current_key2 && !key2_pressed) {
            printf("KEY2 pressed\n");
            trigger = true;
        }
        
        // 更新按键状态
        key1_pressed = current_key1;
        key2_pressed = current_key2;
        
        // 如果检测到按键按下且不在冷却期
        if(trigger) {
            TickType_t current_time = xTaskGetTickCount();
            
            if(current_time - last_trigger_time >= min_interval) {
                last_trigger_time = current_time;
                
                // 点亮LED
                gpio_set_level(LED, 1);
                
                // 舵机旋转115度
                Set_Servo_Angle(115);
                
                // 延时2秒
                vTaskDelay(min_interval);
                
                // 舵机复原
                Set_Servo_Angle(0);
                
                // 熄灭LED
                gpio_set_level(LED, 0);
            } else {
                printf("Button pressed too fast, ignored.\n");
            }
        }
        
        // 短暂延时，减少CPU占用
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_task_wdt_deinit();
    SG90_Init();
    Set_Servo_Angle(0);
    printf("SG90 Start......\r\n");
    
    // 初始化LED引脚为输出
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    
    // 配置按键引脚
    gpio_config_t key_conf = {
        .intr_type = GPIO_INTR_DISABLE, // 禁用中断
        .mode = GPIO_MODE_INPUT, // 输入模式
        .pin_bit_mask = (1ULL << KEY1) | (1ULL << KEY2), // 两个按键
        .pull_down_en = 0, // 禁能内部下拉
        .pull_up_en = 1 // 使能内部上拉
    };
    gpio_config(&key_conf);
    
    // 开启控制任务
    xTaskCreate(control_servo_and_led, "control_task", 2048, NULL, 10, NULL);
}