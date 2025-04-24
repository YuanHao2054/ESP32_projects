#include "driver/i2s.h"
#include "esp_websocket_client.h"
#include "cJSON.h"
#include "opus.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define MAX98375_BCLK_IO1 5
#define MAX98375_LRCLK_IO1 4
#define MAX98375_DOUT_IO1 6
#define SAMPLE_RATE 16000
#define FRAME_DURATION 60
#define bufferLen (SAMPLE_RATE*FRAME_DURATION/1000)

esp_websocket_client_handle_t handle; // 定义websocket句柄
void websocket_sent_data(esp_websocket_client_handle_t ws_client, uint8_t *data_byte_stream, size_t data_len);

QueueHandle_t audio_queue; // 创建消息队列

/* 创建开始结束Json对象 */

void send_start() {
    cJSON *start = cJSON_CreateObject(); // 创建一个json对象

    /* 添加json内容 */
    cJSON_AddStringToObject(start, "type", "listen");
    cJSON_AddStringToObject(start, "mode", "manual");
    cJSON_AddStringToObject(start, "state", "start");

    char *start_json_string = cJSON_Print(start);
    cJSON_Delete(start);

    int sent = esp_websocket_client_send_text(handle, // 客户端句柄
                                              start_json_string, // 要发送的二进制数据指针（内容）
                                              strlen(start_json_string), // 长度
                                              portMAX_DELAY); // 超时时间
    if (sent < 0) {
        ESP_LOGE("WebSocket", "Failed to send start message");
    }
    cJSON_free(start_json_string);
}

void send_end() {
    cJSON *end = cJSON_CreateObject(); // 创建一个json对象

    /* 添加json内容 */
    cJSON_AddStringToObject(end, "type", "listen");
    cJSON_AddStringToObject(end, "mode", "manual");
    cJSON_AddStringToObject(end, "state", "stop");

    char *end_json_string = cJSON_Print(end);
    cJSON_Delete(end);

    int sent = esp_websocket_client_send_text(handle, // 客户端句柄
                                              end_json_string, // 要发送的二进制数据指针（内容）
                                              strlen(end_json_string), // 长度
                                              portMAX_DELAY); // 超时时间
    if (sent < 0) {
        ESP_LOGE("WebSocket", "Failed to send end message");
    }
    cJSON_free(end_json_string);
}

/*****************************************I2S(输出)配置********************************************************/

// 事件处理函数
void Init_max98357_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // 设置为发送模式
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // 如果是立体声，可以改为I2S_CHANNEL_FMT_STEREO
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    i2s_pin_config_t pin1_config = {
        .bck_io_num = 40, // BCLK引脚号
        .ws_io_num = 39, // LRCK引脚号
        .data_out_num = 41, // DATA_OUT引脚号，用于输出
        .data_in_num = -1 // 不需要输入引脚
    };
    i2s_set_pin(I2S_NUM_1, &pin1_config);
    i2s_start(I2S_NUM_1);
}

/*****************************************I2S(输出)配置********************************************************/


/*****************************************I2S(inmp441)配置********************************************************/

// 事件处理函数
void Init_inmp441_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false // 分配中断标志
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_pin_config_t pin_config = {
        .bck_io_num = 37, // BCLK引脚号
        .ws_io_num = 36, // LRCK引脚号
        .data_out_num = -1, // DATA引脚号
        .data_in_num = 38 // DATA_IN引脚号
    };
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_start(I2S_NUM_0);
}

/*****************************************I2S(inmp441)配置********************************************************/

static int16_t i2s_buffer[bufferLen];
/*****************************************音频获取任务********************************************************/
void i2s_read_task(void *pvParameters) {
    size_t bytes_read;
    while (1) {
        i2s_read(I2S_NUM_0, // I2s接口编号
                 i2s_buffer, // 存储读取到的数据
                 bufferLen * sizeof(int16_t), // 读取的数据大小
                 &bytes_read, // 指向一个变量的指针，该变量用于存储实际读取的字节数。如果这个参数为 NULL，则不会返回实际读取的字节数。
                 portMAX_DELAY); // 等待时间
        // 处理读取到的音频数据
        if (xQueueSend(audio_queue, i2s_buffer, portMAX_DELAY) != pdPASS) {
            printf("Failed to send data to queue\r\n");
        }
    }
}
/*****************************************音频获取任务********************************************************/

