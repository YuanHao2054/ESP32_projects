#include "wifi_mqtt.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cJSON.h>

// 功能引脚宏定义
#define LED 3


// 要连接的WiFi网络
#define WIFI_SSID "虚空终端"
#define WIFI_PASS "503503503"

// 电脑上位机信息，用于调试
const char *udp_address = "192.168.31.14"; // 电脑的IP地址
const int udp_port = 1234;

/* 线上环境域名和端口号 */
#define MQTT_SERVER "a1sejDZS1W1.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define MQTT_PORT 1883 // 端口号
#define MQTT_USERNAME "esp-01s&a1sejDZS1W1"
#define CLIENT_ID "esp|securemode=3,signmethod=hmacsha1,timestamp=100|"
#define MQTT_PASSWORD "9F644999EB0E46F9462E38D8BA5507C6C15A70CC"

// 发布消息用的topic
#define PubTopic "/sys/a1sejDZS1W1/esp-01s/thing/event/property/post"
#define SubTopic "/sys/a1sejDZS1W1/esp-01s/thing/service/property/set"


// 创建一个mqtt客户端
WiFiClient espClient;
PubSubClient client(espClient);

// 创建一个udp客户端
WiFiUDP udp_esp32;



// 定义一个枚举型变量，表示门锁的状态
lockstate_t lockstate_flag = LOCKED;


// 存储接收到的消息
char message[256];

// 接收消息后的回调函数释放信号量，在任务中进行解析
extern SemaphoreHandle_t xSemAliyun;

//================================================================================================

// 初始化wifi的各类配置
void wifi_init()
{
    // 连接WiFi
    Wifi_Connect();

    // 连接MQTT服务器
    mqttCheckConnect();

    // 初始化udp客户端
    udp_esp32.begin(udp_port);

    // 打印设备的IP地址
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

// 连接到WiFi
void Wifi_Connect()
{
    WiFi.mode(WIFI_STA); // 配置成sta模式
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");

    // 连接到MQTT服务器
    client.setServer(MQTT_SERVER, MQTT_PORT); /* 连接WiFi之后，连接MQTT服务器 */
    client.setCallback(mqttcallback);         /* 设置回调函数 */
}

// 连接到服务器
void mqttCheckConnect()
{
    while (!client.connected())
    {
        Serial.println("Connecting to MQTT Server...");
        if (client.connect(CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
        {
            Serial.println("MQTT Connected");
        }
        else
        {
            Serial.print("MQTT Connect err: ");
            Serial.println(client.state());
            delay(5000);
        }
    }
}

void mqttcallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    // 将接收到的消息转换为字符串，供void processJsonMessage(char *message)函数解析
    memcpy(message, payload, length);

    // 释放二值信号量
    xSemaphoreGive(xSemAliyun);
}

// 在一个进程中判断mqtt是否连接，如果没有连接则进行连接
void judgemqttconnect()
{
    if (!client.connected())
    {
        mqttCheckConnect();
    }
    client.loop();
}

// 在按下外面的按键关门时，发布关门的消息,lockstate_flag为0
void publock()
{
    // 创建一个足够大的StaticJsonDocument对象来存储预期的JSON数据
    StaticJsonDocument<256> doc;

    // 设置JSON数据
    doc["id"] = "123";
    doc["version"] = "1.0";
    doc["sys"]["ack"] = 0;
    doc["params"]["lockstate"] = 0;
    doc["method"] = "thing.event.property.post";

    // 序列化JSON数据为字符串
    char jsonOutput[256];
    serializeJson(doc, jsonOutput);
    // 打印json字符串
    Serial.println(jsonOutput);

    // 使用修改后的pubMsg函数发送JSON字符串
    if (client.publish(PubTopic, jsonOutput))
    {
        Serial.println("Publish success");
        // 连接成功后订阅主题
        client.subscribe(SubTopic);
    }
    else
    {
        Serial.println("Publish fail");
    }
}

// 订阅阿里云下发的信息
void processJsonMessage(char *message)
{

    StaticJsonDocument<256> doc; // 调整大小以适应您的需要
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // 从JSON中提取信息
    const char *method = doc["method"];         // "thing.service.property.set"
    const char *id = doc["id"];                 // "248166957"
    int lockstate = doc["params"]["lockstate"]; // 1 or 0
    const char *version = doc["version"];       // "1.0.0"

    Serial.print("Method: ");
    Serial.println(method);
    Serial.print("ID: ");
    Serial.println(id);
    Serial.print("lockstate: ");
    Serial.println(lockstate);
    Serial.print("Version: ");
    Serial.println(version);

    // 根据lockstate的值执行操作
    if (lockstate == 1)
    {
        // 如果lockstate为1，执行相应操作
        // digitalWrite(LED_PIN, HIGH); // 示例：打开LED
        lockstate_flag = LOCKED;
        //Serial.println("door opened");
        sendUDPMessage("阿里云下发：门已解锁\n");
    }
    else
    {
        // 如果lockstate为0，执行相应操作
        // digitalWrite(LED_PIN, LOW); // 示例：关闭LED
        lockstate_flag = UNLOCKED;
        //Serial.println("door closed");
        sendUDPMessage("阿里云下发：门锁已锁\n");
    }
}

// udp发送数据
void sendUDPMessage(const char *message)
{
    udp_esp32.beginPacket(udp_address, udp_port);
    udp_esp32.write(reinterpret_cast<const uint8_t*>(message), strlen(message));
    udp_esp32.endPacket();
}


// rtos任务
// 检测mqtt连接状态
void mqttCheckConnect(void *pvParameters)
{
    while (true)
    {
        judgemqttconnect();
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
    }
}

// rtos任务
// 处理mqtt接收到的信息
void mqttcallback_process(void *pvParameters)
{
    while (true)
    {
        if (xSemaphoreTake(xSemAliyun, portMAX_DELAY) == pdTRUE)
        {
            Serial.println("task4");

            // 解析信息

            // 在这个函数中，根据服务器下发的数据，更改lockstate_flag的值
            processJsonMessage(message);

            // 根据当前lockstate_flag的值，确定指示灯的状态
            if (lockstate_flag == LOCKED)
            {
                digitalWrite(LED, LOW);
            }
            else
            {
                digitalWrite(LED, HIGH);
            }

            Serial.println("xSemServo has been taken");
            vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
