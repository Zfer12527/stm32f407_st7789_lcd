# STM32F407 项目模板

这是适用于 STM32F407 的项目模板，同时该模板使用 STM32CubeMX 生成外设的初始化代码

当前适配目标：2.0 英寸 ST7789、240×320、8 针 SPI 屏，使用竖屏配置。照片可辨认 GND、VDD、SCL、SDA、RST、DC、CS、BL，引脚按下面的信号名连接。

例程功能：接入项目内的 LVGL 8.3.11，显示运行时间、自动循环的 0～100% 进度条和每秒闪烁一次的界面状态灯。示例界面使用英文和内置 Montserrat 字体，无需触摸屏或按键。

液晶屏接线（SDA/SCL 为屏幕上的串行外设接口 SPI 信号名称）：

| 屏幕引脚及含义 | STM32F407 引脚 |
| --- | --- |
| SDA / MOSI（串行数据输入） | PB5 |
| SCL / SCK（串行时钟） | PA5 |
| CS（片选） | PA4 |
| DC（命令与数据选择） | PA3 |
| RST（复位） | PA2 |
| BL（背光） | PA1（定时器 2 通道 2，即 TIM2_CH2；5 kHz 脉宽调制 PWM，默认 50% 占空比） |
| GND（地） | GND |
| VDD（电源） | 3.3V |

SPI1 使用单线主机发送、8 位数据、最高有效位（MSB）先发、模式 0，时钟为 10.5 MHz（APB2 84 MHz / 8）。屏幕使用 CS、DC、SCL、SDA 四线通信；硬件抽象层（HAL）的单线方向设置表示只使用 MOSI 发送，不影响屏幕的四线协议。SDA/MOSI 已迁移到 PB5，以避开 PA7 上的板载灯；PA7 配置为未使用的模拟输入。

背光由 PA1/TIM2_CH2 硬件脉宽调制（PWM）控制。APB1 定时器时钟为 84 MHz，预分频寄存器 PSC=83、自动重装载寄存器 ARR=199，得到 5 kHz；屏幕初始化时通道 2 的捕获/比较寄存器 CCR2=0，完成后 CCR2=100，即 50% 占空比。工程未包含 HAL 定时器驱动，当前直接使用微控制器软件接口标准（CMSIS）提供的寄存器定义配置 TIM2，无需中断。`LCD.h` 的 `ST7789_BACKLIGHT_PERCENT` 设置默认占空比，`LCD_Backlight_SetDuty(0～100)` 可在运行时调整。

BL 必须连接 PA1 才能调光。如果此前为排查闪烁而将 BL 直接接了 3.3V，请先断开这条连接，再将 BL 接回 PA1，避免 PWM 低电平输出与电源直连。

LVGL 对接位置：

- `app.c`：调用 `lv_init()`、`lv_port_disp_init()`，创建界面；每 5 ms 左右调用一次 `lv_timer_handler()`。100 ms 的 LVGL 定时器更新进度条，运行时间和状态灯每秒更新一次。
- `cubemx_source/LVGL/examples/porting/lv_port_disp.c`：注册 240×320 显示器，使用单块 20 行缓冲区（9600 字节）。刷新回调将整个变化区域交给 `ST7789_Blit()`，SPI 发送完成后调用 `lv_disp_flush_ready()`。
- `cubemx_source/Core/Src/LCD.c`：`ST7789_Blit()` 连续发送区域内的 RGB565 数据，自动处理 HAL 单次传输的 16 位长度上限。
- `cubemx_source/LVGL/lv_conf.h`：16 位色深，`LV_COLOR_16_SWAP=1`，确保缓冲区按高字节在前发送；通过 `HAL_GetTick()` 直接提供毫秒时钟，无需在系统滴答（SysTick）中另加 `lv_tick_inc()`。保留 48 KiB LVGL 内存池，启用 14/20/28 号字体。

当前使用阻塞 SPI 和局部刷新，没有启用直接存储器访问（DMA）或整屏帧缓冲。后续修改示例可从 `app.c` 的 `demo_create()`（界面）和 `demo_update()`（动态数据）入手；不要在界面运行期间再直接调用画点/清屏覆盖 LVGL 的显示内容。

驱动已换成通用 ST7789 SPI 初始化：硬件/软件复位、退出休眠、RGB565、扫描方向、反色模式、正常显示和开显示。参考 [ST7789 数据手册](https://dl.espressif.com/dl/schematics/ST7789V_SPEC_V1.0.pdf)及 [Adafruit 通用初始化](https://github.com/adafruit/Adafruit-ST7735-Library/blob/master/Adafruit_ST7789.cpp)，使用控制器恢复的面板默认电源/伽马（Gamma）参数。

LCD.h 中集中配置 ST7789_WIDTH、ST7789_HEIGHT、ST7789_X_OFFSET、ST7789_Y_OFFSET、ST7789_MADCTL 和 ST7789_INVERSION_ON。当前为 240×320、零偏移、红绿蓝（RGB）颜色顺序的竖屏，开启常见 IPS 反色模式。若后续规格或实际颜色不同，在这里调整；驱动芯片不同则需要替换对应初始化。旧并口屏的电阻跳线要求和旧商家面板参数已移除。

若背光持续每组三次快闪，表示本机 HAL SPI 发送失败；调试器可查看 ST7789_LastSpiStatus 和 hspi1.ErrorCode。发送成功仅说明单片机（MCU） 完成传输，不能确认屏幕已接收数据。若背光或界面没有正常启动，需先核对烧录、BL 接线和系统时钟配置（当前依赖 8 MHz 外部晶振）。

# 注意事项

- 在 STM32CubeMX 中修改了组件之后（比如添加 FreeRTOS 支持，比如将库从 HAL 更换为 LL），记得查看构建脚本 Makefile 中**宏定义(C_DEFS)**的变化，并及时更新到工程中

