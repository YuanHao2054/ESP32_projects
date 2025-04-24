#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"



#define EXAMPLE_STD_BCLK_IO1        GPIO_NUM_1      // I2S bit clock io number   I2S_BCLK
#define EXAMPLE_STD_WS_IO1          GPIO_NUM_4      // I2S word select io number    I2S_LRC
#define EXAMPLE_STD_DOUT_IO1        GPIO_NUM_2     // I2S data out io number    I2S_DOUT
#define EXAMPLE_STD_DIN_IO1         GPIO_NUM_NC    // I2S data in io number

#define EXAMPLE_BUFF_SIZE               2048
#define SAMPLE_RATE 44100

static i2s_chan_handle_t tx_chan;        // I2S tx channel handler


extern const uint8_t pcm_start[] asm("_binary_o_pcm_start");
extern const uint8_t pcm_end[]   asm("_binary_o_pcm_end");

static void i2s_example_write_task(void *args) {


    uint16_t *buffer = calloc(1, EXAMPLE_BUFF_SIZE * 2);


    size_t w_bytes = 0;
    uint32_t offset = 0;
    while (1) {
        /* Write i2s data */
        if (i2s_channel_write(tx_chan, buffer, EXAMPLE_BUFF_SIZE * 2, &w_bytes, 1000) == ESP_OK) {
            //printf("Write Task: i2s write %d bytes\n", w_bytes);
        } else {
            //printf("Write Task: i2s write failed\n");
        }
        if (offset>(pcm_end-pcm_start)){
            break;
        }

        for (int i = 0; i < EXAMPLE_BUFF_SIZE; i++) {
            offset++;
            buffer[i] = pcm_start[offset]<<3;
        }
        //printf("size %d\noffset %lu\n", pcm_end-pcm_start,offset);

    }
    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));

    vTaskDelete(NULL);
}



static void i2s_example_init_std_simplex(void) {
    printf("I2S_NUM_AUTO = %d\n", I2S_NUM_AUTO);
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(1, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));


    i2s_std_config_t tx_std_cfg = {
            .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
            .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                        I2S_SLOT_MODE_MONO),

            .gpio_cfg = {
                    .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
                    .bclk = EXAMPLE_STD_BCLK_IO1,
                    .ws   = EXAMPLE_STD_WS_IO1,
                    .dout = EXAMPLE_STD_DOUT_IO1,
                    .din  = EXAMPLE_STD_DIN_IO1,
                    .invert_flags = {
                            .mclk_inv = false,
                            .bclk_inv = false,
                            .ws_inv   = false,
                    },
            },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
}



#define EXAMPLE_I2S_DUPLEX_MODE CONFIG_USE_DUPLEX

#define CONFIG_EXAMPLE_BIT_SAMPLE 32
#define CONFIG_EXAMPLE_SAMPLE_RATE 44100
#define NUM_CHANNELS (1) // For mono recording only!
#define SAMPLE_SIZE (CONFIG_EXAMPLE_BIT_SAMPLE * 32)
#define BYTE_RATE (CONFIG_EXAMPLE_SAMPLE_RATE * (CONFIG_EXAMPLE_BIT_SAMPLE / 8)) * NUM_CHANNELS

#define EXAMPLE_STD_BCLK_IO1_RX GPIO_NUM_8  // I2S bit clock io number
#define EXAMPLE_STD_WS_IO1_RX GPIO_NUM_3    // I2S word select io number
#define EXAMPLE_STD_DOUT_IO1_RX GPIO_NUM_NC // I2S data out io number
#define EXAMPLE_STD_DIN_IO1_RX GPIO_NUM_18  // I2S data in io number

// static i2s_chan_handle_t tx_chan; // I2S tx channel handler
static i2s_chan_handle_t rx_chan; // I2S rx channel handler

int32_t r_buf[SAMPLE_SIZE + 32];

static void i2s_example_read_task(void *args)
{

    size_t r_bytes = 0;
    /* ATTENTION: The print and delay in the read task only for monitoring the data by human,
     * Normally there shouldn't be any delays to ensure a short polling time,
     * Otherwise the dma buffer will overflow and lead to the data lost */
    while (1)
    {
        /* Read i2s data */
        if (i2s_channel_read(rx_chan, r_buf, sizeof(int32_t) * SAMPLE_SIZE, &r_bytes, 1000) == ESP_OK)
        {
            i2s_channel_write(tx_chan, r_buf, EXAMPLE_BUFF_SIZE * 2, &r_bytes, 1000);

            // printf("Read Task: i2s read %d bytes\n-----------------------------------\n", r_bytes);
            // printf("[0] %x [1] %x [2] %x [3] %x\n[4] %x [5] %x [6] %x [7] %x\n\n", r_buf[0], r_buf[1], r_buf[2],r_buf[3], r_buf[4], r_buf[5], r_buf[6], r_buf[7]);
             //printf("3=%d\n", r_bytes);
            // int samples_read = r_bytes / sizeof(int32_t);
            // printf("samples_read=%d\n", samples_read);
            // for (int i = 0; i < samples_read; i++)
            // {

            //     printf("1=%ld\n", r_buf[i]);
            //     // printf("2=%d\n", i);
            //     // printf("%d\n", a);
            //     //  vTaskDelay(pdMS_TO_TICKS(500));
            //     //  printf("%d\r\n", 1);
            //     // vTaskDelay(pdMS_TO_TICKS(500));
            // }
            //esp_task_wdt_reset(); TODO
        }
        else
        {
            printf("Read Task: i2s read failed\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    // free(r_buf);
    vTaskDelete(NULL);
}

static void i2s_example_init_std_duplex(void)
{
    /* Setp 1: Determine the I2S channel configuration and allocate both channels
     * The default configuration can be generated by the helper macro,
     * it only requires the I2S controller id and I2S role */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    /* Step 2: Setting the configurations of standard mode, and initialize rx & tx channels
     * The slot configuration and clock configuration can be generated by the macros
     * These two helper macros is defined in 'i2s_std.h' which can only be used in STD mode.
     * They can help to specify the slot and clock configurations for initialization or re-configuring */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_EXAMPLE_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                .mclk = I2S_GPIO_UNUSED, // some codecs may require mclk signal, this example doesn't need it
                .bclk = EXAMPLE_STD_BCLK_IO1_RX,
                .ws = EXAMPLE_STD_WS_IO1_RX,
                .dout = I2S_GPIO_UNUSED,
                .din = EXAMPLE_STD_DIN_IO1_RX, // In duplex mode, bind output and input to a same gpio can loopback
                // internally
                .invert_flags =
                    {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv = false,
                    },
            },
    };
    /* Initialize the channels */
    // ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    /* Default is only receiving left slot in mono mode,
     * update to right here to show how to change the default configuration */
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
}

void app_main(void) {
    i2s_example_init_std_simplex();

    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    // // /* Step 4: Create writing and reading task */
    xTaskCreate(i2s_example_write_task, "i2s_example_write_task", 4096, NULL, 5, NULL);


    
    // #if EXAMPLE_I2S_DUPLEX_MODE
    i2s_example_init_std_duplex();
    
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    /* Step 4: Create writing and reading task */
    // TODO Task watchdog got triggered
    xTaskCreate(i2s_example_read_task, "i2s_example_read_task", 4096 * 2, NULL, tskIDLE_PRIORITY, NULL);
}
