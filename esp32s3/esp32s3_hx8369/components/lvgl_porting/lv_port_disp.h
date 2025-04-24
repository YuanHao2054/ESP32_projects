/*
 * @Descripttion: 文件描述
 * @version: 文件版本
 * @Author: jinsc
 * @Date: 2022-09-05 20:39:23
 * @LastEditors: Jinsc
 * @LastEditTime: 2022-11-28 20:36:10
 * @FilePath: /lvgl_epd/components/lvgl_porting/lv_port_disp.h
 */
/**
 * @file lv_port_disp_templ.h
 *
 */

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 0

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include "epd_dev.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(void);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);
/* 另外启动一个任务用于刷新epd */
void init_epd_ref_task(void);
/* 强行全屏刷新 */
void epd_ref_all_next(void);
/**********************
 *      MACROS
 **********************/
extern EpdDevConfigDef *EpdDevConfig;;
extern lv_color_t lvgl_buf1_back[];     /*给墨水屏设置一个全屏缓存区*/
extern uint16_t   refresh_edp_cnt;      /* 用于判断屏幕是否需要刷新的标志位 使用该标志位纯粹是为了适配墨水屏的局部刷新特性，让墨水屏重绘完所有数据后再刷新屏幕 减少屏幕刷写次数 */
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_H*/

#endif /*Disable/Enable content*/
