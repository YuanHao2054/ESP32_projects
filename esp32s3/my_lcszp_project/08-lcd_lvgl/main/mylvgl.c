#include "mylvgl.h"

// 液晶屏面板句柄，在lcd.c中定义
extern esp_lcd_panel_handle_t panel_handle;

extern esp_lcd_panel_io_handle_t io_handle;

// 触摸屏句柄
static esp_lcd_touch_handle_t tp;

static lv_disp_t *disp;      // 指向液晶屏
static lv_indev_t *disp_indev = NULL; // 指向触摸屏

// 日志信息标签
const static char *TAG_LVGL = "LVGL";

// 液晶屏初始化+添加lvgl接口
// 返回一个lv_disp_t类型的指针
// static lv_disp_t *bsp_display_lcd_init(void)
// {
//     // 调用lcd.c中的函数，初始化液晶屏
//     bsp_display_new();                             // 液晶屏驱动初始化
//     lcd_set_color(0xffff);                         // 设置整屏背景白色
//     esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示

//     // 液晶屏添加lvgl接口
//     ESP_LOGD(TAG_LVGL, "添加液晶屏驱动接口");
//     // 配置lvgl参数
//     const lvgl_port_display_cfg_t disp_cfg =
//         {
//             .io_handle = io_handle,
//             .panel_handle = panel_handle,
//             .buffer_size = BSP_LCD_DRAW_BUF_HEIGHT * BSP_LCD_H_RES, // LVGL缓存大小
//             //.buffer_size = 48 * 1024,
//             .double_buffer = false,                                 // 双缓存
//             .hres = BSP_LCD_H_RES,                                  // 水平分辨率
//             .vres = BSP_LCD_V_RES,                                  // 垂直分辨率
//             .monochrome = false,                                    // 是否单色

//             // 显示属性 必须和液晶屏初始化中的设置一样
//             .rotation =
//                 {
//                     .swap_xy = true,   // 是否交换xy
//                     .mirror_x = true,  // x方向是否镜像
//                     .mirror_y = false, // y方向是否镜像
//                 },
//             // 缓存选项 dma和spi ram不能同时为true
//             .flags =
//                 {
//                     .buff_dma = false,   // 是否使用DMA
//                     .buff_spiram = true, // 是否使用SPI RAM
//                 },
//         };

//     return lvgl_port_add_disp(&disp_cfg);
// }

static lv_disp_t *bsp_display_lcd_init(void)
{
    /* 初始化液晶屏 */
    bsp_display_new(); // 液晶屏驱动初始化
    lcd_set_color(0xffff); // 设置整屏背景白色
    esp_lcd_panel_disp_on_off(panel_handle, true); // 打开液晶屏显示

    /* 液晶屏添加LVGL接口 */
    ESP_LOGD(TAG_LVGL, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,   // LVGL缓存大小 
        .double_buffer = false, // 是否开启双缓存
        .hres = BSP_LCD_H_RES, // 液晶屏的宽
        .vres = BSP_LCD_V_RES, // 液晶屏的高
        .monochrome = false,  // 是否单色显示器
        /* Rotation的值必须和液晶屏初始化里面设置的 翻转 和 镜像 一样 */
        .rotation = {
            .swap_xy = true,  // 是否翻转
            .mirror_x = true, // x方向是否镜像
            .mirror_y = false, // y方向是否镜像
        },
        .flags = {
            .buff_dma = false,  // 是否使用DMA 注意：dma与spiram不能同时为true
            .buff_spiram = true, // 是否使用PSRAM 注意：dma与spiram不能同时为true
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

// 触摸屏初始哈
esp_err_t bsp_touch_new(esp_lcd_touch_handle_t *ret_touch)
{
    // 配置参数
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES,
        .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    //通过iic总线初始化触摸屏
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM, &tp_io_config, &tp_io_handle), TAG_LVGL, "");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, ret_touch));

    return ESP_OK;
}


// 触摸屏初始化+添加LVGL接口
static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp)
{
    /* 初始化触摸屏 */
    ESP_ERROR_CHECK(bsp_touch_new(&tp));
    assert(tp);

    /* 添加LVGL接口 */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

//lvgl显示初始化
void bsp_lvgl_start(void)
{
    //初始化lvgl
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG(); 
    lvgl_port_init(&lvgl_cfg);
    /* 初始化液晶屏 并添加LVGL接口 */
    disp = bsp_display_lcd_init(); 
    /* 初始化触摸屏 并添加LVGL接口 */
    disp_indev = bsp_display_indev_init(disp); 
    /* 打开液晶屏背光 */
    bsp_display_backlight_on();
    
}
