/*
 * @Author: Jinsc
 * @Date: 2022-11-13 13:47:15
 * @LastEditors: Jinsc
 * @LastEditTime: 2022-11-28 20:50:42
 * @FilePath: /lvgl_epd/components/lvgl_porting/lv_task.c
 * @Description: 
 * Copyright (c) 2022 by jinsc123654@gmail.com All Rights Reserved. 
 */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_demos.h"
#include "lv_task.h"
#include "lvgl_helpers.h"
#include "esp_lcd_panel_interface.h"
#define TAG "lvgl task"
/*用定时器给LVGL提供时钟*/
static void lv_tick_task(void *arg)
{
	(void)arg;
	lv_tick_inc(10);
}
/* 其他线程如果向lvgl发送回调 需要使用这个带锁的函数 否则会出现lvgl异常 */
SemaphoreHandle_t xGuiSemaphore;
void lvglMsgSend(int msgId, void * magData)
{
    if (xGuiSemaphore != NULL && pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))/* 等待lvgl线程获取锁 */
    {
        lv_msg_send(msgId, magData);
        xSemaphoreGive(xGuiSemaphore);
    }
}
void lv_task(void *arg)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    xGuiSemaphore = xSemaphoreCreateMutex();
    
    lv_init();
    lvgl_driver_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;
    // buf1 = heap_caps_aligned_alloc(PSRAM_DATA_ALIGNMENT, LCD_H_RES * 480 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // buf2 = heap_caps_aligned_alloc(PSRAM_DATA_ALIGNMENT, LCD_H_RES * 480 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    buf1 = heap_caps_malloc(LCD_V_RES * 50 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = heap_caps_malloc(LCD_V_RES * 50 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    assert(buf1);
    assert(buf2);
    ESP_LOGI(TAG, "buf1@%p, buf2@%p", buf1, buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_V_RES * 50);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    // lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    lv_disp_drv_register(&disp_drv);

#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif
    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 10 * 1000));
    ESP_LOGI(TAG, "Turn on LCD backlight");


    while(1)
    {
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_timer_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}