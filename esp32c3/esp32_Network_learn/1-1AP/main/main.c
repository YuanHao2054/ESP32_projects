#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include <string.h>
#include "esp_mac.h"

// STA接入事件处理函数 回调函数由默认事件循环调用
void WIFI_Callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) // 判断是否是STA接入事件(接入事件的ID是IP_EVENT_AP_STAIPASSIGNED)
    {
        //把接入的STA的信息写入到event_data中
        ip_event_got_ip_t *info = (ip_event_got_ip_t *)event_data; // 把event_data转换为ip_event_got_ip_t类型的指针, info为指向event_data的指针
        //打印STA的IP地址
        //printf("STA的IP:%d.%d.%d.%d", IP2STR(&info->ip_info.ip)); // 打印STA的IP地址
        printf("STA的IP:"IPSTR"\n", IP2STR(&info->ip_info.ip)); // 打印STA的IP地址
        //打印STA的MAC地址
        wifi_sta_list_t sta_list; // 定义STA列表
        esp_wifi_ap_get_sta_list(&sta_list); // 获取STA的MAC地址
        //printf("STA的MAC:"MACSTR"\n", MAC2STR(sta_mac)); // 打印STA的MAC地址
        for (int i = 0; i < sta_list.num; i++)
        {
            printf("新接入的STA的MAC:"MACSTR"\n", MAC2STR(sta_list.sta[i].mac));
        }
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) // 判断是否是STA断开事件(断开事件的ID是WIFI_EVENT_AP_STADISCONNECTED)
    {
        //把断开的STA的信息写入到event_data中
        wifi_event_ap_stadisconnected_t *info = (wifi_event_ap_stadisconnected_t *)event_data; // 把event_data转换为wifi_event_ap_stadisconnected_t类型的指针, info为指向event_data的指针
        //打印断开的STA的MAC地址
        printf("断开的STA的MAC:"MACSTR"\n", MAC2STR(info->mac)); // 打印断开的STA的MAC地址

    }
}

void app_main(void)
{
    nvs_flash_init();
    esp_event_loop_create_default(); // 创建默认事件循环

    // 注册事件处理程序 STA接入事件
    //如果读到一个结构体，base是IP_EVENT，id是IP_EVENT_AP_STAIPASSIGNED时，调用WIFI_Callback函数
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &WIFI_Callback, NULL, NULL); // 注册wifi事件处理函数
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &WIFI_Callback, NULL, NULL); // 注册当STA设备断开时的事件处理函数

    // 初始化网卡的底层配置
    esp_netif_init();

    // 以默认的方式创建一个AP类型的网卡
    esp_netif_t *pNetif = esp_netif_create_default_wifi_ap();

    // 初始化wifi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 设置wifi模式
    esp_wifi_set_mode(WIFI_MODE_AP);

    // 配置AP的参数
    // 管理参数的结构体
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "esp32-c3",                // wifi的名字
            .ssid_len = strlen("esp32-c3"),    //
            .channel = 1,                      // 信道
            .password = "123456789",           // 密码
            .max_connection = 4,               // 最大连接数
            .authmode = WIFI_AUTH_WPA_WPA2_PSK // 加密方式 认证方式，大部分设备都支持的WPA2_PSK
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);

    // 启动wifi
    esp_wifi_start();

    // 初始化gpio12、13，设置为推挽输出
    esp_rom_gpio_pad_select_gpio(4);
    esp_rom_gpio_pad_select_gpio(13);
    
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_direction(13, GPIO_MODE_OUTPUT);

    // 在循环里每1s打印一次hello world
    while (1)
    {
        // 打印1到10，循环
        for (int i = 1; i <= 10; i++)
        {
            printf("%d\n", i);
            gpio_set_level(4, 1);
            gpio_set_level(13, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            printf("hello world\n");
            gpio_set_level(4, 0);
            gpio_set_level(13, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            printf("hello world\n");

            if (i == 10)
            {
                i = 0;
            }
        }
    }
}
