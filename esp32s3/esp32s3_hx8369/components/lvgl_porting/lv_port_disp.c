/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 0

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "bsp_board.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void set_px_cb(struct _lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
static void rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

static void epd_ref_start(void);
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
static lv_color_t *buf_1;                              /*A buffer for 10 rows*/
/**********************
 *      MACROS
 **********************/
EpdDevConfigDef *EpdDevConfig;
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    // /* Example for 1) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    buf_1 = malloc_ram(EpdDevConfig->x_size*EpdDevConfig->y_size / 8);
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, EpdDevConfig->x_size*EpdDevConfig->y_size); /*Initialize the display buffer*/
    /* Example for 2) */
    // static lv_disp_draw_buf_t draw_buf_dsc_2;
    // static lv_color_t buf_2_1[DISP_BUF_SIZE];                        /*A buffer for 10 rows*/
    // static lv_color_t buf_2_2[DISP_BUF_SIZE];                        /*An other buffer for 10 rows*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, DISP_BUF_SIZE);   /*Initialize the display buffer*/

    // /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    // static lv_disp_draw_buf_t draw_buf_dsc_3;
    // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
    //                       MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/


    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = EpdDevConfig->y_size;
    disp_drv.ver_res = EpdDevConfig->x_size;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    disp_drv.full_refresh = 1;/* 这里针对墨水屏启用全屏刷新 */

    disp_drv.set_px_cb  = &set_px_cb;//注册一个单点写入的函数
    disp_drv.rounder_cb = &rounder_cb;//四舍五入要重绘的区域的坐标（2x2 像素可以转换为 2x8） 单色屏中该函数比较重要

    // disp_drv.sw_rotate = 1;
    // disp_drv.rotated = LV_DISP_ROT_90;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    EpdDevConfig = epd_init();
    epd_hw_init(EpdDevConfig);  //Electronic paper initialization
    epd_white_screen_white(EpdDevConfig);                            //Show all white
    epd_deep_sleep(EpdDevConfig);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
    /* 预计300ms刷新完毕一次 所以设置300ms刷新周期 后续需要将刷新单独启动一个任务 通过消息队列进行数据发送 */
    epd_ref_start();/* 请求刷新 */
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    //lv_disp_flush_ready(&disp_drv);
}
#if 1 //这是两种屏幕的选择 
/*
    下方代表y轴一个byte像素点排序，当屏幕为这种排序时使用此宏定义 例如EPD
    7
    6
    5
    4
    3
    2
    1
    0
*/
#define BIT_SET(a, b) ((a) |= (1U << (7 - b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1U << (7 - b)))
#else
/*
    下方代表y轴一个byte像素点排序，当屏幕为这种排序时使用此宏定义 例如sd1306
    如果配合逆序填充，再加上打开这个显示则可实现墨水屏方向控制
    0
    1
    2
    3
    4
    5
    6
    7
*/
#define BIT_SET(a, b) ((a) |= (1U << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1U << (b)))
#endif
/* 本函数是对单个字节进行拼装 */
static void set_px_cb(struct _lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa)
{
    uint16_t byte_index = x + ((y >> 3) * buf_w);
    uint8_t bit_index = y & 0x7;
    // == 0 inverts, so we get blue on black
    if (color.full != 0)
    {
        BIT_SET(buf[byte_index], bit_index);
    }
    else
    {
        BIT_CLEAR(buf[byte_index], bit_index);
    }
}
/* 本函数是将Y轴方形步进切换为8bit的作用 */
static void rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    area->y1 = (area->y1 & (~0x7));
    area->y2 = (area->y2 & (~0x7)) + 7;
}
/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}
/* 专用与edp的刷新任务 */
static EventGroupHandle_t ref_xEventGroup;
#define EPD_REF_ALL_CHK_NUM     (1)     /* 全屏刷新一次后会进行这个次数的局部刷新 */
#define EPD_REF_ALL_NUM         (100)   /* 局部刷新这个次数会进行一次全局刷新 */
/* 定义事件位的意义 */
#define EPD_REF_START_TASK_BIT ( 1UL << 0UL )
#define EPD_REF_ALL_NEXT_BIT   ( 1UL << 1UL )

static void epd_ref_start(void)
{
    xEventGroupSetBits( ref_xEventGroup, EPD_REF_START_TASK_BIT );
}
/* 下一次将会全屏刷新 */
void epd_ref_all_next(void)
{
    xEventGroupSetBits( ref_xEventGroup, EPD_REF_ALL_NEXT_BIT );
}

static void reverseArray(uint8_t arr[], int size) 
{
    int start = 0;        // 开始指针
    int end = size - 1;   // 结束指针

    while (start < end) 
    {
        // 交换开始和结束指针处的元素
        uint8_t temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;

        // 移动指针
        start++;
        end--;
    }
}
static void lv_epd_ref(void *arg)
{
    uint16_t ref_num = 0;   /* 全屏刷新计数器  */
    bool ref_next = false;  /* 全屏刷新计数器  */
    EventBits_t xEventGroupValue;
	const EventBits_t xBitsToWaitFor = ( EPD_REF_START_TASK_BIT|EPD_REF_ALL_NEXT_BIT );
    //vTaskDelay(pdMS_TO_TICKS(10))
    while(1)
    {
        xEventGroupValue = xEventGroupWaitBits( ref_xEventGroup,/* 事件组的句柄 */
												xBitsToWaitFor, /* 待收取的事件位 */
												pdTRUE,         /* 满足添加时清除上面的事件位 */
												pdFALSE,        /* 任意事件位被设置就会退出阻塞态 */
												portMAX_DELAY );/* 没有超时 */
        if( ( xEventGroupValue & EPD_REF_ALL_NEXT_BIT ) != 0 )
        {
            ref_next = true;
        }
        if( ( xEventGroupValue & EPD_REF_START_TASK_BIT ) != 0 )
        {
            ref_num++;
            if(ref_num > EPD_REF_ALL_NUM || ref_next == true)
            {
                ref_num = 0;
                ref_next = false;
                epd_hw_reset(EpdDevConfig);;              //Electronic paper initialization
                epd_white_screen_white(EpdDevConfig);   //Show all white
                epd_deep_sleep(EpdDevConfig);
            }
            //printf( "epf_ref\r\n" );
            epd_hw_reset(EpdDevConfig);
            uint8_t *epd_buf = (uint8_t *)buf_1;
            epd_set_win_size(EpdDevConfig, 0, 0, EpdDevConfig->x_size, EpdDevConfig->y_size );
            epd_spi_write_data_buff(EpdDevConfig,  epd_buf, EpdDevConfig->x_size*EpdDevConfig->y_size / 8);
            epd_part_update(EpdDevConfig);
            lv_disp_flush_ready(&disp_drv);
            epd_deep_sleep(EpdDevConfig);
        }

    }
}
void init_epd_ref_task(void)
{
    ref_xEventGroup = xEventGroupCreate();
    xTaskCreate(lv_epd_ref, "lv_epd_ref",1024*2,NULL,5,NULL);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
