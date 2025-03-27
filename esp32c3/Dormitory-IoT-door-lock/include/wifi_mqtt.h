#ifndef  WIFI_MQTT_H
#define  WIFI_MQTT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h> //PubSubClient 作者 Nick O’Leary
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h> // 引入ArduinoJson库
#include "WiFiUdp.h"

//定义一个枚举型变量，表示门锁的状态
enum lockstate_t
{
    LOCKED = 0,
    UNLOCKED = 1
};

void wifi_init();
void Wifi_Connect();
void mqttCheckConnect();
void mqttcallback(char *topic, byte *payload, unsigned int length);
void judgemqttconnect();
void sendUDPMessage(const char *message);
void processJsonMessage(char *message);
void publock();


//在该c文件中定义的rtos的任务
void mqttCheckConnect(void *pvParameters);
void mqttcallback_process(void *pvParameters);


#endif // ! WIFI_MQTT_H
