#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h> 
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h> 
#include "wifi_mqtt.h"

#define USE_MULTCORE 0 // 使用多核心

// 功能引脚宏定义  锁门的是key1，开门的是key2
#define LED 3  //LED在wifi_mqtt.cpp中也有定义，需要一并修改
#define KEY1 18
#define KEY2 19
#define SERVO 6

// 声明信号量句柄
// 两个按键对应的二值信号量
SemaphoreHandle_t xSem_key1 = NULL;
SemaphoreHandle_t xSem_key2 = NULL;

// 创建一个舵机对象
Servo servo;

// 创建进行阿里云下发信息解析的二值信号量的句柄
SemaphoreHandle_t xSemAliyun = NULL;

// 全局变量声明
extern lockstate_t lockstate_flag;
extern char message[256];

// rtos任务
// 轮询方式读取按键状态，如果对应按键按下，释放对应的二值信号量
void button_Read(void *pvParameters)
{
  // 创键key1 key2的二值信号量
  xSem_key1 = xSemaphoreCreateBinary();
  xSem_key2 = xSemaphoreCreateBinary();
  while (true)
  {
    // 读取按键状态
    if (digitalRead(KEY2) == LOW)
    {
      // 软件消抖
      vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
      if (digitalRead(KEY2) == LOW)
      {
        // 每次按键按下，释放控制舵机的二值信号量
        xSemaphoreGive(xSem_key2);
        // 打印 提示释放了二值信号量
        // Serial.println("xSem_key2 has been released");
        sendUDPMessage("key2 has been released\n");
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
      }
    }

    if (digitalRead(KEY1) == LOW)
    {
      // 软件消抖
      vTaskDelay(10 / portTICK_PERIOD_MS); // 10ms
      if (digitalRead(KEY1) == LOW)
      {
        // 每次按键按下，释放控制舵机的二值信号量
        xSemaphoreGive(xSem_key1);
        // 打印 提示释放了二值信号量
        // Serial.println("xSem_key1 has been released");
        sendUDPMessage("key1 has been released\n");
        vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
  }
}

// rtos任务
// 判断或者执行锁门操作
void lock_door(void *pvParameters)
{
  while (true)
  {
    if (xSemaphoreTake(xSem_key1, portMAX_DELAY) == pdTRUE)
    {
      // Serial.println("lock_door");
      lockstate_flag = LOCKED;
      // printf("lockstate_flag = %d\n", lockstate_flag);

      // 发送udp消息
      char temp_mes[64];
      sprintf(temp_mes, "lock door %d\n", lockstate_flag);
      sendUDPMessage(temp_mes);

      // 给服务器发送锁门信息
      publock();
      // 指示灯灭
      digitalWrite(LED, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
  }
}

// rtos任务
// 判断或者执行开门操作
void open_door(void *pvParameters)
{
  while (true)
  {
    if (xSemaphoreTake(xSem_key2, portMAX_DELAY) == pdTRUE)
    {
      // 判断标志位lockstate_flag是否为0，如果为0，表示门未锁，可以开门
      if (lockstate_flag == UNLOCKED)
      {
        servo.write(115);
        vTaskDelay(2000 / portTICK_PERIOD_MS); // 2s
        servo.write(0);
        sendUDPMessage("open door\n");
      }
      else
      {
        // Serial.println("开门失败");
        sendUDPMessage("开门失败\n");
        // 指示灯连闪三次，表示开门失败，闪完后灭
        for (int i = 0; i < 3; i++)
        {
          digitalWrite(LED, HIGH);
          vTaskDelay(200 / portTICK_PERIOD_MS); // 500ms
          digitalWrite(LED, LOW);
          vTaskDelay(200 / portTICK_PERIOD_MS); // 500ms
        }
      }
      vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); // 100ms
  }
}

// rtos任务
// 使用udp向电脑发送调试信息 兼顾每5秒根据lockstate_flag的值调整led的状态
void sendUDPMessage_task(void *pvParameters)
{
  while (true)
  {
    sendUDPMessage("esp32-c3 is normally running\n");

    // 发送lockstate_flag的值
    if (lockstate_flag == LOCKED)
    {
      // sendUDPMessage("lockstate_flag = LOCKED\n");
      sendUDPMessage("门锁已锁\n");
      digitalWrite(LED, LOW);
    }
    else if (lockstate_flag == UNLOCKED)
    {
      // sendUDPMessage("lockstate_flag = UNLOCKED\n");
      sendUDPMessage("门已解锁\n");
      digitalWrite(LED, HIGH);
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5s
  }
}


// 配置各个引脚，连接WiFi，连接MQTT服务器，创建任务
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  // 初始化按键，默认上拉输入，在进程中读取按键状态 低电平表示按下
  pinMode(KEY1, INPUT_PULLUP); // 这个按键用于开门
  pinMode(KEY2, INPUT_PULLUP); // 这个按键用于锁门

  // io5作为指示灯，用于在外面提示门锁是否打开
  pinMode(LED, OUTPUT);
  // 初值为低电平
  digitalWrite(LED, LOW);

  // 创建二值信号量，用于阿里云下发信息解析任务
  xSemAliyun = xSemaphoreCreateBinary();

  // 指定舵机引脚
  servo.attach(SERVO);

  // 初始化WiFi、MQTT、UDP
  wifi_init();

  Serial.println("setup");
  sendUDPMessage("系统已启动\n");

#if !USE_MULTCORE // 如果不使用多核，使用rtos创建任务

  xTaskCreate(button_Read, "button_Read", 1024, NULL, 5, NULL);               // 用于轮询读取两个按键的状态
  xTaskCreate(lock_door, "lock_door", 1024, NULL, 4, NULL);                   // 用于判断或者执行锁门操作
  xTaskCreate(open_door, "open_door", 1024, NULL, 4, NULL);                   // 用于判断或者执行开门操作
  xTaskCreate(mqttCheckConnect, "mqttCheckConnect", 4 * 1024, NULL, 1, NULL); // 用于检测mqtt连接状态
  xTaskCreate(mqttcallback_process, "mqttcallback", 1024, NULL, 3, NULL);     // 用于处理mqtt接收到的信息
  xTaskCreate(sendUDPMessage_task, "sendUDPMessage", 1024, NULL, 2, NULL);    // 用于发送udp消息

#else // 使用多核
  xTaskCreatePinnedToCore(task1, "task1", 2048, NULL, 3, NULL, 0); // 0号核心
  xTaskCreatePinnedToCore(task2, "task2", 1024, NULL, 2, NULL, 0); // 1号核心
  xTaskCreatePinnedToCore(task3, "task3", 2048, NULL, 4, NULL, 0); // 0号核心 检测按键状态的任务，优先级最高
  // 创建进行信息解析的任务
  xTaskCreatePinnedToCore(task4, "task4", 1024, NULL, 1, NULL, 1);

#endif
}

void loop()
{
#if 0
  sendUDPMessage("hello world");
  vTaskDelay(1000 / portTICK_PERIOD_MS); // 1s
  Serial.print("loop");
  sendUDPMessage("舵机测试\n");
  servo.write(115);
  vTaskDelay(2000 / portTICK_PERIOD_MS); // 2s
  servo.write(0);
  vTaskDelay(2000 / portTICK_PERIOD_MS); // 2s

#endif
}
