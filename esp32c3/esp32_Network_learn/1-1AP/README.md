# ESP32配置AP模式
## 一、启动流程  
#### 1、启动nvs分区  
esp32的nvs分区专门用来存储键值对，头文件需要包含nvs_flash.h

#### 2、创建默认事件  
**esp32默认启用freeRTOS**  
ESP32 默认事件循环是 FreeRTOS 中用于处理系统事件的机制。它的主要作用是管理和分发系统事件，例如 Wi-Fi 连接、断开连接、IP 地址分配等。通过使用默认事件循环，开发者可以简化事件处理的流程，不需要手动创建和管理事件队列。

以下是 ESP32 默认事件循环的一些关键点：

* 事件注册：开发者可以注册事件处理程序来处理特定的系统事件。
    ```c
    // 注册事件处理程序 STA接入事件
    //如果读到一个结构体，base是IP_EVENT，id是IP_EVENT_AP_STAIPASSIGNED时，调用WIFI_Callback函数
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &WIFI_Callback, NULL, NULL); // 注册wifi事件处理函数
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &WIFI_Callback, NULL, NULL); // 注册当STA设备断开时的事件处理函数
    ```  
    当触发事件时，将调用对应的回调函数，进行处理 

* 事件分发：当系统事件发生时，默认事件循环会自动调用相应的事件处理程序。
* 简化开发：使用默认事件循环可以减少代码复杂度，让开发者专注于应用逻辑。  

#### 3、配置WiFi为AP模式  
**初始化网卡的底层配置**
```c
esp_netif_init();
```
**以默认的方式创建一个AP类型的网卡**
```c
esp_netif_create_default_wifi_ap();
```
**初始化WiFi**
```c
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);
```
esp_wifi_init参数用宏配置，该宏为官方给的默认wifi配置

**配置wifi模式**
```c
esp_wifi_set_mode(WIFI_MODE_AP);
```
参数为枚举型变量：
```c
typedef enum {
    WIFI_MODE_NULL = 0,  /**< null mode */
    WIFI_MODE_STA,       /**< WiFi station mode */
    WIFI_MODE_AP,        /**< WiFi soft-AP mode */
    WIFI_MODE_APSTA,     /**< WiFi station + soft-AP mode */
    WIFI_MODE_NAN,       /**< WiFi NAN mode */
    WIFI_MODE_MAX
} wifi_mode_t;
```
**配置AP参数**
```c
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
```  
**启动wifi**  
默认函数
```c
esp_wifi_start();
```

## 二、事件处理  

