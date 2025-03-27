#include <stdio.h>
#include <string.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define BSP_SD_CLK          (47)
#define BSP_SD_CMD          (48)
#define BSP_SD_D0           (21)

static const char *TAG = "main";

#define MOUNT_POINT              "/sdcard" //挂载点
#define EXAMPLE_MAX_CHAR_SIZE    64

// 写文件内容 path是路径 data是内容
static esp_err_t s_example_write_file(const char *path, char *data) //文件路径 要写入的内容
{
    //打印调试信息
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w"); //打开文件
    /*
    fopen 参数 
    path 文件路径
    "w" 文件打开方式：如果文件不存在，创建文件；如果文件存在，清空文件内容
    */
    if (f == NULL) //如果文件打开失败
    {
        //打印错误信息
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    //将格式化的字符串写入文件
    fprintf(f, data); //将内容写入文件
    //fprintf(f, "%s", data); //将内容写入文件
    fclose(f); //关闭文件
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

// 读文件内容 path是路径
static esp_err_t s_example_read_file(const char *path)
{
    //打印调试信息
    ESP_LOGI(TAG, "Reading file %s", path);
    //以只读的方式打开文件
    FILE *f = fopen(path, "r");
    if (f == NULL) //如果文件打开失败
    {
        //打印错误信息
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    //定义一个缓冲区，用于存储读取的内容
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f); //读取文件内容
    fclose(f); //关闭文件

    //去除字符串中的换行符
    char *pos = strchr(line, '\n'); //查找换行符的位置
    if (pos)
    {
        *pos = '\0'; //将换行符替换为空字符
    }
    //打印读取的内容
    ESP_LOGI(TAG, "Read from file: '%s'", line);
    return ESP_OK;
}



void app_main(void)
{
    //定义一个esp_err_t类型的变量，用于存储函数返回值
    esp_err_t ret;
·
    //定义sd卡挂载选项
    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = true, //如果挂载失败是否格式化sd卡 是
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT; //挂载点

    //打印调试信息
    ESP_LOGI(TAG, "Initializing SD card 初始化SD卡");
    ESP_LOGI(TAG, "Using SDMMC peripheral 使用SDMMC外设");
    
    //初始化SDMMC主机
    sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // SDMMC主机接口配置
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT(); // SDMMC插槽配置
    slot_config.width = 1; // 1位数据线
    slot_config.clk = BSP_SD_CLK;//设置SDMMC主机的引脚
    slot_config.cmd = BSP_SD_CMD;
    slot_config.d0 = BSP_SD_D0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP; //内部上拉

    ESP_LOGI(TAG, "Mounting file system 挂载文件系统");

    //挂载SD卡
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    /*
    参数解释：
    mount_point: 挂载点 例如：/sdcard 路径
    host: SDMMC主机接口配置
    slot_config: SDMMC插槽配置 包含数据线、时钟线、命令线等
    mount_config: 挂载选项 包含是否格式化、最大文件数、分配单元大小等
    card: SD卡信息

    如果挂载成功，返回ESP_OK，否则返回错误码
    */

    if (ret != ESP_OK)
    {
        //打印错误信息
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }

    //打印挂载成功的信息
    ESP_LOGI(TAG, "SD card mount successfully 挂载成功");
    sdmmc_card_print_info(stdout, card); //在终端打印SD卡信息

    //新建一个txt文件，用指针指向这个文件
    const char *file_hello = MOUNT_POINT"/你好hellotest.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    //将内容写入缓冲区
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "你好hello先生", card->cid.name);
 
    // 打印MOUNT_POINT路径下的所有文件
    //list_files(MOUNT_POINT);

    //写文件
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK)
    {
        //打印错误信息
        ESP_LOGE(TAG, "Failed to write file");
        return;
    }

    //打开txt文件，读内容
    ret = s_example_read_file(file_hello);
    if (ret != ESP_OK)
    {
        //打印错误信息
        ESP_LOGE(TAG, "Failed to read file");
        return;
    }

    //卸载SD卡
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted 卸载成功");


}
