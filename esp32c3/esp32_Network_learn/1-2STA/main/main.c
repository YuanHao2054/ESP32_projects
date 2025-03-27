#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"

uint8_t connect_count = 0; //连接次数

//wifi事件回调函数
void wifi_callback(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) //WIFI启动成功事件
    {
        esp_wifi_connect();
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) //WIFI连接AP失败
    {
        connect_count++;
        if(connect_count <= 5)
          esp_wifi_connect();
        else
          printf("WIFI连接失败\n");
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) //WIFI连接AP成功
    {
        //connect_count = 0;
        printf("WIFI连接成功\n");

        //获取IP信息
        ip_event_got_ip_t* info = (ip_event_got_ip_t*)event_data;//获取IP信息
        printf("IP:" IPSTR "\n", IP2STR(&info->ip_info.ip)); //&(info->ip_info.ip)为IP信息的地址
    }
}

void app_main(void)
{
    //初始化nvs分区
    nvs_flash_init();

    //创建默认事件循环
    esp_event_loop_create_default();
    //注册事件
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_START, wifi_callback, NULL, NULL);//WIFI启动成功事件
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_callback, NULL, NULL);//WIFI连接AP失败
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_callback, NULL, NULL);//WIFI连接AP成功，且获取到IP
    

    //初始化网络接口
    esp_netif_init();

    //创建STA类型网卡
    esp_netif_create_default_wifi_sta();

    //初始化wifi
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    //设置wifi模式为STA
    esp_wifi_set_mode(WIFI_MODE_STA);

    //配置STA模式下的wifi连接信息
    wifi_config_t sta_cfg = 
    {
        .sta = 
        {
            .ssid = "虚空终端",
            .password = "503503503"
            //.listen_interval = 10 //设置STA的listen interval，单位为AP的beacon帧间隔，范围为1~255，默认为3
            //关系到STA的省电模式，listen interval越大，省电越好，但是网速越慢
        }
    };
    esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);

    //启动wifi
    esp_wifi_start();

    //esp_wifi_connect();
    //该函数放到wifi启动成功事件回调函数中

    //设置低功耗模式
    esp_wifi_set_ps(WIFI_PS_NONE); //任何时候都不进入低功耗模式，网速快，但是耗电多
    //esp_wifi_set_ps(WIFI_PS_MIN_MODEM); //最小功耗模式，网速慢，但是省电，不调用该函数默认为该模式
    //esp_wifi_set_ps(WIFI_PS_MAX_MODEM); //最大功耗模式，网速快，但是耗电多



    // 初始化gpio12、13，设置为推挽输出
    esp_rom_gpio_pad_select_gpio(12);
    esp_rom_gpio_pad_select_gpio(13);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);
    gpio_set_direction(13, GPIO_MODE_OUTPUT);

    // 在循环里每1s打印一次hello world
    while (1)
    {
        // 打印1到10，循环
        for (int i = 1; i <= 10; i++)
        {
            printf("%d\n", i);
            gpio_set_level(12, 1);
            gpio_set_level(13, 0);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            gpio_set_level(12, 0);
            gpio_set_level(13, 1);
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            if (i == 10)
            {
                i = 0;
            }
        }
    }
}
