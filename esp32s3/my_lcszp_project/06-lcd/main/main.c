#include <stdio.h>
#include "lcd.h"
#include "pca9557.h"
#include "yingwu.h"
#include "logo_en_240x240_lcd.h"


void app_main(void)
{
    bsp_i2c_init();  // I2C初始化
    pca9557_init();  // IO扩展芯片初始化
    bsp_lcd_init();  // 液晶屏初始化
    //lcd_draw_pictrue(0, 0, 240, 240, logo_en_240x240_lcd); // 显示乐鑫logo图片
    vTaskDelay(2000 / portTICK_PERIOD_MS); // 延时2s
    lcd_draw_pictrue(0, 0, 320, 240, gImage_yingwu); // 显示3只鹦鹉图片

}
