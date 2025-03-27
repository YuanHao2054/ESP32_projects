#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "string.h"
#include <inttypes.h>
#include <stdlib.h>

//声明队列
static QueueHandle_t gpio_evt_queue = NULL;

//gpio中断处理函数
//参数表示：gpio_num
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    //将gpio_num发送到队列，第三个参数表示不等待
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

//任务
static void gpio_task(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) //如果队列有数据
        {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}


void app_main(void)
{
    // 初始化gpio 0
    gpio_config_t io_conf =
        {
            .intr_type = GPIO_INTR_NEGEDGE,     // 下降沿中断
            .mode = GPIO_MODE_INPUT,            // 输出模式
            .pin_bit_mask = 1ULL << GPIO_NUM_0, // 选择gpio 0
            .pull_down_en = 0,                  // 禁用下拉
            .pull_up_en = 1                     // 禁用上拉
        };
    gpio_config(&io_conf); // 配置gpio

    //创建队列，用于存储gpio中断事件
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    //创建任务，用于处理gpio中断事件
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    //注册gpio中断服务
    gpio_install_isr_service(0); //参数0表示默认中断处理程序

    gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *)GPIO_NUM_0); //注册gpio中断处理程序
}
