#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"
#include "es8311.h"
#include "pca9557.h"
#include "myes8311.h"

const char *TAG_MAIN = "main";

void app_main(void)
{
    printf("开始执行i2s总线的es8311芯片的示例代码\n");
    /* 初始化I2S外设 */
    if (i2s_driver_init() != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "i2s driver init failed");
        abort();
    } else {
        ESP_LOGI(TAG_MAIN, "i2s driver init success");
    }
    /* 初始化I2C 以及初始化es8311芯片 */
    if (es8311_codec_init() != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "es8311 codec init failed");
        abort();
    } else {
        ESP_LOGI(TAG_MAIN, "es8311 codec init success");
    }
    pca9557_init(); //初始化IO扩展芯片
    pa_en(1); // 打开音频

    /* 创建播放音乐任务 */
    xTaskCreate(i2s_music, "i2s_music", 4096, NULL, 5, NULL);

}
