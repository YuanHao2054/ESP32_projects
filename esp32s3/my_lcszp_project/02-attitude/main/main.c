#include <stdio.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp32_s3_szp.h"
#include "driver/gpio.h"


static const char *TAG = "main";

//定义QMI8658结构体变量
t_sQMI8658 QMI8658;

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_i2c_init());  //检查初始化是否成功，不成功则打印错误信息
    ESP_LOGI(TAG, "I2C initialized"); //打印初始化成功信息

    qmi8658_init(); //初始化qmi8658

    while(1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS); //延时1s
        qmi8658_fetch_angleFromAcc(&QMI8658); //获取加速度角度
        ESP_LOGI(TAG, "angle_x = %.1f  angle_y = %.1f angle_z = %.1f",QMI8658.AngleX, QMI8658.AngleY, QMI8658.AngleZ);
    }


}