static const char *TAG = "PCM_to_Opus";
#define MAX_PACKET_SIZE 4000 // 最大的OPUS数据包大小
uint8_t encoded_data[MAX_PACKET_SIZE];

/*****************************************Opus配置********************************************************/

OpusEncoder *encoder = NULL;
// 创建并且初始化Opus（编码器）
esp_err_t initialize_opus_encoder() {
    int error;
    encoder = opus_encoder_create(16000, // 采样率
                                  1, // 声道设置
                                  OPUS_APPLICATION_AUDIO, // 应用场景（质量）
                                  &error); // 错误指针
    if (error != OPUS_OK) {
        ESP_LOGE(TAG, "Opus编码器创建失败: %s", opus_strerror(error));
        return ESP_FAIL;
    }
    return ESP_OK;
}
OpusDecoder *decoder = NULL;
// 创建并且初始化Opus（解码器）
esp_err_t initialize_opus_decoder() {
    int error;
    decoder = opus_decoder_create(16000, // 采样率
                                  1, // 声道设置
                                  &error); // 错误指针
    if (error != OPUS_OK) {
        ESP_LOGE(TAG, "Opus解码器创建失败: %s", opus_strerror(error));
        return ESP_FAIL;
    }
    return ESP_OK;
}

/*****************************************Opus配置********************************************************/

/*****************************************音频处理任务********************************************************/

static int16_t audio_samples[bufferLen];
void audio_processing_task(void *pvParameters) {
    while (1) {
        //如果按键被按下发送开始指令没有被按下正常处理但服务器不识别
        send_start();
        TickType_t start_time = xTaskGetTickCount(); // 获取当前时间

        while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(5000)) {
            // 从队列中接收音频数据
            if (xQueueReceive(audio_queue, audio_samples, portMAX_DELAY)) {
                int samples_per_frame = FRAME_DURATION * SAMPLE_RATE / 1000;
                int encoded_size = opus_encode(encoder, // 编码器指针
                                               audio_samples, // 数据来源
                                               samples_per_frame, // 每帧的样本数
                                               (unsigned char *)encoded_data, // 编码后的 Opus 数据存储位置
                                               MAX_PACKET_SIZE);
                if (encoded_size < 0) {
                    ESP_LOGE("Opus", "Opus编码失败: %s", opus_strerror(encoded_size));
                    continue;
                }
                int sent = esp_websocket_client_send_bin(handle, (const char *)encoded_data, encoded_size, portMAX_DELAY);
                if (sent < 0) {
                    ESP_LOGE("WebSocket", "Failed to send encoded data");
                }
            }
        }
        //如果start才会end,否则两个cmd都不发送
        send_end();
        printf("赶紧说话\r\n");
    }
}
/*****************************************音频处理任务********************************************************/

/*****************************************wifi配置********************************************************/
static TaskHandle_t client_task_handle;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data) {
    if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            printf("got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        }
    } else if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            vTaskDelete(client_task_handle);
        } else if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        }
    }
}

