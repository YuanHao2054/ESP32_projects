/**
 * @file lv_templ.h
 *
 */

#ifndef HX8369_H
#define HX8369_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#include "esp_lcd_panel_interface.h"
#include "esp_lcd_types.h"
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "sdkconfig.h"

#define LCD_H_RES 480
#define LCD_V_RES 800
/*********************
 *      DEFINES
 *********************/

// Supported alignment: 16, 32, 64. A higher alignment can enables higher burst transfer size, thus a higher i80 bus throughput.
#define PSRAM_DATA_ALIGNMENT 64
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern      lv_disp_drv_t disp_drv;      // contains callback functions
extern      esp_lcd_panel_handle_t panel_handle;
void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void    hx8369_init(void);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*HX8369_H*/