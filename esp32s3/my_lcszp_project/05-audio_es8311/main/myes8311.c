#include "myes8311.h"

/* Import music file as buffer */
//canon_pcm_start和canon_pcm_end是通过音频文件转换工具生成的音频数据的起始地址和结束地址
extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");

//预定义的错误消息
static const char err_reason[][30] = {"input param is invalid",
                                      "operation timeout"
                                     };

//i2s发送通道句柄
i2s_chan_handle_t tx_handle = NULL;

// 初始化I2S外设
esp_err_t i2s_driver_init(void)
{
    /* 配置i2s发送通道 */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));
    /* 初始化i2s为std模式 并打开i2s发送通道 */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    return ESP_OK;
}

// 初始化I2C接口
esp_err_t bsp_i2c_init(void)
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ_HZ
    };
    i2c_param_config(BSP_I2C_NUM, &i2c_conf);

    return i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0);
}

// 初始化I2C接口 并初始化es8311芯片
esp_err_t es8311_codec_init(void)
{
    /* 初始化I2C接口 */
    ESP_ERROR_CHECK(bsp_i2c_init());

    /* 初始化es8311芯片 */
    es8311_handle_t es_handle = es8311_create(BSP_I2C_NUM, ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE
    };

    ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");

    return ESP_OK;
}

void i2s_music(void *args)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_write = 0;//用于存储音频数据长度
    //指针指向音乐文件的起始地址，用于在循环中读取音乐文件的数据
    uint8_t *data_ptr = (uint8_t *)music_pcm_start;

    /*在启用传输通道TX前线禁用它并加载数据，确保在传输通道启用后，数据可以立即进行有效传输*/
    ESP_ERROR_CHECK(i2s_channel_disable(tx_handle));  
    ESP_ERROR_CHECK(i2s_channel_preload_data(tx_handle, data_ptr, music_pcm_end - data_ptr, &bytes_write));
    /*
    参数解释：
    tx_handle：发送通道句柄
    data_ptr：音乐文件的起始地址，指向音频数据的指针
    music_pcm_end - data_ptr：音乐文件的大小，音频数据的大小，计算出要预加载的数据长度
    bytes_write：实际写入的数据长度
    */

    data_ptr += bytes_write; //更新数据指针，指向下一段音频数据的起始地址
    //启用传输通道TX
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    /*
    在一个无限循环里，通过i2s_channel_write函数将音频数据写入I2S发送通道
    将data_ptr指向的音频数据写入I2S发送通道
    写入数据的长度由music_pcm_end - data_ptr计算得出
    */
    
    while (1)
    {
        ret = i2s_channel_write(tx_handle, data_ptr, music_pcm_end - data_ptr, &bytes_write, portMAX_DELAY);
        /*
        参数解释
        tx_handle：发送通道句柄
        data_ptr：音乐文件的起始地址，指向音频数据的指针
        music_pcm_end - data_ptr：音乐文件的大小，音频数据的大小，计算出要写入的数据长度
        &bytes_write：实际写入的数据长度
        portMAX_DELAY：超时时间
        */
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "音乐数据I2S写入失败, %s", err_reason[ret == ESP_ERR_TIMEOUT]);
            abort();
        }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
        if(bytes_write > 0)
        {
            ESP_LOGI(TAG, "音乐数据I2S写入成功, 写入数据长度: %d", bytes_write);
        }
        else
        {
            ESP_LOGE(TAG, "音乐数据I2S写入失败");
            abort();
        }
        //再次将数据指针指向canon.pcm的起始地址，等待重复播放该音乐文件
        data_ptr = (uint8_t *)music_pcm_start; //更新数据指针，指向音频数据的起始地址

      
        //延时1s
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    
    //删除任务
    vTaskDelete(NULL);
}
