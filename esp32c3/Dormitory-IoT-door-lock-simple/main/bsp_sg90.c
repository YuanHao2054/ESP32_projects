/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：club.szlcsc.com
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-15     LCKFB-lp     first version
 */
 #include "bsp_sg90.h"

 unsigned int Servo_Angle = 0;//舵机角度
 
 /******************************************************************
  * 函 数 名 称：SG90_Init
  * 函 数 说 明：PWM配置
  * 函 数 形 参： pre定时器时钟预分频值    per周期
  * 函 数 返 回：无
  * 作       者：LC
  * 备       注：PWM频率=80 000 000 /( (pre+1) * (per+1) )
 ******************************************************************/
 void SG90_Init(void)
 {
     // 准备并应用PWM定时器配置
     ledc_timer_config_t ledc_timer = {
         .speed_mode       = PWM_MODE,           //低速模式
         .timer_num        = PWMA_TIMER,         //通道的定时器源    定时器0
         .duty_resolution  = LEDC_DUTY_RES,      //将占空比分辨率设置为12位
         .freq_hz          = LEDC_FREQUENCY,     // 设置输出频率为50 kHz
         .clk_cfg          = LEDC_AUTO_CLK       //设置LEDPWM的时钟来源 为自动
         //LEDC_AUTO_CLK = 启动定时器时，将根据给定的分辨率和占空率参数自动选择源时钟
     };
     ledc_timer_config(&ledc_timer);
 
     // 准备并应用LEDC1 PWM通道配置
     ledc_channel_config_t ledc_channel = {
         .speed_mode     = PWM_MODE,             //低速模式
         .channel        = SG90_CHANNEL,         //通道0
         .timer_sel      = PWMA_TIMER,           //定时器源 定时器0
         .intr_type      = LEDC_INTR_DISABLE,    //关闭中断
         .gpio_num       = GPIO_SIG,             //输出引脚  GPIO1
         .duty           = 0,                    // 设置占空比为0
         .hpoint         = 0
     };
     ledc_channel_config(&ledc_channel);
 }
 
 /******************************************************************
  * 函 数 名 称：Set_Servo_Angle
  * 函 数 说 明：设置角度
  * 函 数 形 参：angle=要设置的角度，范围0-180
  * 函 数 返 回：无
  * 作       者：LC
  * 备       注：无
 ******************************************************************/
 void Set_Servo_Angle(unsigned int angle)
 {
     unsigned int ServoAngle = 0;
     float min = 101.375;
     float max = 510.875;
 
     //设置的角度超过180度
     if( angle > 180 )
     {
         return ;
     }
 
     //保存设置的角度
     Servo_Angle = angle;
 
     //换算角度
     ServoAngle = (unsigned int)( min + ( (float)angle * 2.2777 ));
     printf("ServoAngle = %d \r\n",ServoAngle);
 
     ledc_set_duty(PWM_MODE, SG90_CHANNEL, ServoAngle);
     ledc_update_duty(PWM_MODE, SG90_CHANNEL);
 }
 
 /******************************************************************
  * 函 数 名 称：读取当前角度
  * 函 数 说 明：Get_Servo_Angle
  * 函 数 形 参：无
  * 函 数 返 回：当前角度
  * 作       者：LC
  * 备       注：使用前必须确保之前使用过
                 void Set_Servo_Angle(unsigned int angle)
                 函数设置过角度
 ******************************************************************/
 unsigned int Get_Servo_Angle(void)
 {
         return Servo_Angle;
 }