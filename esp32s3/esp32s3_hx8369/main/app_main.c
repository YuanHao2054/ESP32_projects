/* esp32-idf */
#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

/* 个人定义 */
#include "lv_task.h"
void app_main(void)
{

    xTaskCreate(lv_task, "lv_task",1024*8,NULL,5,NULL);

    while(1)
    {

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
