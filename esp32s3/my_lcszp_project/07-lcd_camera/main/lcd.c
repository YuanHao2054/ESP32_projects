#include "lcd.h"

const static char *TAG_LCD = "LCD";

//背光pwm初始化
esp_err_t bsp_display_brightness_init(void)
{
    //为pwm背光控制初始化ledc外设
    const ledc_channel_config_t LCD_backlight_channel = 
    {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE, //不使用中断
        .timer_sel = 1, //使用timer1
        .duty = 0, //初始化占空比
        .hpoint = 0, 
        .flags.output_invert = true
    };
    //初始化定时器
    const ledc_timer_config_t LCD_backlight_timer = 
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,//10位分辨率
        .timer_num = 1, //使用timer1
        .freq_hz = 5000, //5kHz
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

//背光亮度设置 有效范围0-100
esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    //限制范围
    if (brightness_percent < 0)
    {
        brightness_percent = 0;
    }
    else if (brightness_percent > 100)
    {
        brightness_percent = 100;
    }
    ESP_LOGI(TAG_LCD, "设置背光亮度为 %d%%", brightness_percent);

    //亮度数据为0到100，10位分辨率的占空比为0到1023，所以要进行转换
    uint32_t duty_cycle = (1023 * brightness_percent) / 100;
    //更新占空比
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));

    return ESP_OK;
}

// 关闭背光，亮度设置为0
esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

// 打开背光 最亮，亮度设置为100
esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

//定义一个存储LCD面板的句柄
esp_lcd_panel_handle_t panel_handle = NULL;

//LCD初始化
esp_err_t bsp_display_new(void)
{
    esp_lcd_panel_io_handle_t io_handle = NULL; //定义一个存储LCD面板IO的句柄
    esp_err_t ret = ESP_OK;

    //背光初始化
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG_LCD, "背光初始化失败");

    //初始化spi总线
    ESP_LOGD(TAG_LCD, "初始化SPI总线");
    const spi_bus_config_t buscfg =
    {
        .sclk_io_num = BSP_LCD_SPI_CLK,
        .mosi_io_num = BSP_LCD_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG_LCD, "SPI总线初始化失败");

    //初始化LCD面板IO 
    ESP_LOGD(TAG_LCD, "初始化LCD面板IO");
    const esp_lcd_panel_io_spi_config_t io_config =
    {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_SPI_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 2,
        .trans_queue_depth = 10,
    };
    /*
    参数解释
    dc_gpio_num: 数据/命令选择引脚
    cs_gpio_num: 片选引脚
    pclk_hz: 像素时钟频率
    lcd_cmd_bits: 命令位数
    lcd_param_bits: 参数位数
    spi_mode
    trans_queue_depth: 传输队列深度
    */
   ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle), err, TAG_LCD, "面板io配置失败");

   //初始化lcd驱动芯片st7789
   ESP_LOGD(TAG_LCD, "初始化LCD驱动芯片");
   const esp_lcd_panel_dev_config_t panel_config = 
   {
    .reset_gpio_num = BSP_LCD_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
   };
   /*
   参数解释
    reset_gpio_num: 复位引脚
    rgb_ele_order: 颜色顺序
    bits_per_pixel: 每个像素的位数
   */
   ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle), err, TAG_LCD, "New panel failed");

    esp_lcd_panel_reset(panel_handle);  // 液晶屏复位
    lcd_cs(0);  // 拉低CS引脚
    esp_lcd_panel_init(panel_handle);  // 初始化配置寄存器
    esp_lcd_panel_invert_color(panel_handle, true); // 颜色反转
    esp_lcd_panel_swap_xy(panel_handle, true);  // 显示翻转 
    esp_lcd_panel_mirror(panel_handle, true, false); // 镜像

    return ret;

err:
    if (panel_handle) {
        esp_lcd_panel_del(panel_handle);
    }
    if (io_handle) {
        esp_lcd_panel_io_del(io_handle);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

//设置液晶屏颜色
void lcd_set_color(uint16_t color)
{
    //分配内存，在这里分配液晶屏一行数据需要的大小
    uint16_t *buffer = (uint16_t *)heap_caps_malloc(BSP_LCD_H_RES * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    /*
    heap_caps_malloc从指定内存区域分配内存'
    大小为显示屏的水平长度 乘以 每个像素的大小（2字节）
    其他参数设置分配内存的属性 8位对齐 优先从外部SPI RAM分配
    
    分配完成后，指针buffer指向分配的内存
    */
    if (buffer == NULL)
    {
        ESP_LOGE(TAG_LCD, "内存分配不足，分配失败");
    }
    else
    {
        for (size_t i = 0; i < BSP_LCD_H_RES; i++) //遍历每个像素，往缓存里写入数据
        {
            buffer[i] = color;
        }
        for (int y = 0; y < BSP_LCD_V_RES; y++) //遍历每一行，往液晶屏写入数据
        {
            //绘制一行的像素数据，逐行更新
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, BSP_LCD_H_RES, y+1, buffer);
            //0, y: 起始坐标 BSP_LCD_H_RES, y+1: 终止坐标
            //整屏刷新为同一个颜色，所以只用一个buffer
        }
        free(buffer); //释放内存
    }


}

//LCD显示初始化
esp_err_t bsp_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    ret = bsp_display_new();
    //lcd_set_color(0x0000); //黑色
    lcd_set_color(0xffff); //白色

    ret = esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示
    ret = bsp_display_backlight_on(); // 打开背光显示

    return  ret;

}

//显示图片
void lcd_draw_pictrue(int x_start, int y_start, int x_end, int y_end, const unsigned char *gImage)
{
    //为图片分配内存，大小为图片的长乘以宽，再乘以2（每个像素2字节）
    size_t pixels_byte_size = (x_end - x_start) * (y_end - y_start) * 2;
    //指定再外部SPIRAM中分配
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG_LCD, "内存不足，分配失败");
        return;
    }
    //将图片数据写入内存
    memcpy(pixels, gImage, pixels_byte_size); 

    //绘制图片
    esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_end, y_end, (uint16_t *)pixels);
    heap_caps_free(pixels);  // 释放内存
}

