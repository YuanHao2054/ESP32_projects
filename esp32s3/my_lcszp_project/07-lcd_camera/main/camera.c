#include "camera.h"

const char *TAG_CA = "camera";

// lcd面板句柄，用于lcd显示任务
extern esp_lcd_panel_handle_t panel_handle;

// 定义一个消息队列句柄，用于摄像头向lcd传递图像数据
static QueueHandle_t lcd_queue = NULL;

// 摄像头硬件初始化
void bsp_camera_init(void)
{
    dvp_pwdn(0); // 打开摄像头 用pca9557 io拓展芯片，使能摄像头的硬件

    // 配置摄像头的硬件参数
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_1; // LEDC通道选择  用于生成XCLK时钟 但是S3不用
    config.ledc_timer = LEDC_TIMER_1;     // LEDC timer选择  用于生成XCLK时钟 但是S3不用
    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;
    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;
    config.pin_sccb_sda = -1; // 这里写-1 表示使用已经初始化的I2C接口
    config.pin_sccb_scl = CAMERA_PIN_SIOC;
    config.sccb_i2c_port = 0;
    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA;
    //config.frame_size = FRAMESIZE_5MP;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    esp_err_t err = esp_camera_init(&config); // 配置上面定义的参数
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_CA, "摄像头初始化失败 0x%x", err);
        return;
    }

    // 获取摄像头的型号
    sensor_t *s = esp_camera_sensor_get();

    if (s->id.PID == GC0308_PID)
    {
        s->set_hmirror(s, 1); // 设置水平镜像
    }
}

// 摄像头处理任务 当摄像头采集到一帧图像数据后，将数据传递给lcd显示
static void task_process_camera(void *arg)
{
    const char *TAG_FPS = "fps";
    int frame_count = 0;
    TickType_t start_time = xTaskGetTickCount();
    while (true)
    {
        camera_fb_t *frame = esp_camera_fb_get(); // 获取一帧图像数据
        if (frame != NULL)
        {
            xQueueSend(lcd_queue, &frame, portMAX_DELAY); // 将图像数据传递给lcd显示
            frame_count++;

            //每秒计算一次帧率
            TickType_t current_time = xTaskGetTickCount();
            if ((current_time - start_time) >= pdMS_TO_TICKS(1000))
            {
                float elapsed_time = (current_time - start_time) * portTICK_PERIOD_MS; // 计算经过的时间（毫秒）
                float fps = (float)frame_count * 1000 / elapsed_time; // 计算帧率
                ESP_LOGI(TAG_FPS, "Frame rate: %.2f FPS", fps);
                frame_count = 0;
                start_time = current_time;
            }
        }
    }
}

// lcd显示函数
static void task_process_lcd(void *arg)
{
    camera_fb_t *frame = NULL;
    while (true)
    {
        if (xQueueReceive(lcd_queue, &frame, portMAX_DELAY) == pdTRUE)
        {
            //lcd_draw_pictrue(0, 0, 320, 240, (unsigned char *)frame->buf); // 显示图像数据
            esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, frame->width, frame->height, (uint16_t *)frame->buf);
            esp_camera_fb_return(frame); // 释放图像数据
        }
    }
}

// 让摄像头显示到LCD
void app_camera_lcd(void)
{
    // 创建队列
    lcd_queue = xQueueCreate(3, sizeof(camera_fb_t *));
    // 创建任务 分别放到不同的核心上
    xTaskCreatePinnedToCore(task_process_camera, "task_process_camera", 3 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_process_lcd, "task_process_lcd", 4 * 1024, NULL, 5, NULL, 0);
}