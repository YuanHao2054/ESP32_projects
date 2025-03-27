#include <stdio.h>
#include "lcd.h"
#include "pca9557.h"
#include "yingwu.h"
#include "camera.h"

void app_main(void)
{
    bsp_i2c_init();  // I2C初始化
    pca9557_init();  // IO扩展芯片初始化
    bsp_lcd_init();  // 液晶屏初始化
    lcd_draw_pictrue(0, 0, 320, 240, gImage_yingwu); // 显示3只鹦鹉图片
    vTaskDelay(500 / portTICK_PERIOD_MS);  // 延时500毫秒
    bsp_camera_init(); // 摄像头初始化
    app_camera_lcd(); // 让摄像头画面显示到LCD上

}