void wifi_init_sta(void) {
    /* 初始化WiFi协议栈 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Eweb",
            .password = "12345678",
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
/*****************************************wifi配置********************************************************/

/*****************************************websocket********************************************************/

static const char *WEB_TAG = "websocket_client";

#define WEBSOCKET_URI "ws://192.168.10.10:8000/xiaozhi/v1/"
// #define WEBSOCKET_URI "ws://122.228.26.226:29448/xiaozhi/v1/"
// 给服务发送配置消息
void send_hello_msg(void) {
    cJSON *root = cJSON_CreateObject(); // 创建一个json对象

    /* 添加json内容 */
    cJSON_AddStringToObject(root, "type", "hello");
    cJSON_AddNumberToObject(root, "version", 1);
    cJSON_AddStringToObject(root, "transport", "websocket");

    cJSON *audio_params = cJSON_CreateObject();
    cJSON_AddStringToObject(audio_params, "format", "opus");
    cJSON_AddNumberToObject(audio_params, "sample_rate", 16000);
    cJSON_AddNumberToObject(audio_params, "channels", 1);
    cJSON_AddNumberToObject(audio_params, "frame_duration", 60);

    cJSON_AddItemToObject(root, "audio_params", audio_params);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);

    if (json_string == NULL) {
        printf("create json obj falut\r\n");
        return;
    }
    int sent = esp_websocket_client_send_text(handle, // 客户端句柄
                                              json_string, // 要发送的二进制数据指针（内容）
                                              strlen(json_string), // 长度
                                              portMAX_DELAY); // 超时时间
    if (sent > 0) {
        printf("hello msg send successfully\r\n");
    } else {
        printf("falut!!!!!!\r\n");
    }
    cJSON_free(json_string);
}
short pcm[960];
// WebSocket事件处理函数
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(WEB_TAG, "WebSocket connected");
            send_hello_msg();
            xTaskCreate(audio_processing_task, "audio_processing_task", 1024 * 30, NULL, 5, NULL);
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(WEB_TAG, "WebSocket disconnected");
            break;
        case WEBSOCKET_EVENT_DATA:
         if (data->op_code == 2) {
                ESP_LOGI(TAG, "Received binary data of length: %d", data->data_len);

                int samples_decoded =   opus_decode(decoder, 
                                        (unsigned char *)data->data_ptr, 
                                        data->data_len, 
                                        pcm, 
                                        960, 
                                        0);

                if(samples_decoded>0){
                    size_t written_bytes;
                    // 给输出设备发送解码后的数据
                    i2s_write(  I2S_NUM_1, //i2s接口
                                pcm, //数据来源
                                960 * sizeof(int16_t), //数据大小
                                &written_bytes, //数据大小的返回值
                                portMAX_DELAY);
                    // printf("OK!\r\n");
                }
            }
            // ESP_LOGI(WEB_TAG, "Received data: %.*s", data->data_len, (char *)data->data_ptr);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(WEB_TAG, "WebSocket error");
            break;
        default:
            break;
    }
}

// 初始化WebSocket客户端
esp_websocket_client_handle_t the_websocket_init() {
    const esp_websocket_client_config_t ws_cfg = {
        .uri = WEBSOCKET_URI,
        .task_stack = 8192,
    };
    esp_websocket_client_handle_t client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, client);
    esp_websocket_client_start(client);
    return client;
}

void websocket_sent_data(esp_websocket_client_handle_t ws_client, uint8_t *data_byte_stream, size_t data_len) {
    // 准备要发送的字节流数据
    int sent = esp_websocket_client_send_bin(ws_client, // 客户端句柄
                                             (const char *)data_byte_stream, // 要发送的二进制数据指针（内容）
                                             data_len, // 长度
                                             portMAX_DELAY); // 超时时间
    if (sent > 0) {
        ESP_LOGI(WEB_TAG, "Sent %d bytes of binary data", sent);
    } else {
        ESP_LOGE(WEB_TAG, "Failed to send binary data");
    }
}

/*****************************************websocket********************************************************/

void app_main() {

    // 初始化nvs
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    // 连接wifi
    wifi_init_sta();
    handle = the_websocket_init(); // 初始化websocket句柄
    Init_inmp441_i2s();//初始化麦克风
    Init_max98357_i2s();//初始化喇叭

    // 初始化opus编码器
    if (initialize_opus_encoder() != ESP_OK) {
        return;
    }

    // 初始化opus解码器
    if (initialize_opus_decoder() != ESP_OK) {
        return;
    }

    // 创建音频数据队列
    audio_queue = xQueueCreate(5, sizeof(int16_t) * bufferLen); // 队列可以存储 10 个 bufferLen 大小的数据
    if (audio_queue == NULL) {
        printf("Failed to create queue\r\n");
        return;
    }

    xTaskCreate(i2s_read_task, "i2s_read_task", 4096, NULL, 5, NULL);
    // xTaskCreate(audio_processing_task, "audio_processing_task", 16384, NULL, 5, NULL);

    while (1) {
        vTaskDelay(100);
    }
}