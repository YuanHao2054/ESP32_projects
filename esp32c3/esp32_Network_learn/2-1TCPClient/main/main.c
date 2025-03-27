#include <stdio.h>
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "lwip/sockets.h"

uint8_t connect_count = 0; //连接次数
void wifi_sta_init(void); //初始化STA模式的WIFI

//声明接收数据缓冲区
char recv_buf[1000] = {0};

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

    //初始化STA模式的WIFI
    wifi_sta_init();

    //延时等待WIFI连接成功
    vTaskDelay(2000 / portTICK_PERIOD_MS);


    //创建服务端套接字 描述服务端类型，并创建服务端代号
    int socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    //描述通信接口：AF_INET:IPV4，SOCK_STREAM:字节流，IPPROTO_TCP:TCP协议
    
    if (socket_server == -1)
    {
        printf("创建套接字失败\n");
        return;
    }

    //绑定服务端套接字，并连接服务端
    struct sockaddr_in server_msg;
    //指明ip和端口号，明确服务端的位置，并和服务端建立连接
    server_msg = (struct sockaddr_in)
    {
        .sin_family = AF_INET,                          //IPV4
        .sin_port = htons(30000),                       //端口号
        .sin_addr.s_addr = inet_addr("192.168.31.14")
    };

    connect(socket_server, (struct sockaddr*)&server_msg, sizeof(server_msg));

    //收发数据
    send(socket_server, "hello world", 11, 0);
    //给socker_server发送数据，数据为"hello world"，长度为11，标志为0
    int recv_count = recv(socket_server, recv_buf, 999, 0); //传入recv_buf首地址
    printf("接收到了%d个字节的数据\n内容为:%s\n", recv_count, recv_buf);

    
    while (1)
    {
        int recv_count = recv(socket_server, recv_buf, 999, 0);
        if (recv_count == 0)
        {
            close(socket_server); //断开连接
            //1.断线 下线 2.销毁套接字 回收资源（内存)
            printf("服务端下线\n");
            return;
        }
        else if (recv_count > 0)
        {
            recv_buf[recv_count] = 0; //字符串结尾 0: \0的ASCII码，无论接收到的数据多长，都在最后加上\0
            printf("接收到了%d个字节的数据\n内容为:%s\n", recv_count, recv_buf);
        }
    }
}

void wifi_sta_init(void)
{
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

}
