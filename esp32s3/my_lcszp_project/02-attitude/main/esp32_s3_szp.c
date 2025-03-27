#include "esp32_s3_szp.h"


static const char *TAG = "esp32_s3_szp"; //定义一个TAG，用于打印信息

// 初始化IIC
esp_err_t bsp_i2c_init(void)
{
    i2c_config_t i2c_conf = 
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_I2C_FREQ_HZ

    };
    i2c_param_config(I2C_NUM_0, &i2c_conf); //初始化到i2c0
    return i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

// 读取QMI8658寄存器
esp_err_t qmi8658_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    //读取指定寄存器的数据
    return i2c_master_write_read_device(I2C_NUM_0, QMI8658_IIC_ADDR, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
}

// 写入QMI8658寄存器
esp_err_t qmi8658_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    //写入数据缓冲区
    uint8_t buf[2] = {reg_addr, data};
    //写入指定寄存器的数据
    return i2c_master_write_to_device(I2C_NUM_0, QMI8658_IIC_ADDR, buf, sizeof(buf), 1000 / portTICK_PERIOD_MS);
}

//初始化qmi8658
void qmi8658_init(void)
{
    uint8_t id = 0;

    qmi8658_register_read(QMI8658_WHO_AM_I, &id, 1); //读取id
    while (id != 0x05)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        qmi8658_register_read(QMI8658_WHO_AM_I, &id, 1);
    }
    ESP_LOGI(TAG, "QMI8658 OK");

    //初始化qmi8658
    qmi8658_register_write_byte(QMI8658_RESET, 0xb0);  // 复位  
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms
    qmi8658_register_write_byte(QMI8658_CTRL1, 0x40); // CTRL1 设置地址自动增加
    qmi8658_register_write_byte(QMI8658_CTRL7, 0x03); // CTRL7 允许加速度和陀螺仪
    qmi8658_register_write_byte(QMI8658_CTRL2, 0x95); // CTRL2 设置ACC 4g 250Hz
    qmi8658_register_write_byte(QMI8658_CTRL3, 0xd5); // CTRL3 设置GRY 512dps 250Hz 
}

//读取加速度和陀螺仪寄存器的值
void qmi8658_read_AccAndGry(t_sQMI8658 *p) //传入陀螺仪数据结构体指针
{
    uint8_t status, data_ready=0;
    int16_t buf[6]; //数据接收缓冲区

    qmi8658_register_read(QMI8658_STATUS0, &status, 1); //读取状态寄存器
    if (status & 0x03)
       data_ready = 1;
    if (data_ready == 1)//如果数据可读
    {
        data_ready = 0;
        qmi8658_register_read(QMI8658_AX_L, (uint8_t *)buf, 12); // 读加速度和陀螺仪值
        //将读取到的数据存入结构体
        p->acc_x = buf[0];
        p->acc_y = buf[1];
        p->acc_z = buf[2];
        p->gyr_x = buf[3];
        p->gyr_y = buf[4];
        p->gyr_z = buf[5];
    }
}

//根据读取到的数据计算角度
void qmi8658_fetch_angleFromAcc(t_sQMI8658 *p)
{
    //计算角度
    float temp;

    qmi8658_read_AccAndGry(p); //读取加速度和陀螺仪数据
    temp = (float)p->acc_x / sqrt( ((float)p->acc_y * (float)p->acc_y + (float)p->acc_z * (float)p->acc_z) );
    p->AngleX = atan(temp)*57.29578f; // 180/π=57.29578
    temp = (float)p->acc_y / sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_z * (float)p->acc_z) );
    p->AngleY = atan(temp)*57.29578f; // 180/π=57.29578
    temp = sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_y * (float)p->acc_y) ) / (float)p->acc_z;
    p->AngleZ = atan(temp)*57.29578f; // 180/π=57.29578

}







