#include "app.h"  // 引入应用头文件，间接获取 STM32 HAL 的类型和函数声明。
#include "LCD.h"  // 引入屏幕驱动头文件，获取屏幕宽度等配置宏。
#include "lv_port_disp.h"  // 引入 LVGL 显示接口头文件，同时获取 LVGL 的控件和定时器接口。

static lv_obj_t *uptime_label;  // 保存运行时间标签的指针，仅在当前文件中使用。
static lv_obj_t *progress_bar;  // 保存进度条控件的指针，供定时更新函数设置数值。
static lv_obj_t *progress_label;  // 保存百分比标签的指针，用于显示进度条的当前数值。
static lv_obj_t *status_led;  // 保存界面虚拟状态灯的指针；它不控制板载灯或屏幕背光。
static uint32_t demo_start_tick;  // 记录示例开始时的毫秒计数，用于计算界面运行时间。
static uint32_t last_seconds = UINT32_MAX;  // 记录上次显示的秒数；初值设为最大值，确保第一次调用会更新。
static int32_t last_progress = -1;  // 记录上次显示的进度；初值设为 -1，确保第一次会显示 0%。

static lv_obj_t *demo_label(lv_obj_t *parent, const char *text, int16_t x, int16_t y,  // 定义标签创建辅助函数，接收父对象、显示文本及相对父容器的位置。
                            uint32_t color, const lv_font_t *font)  // 继续声明标签参数：颜色采用十六进制 RGB 值，font 指定字体。
{  // 开始当前函数的代码块。
    lv_obj_t *label = lv_label_create(parent);  // 在指定父对象内创建标签，并保存返回的对象指针。
    lv_label_set_text(label, text);  // 设置标签文本，LVGL 会在内部复制这段字符串。
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);  // 将 RGB 数值转换为 LVGL 颜色，并设置标签默认状态的文字颜色。
    lv_obj_set_style_text_font(label, font, 0);  // 设置标签默认状态使用的字体。
    lv_obj_set_pos(label, x, y);  // 设置标签相对父容器内容区域的位置，单位为像素。
    return label;  // 返回创建的标签指针，便于后续修改文本或样式。
}  // 结束当前函数或控制流程的代码块。

static lv_obj_t *demo_card(lv_obj_t *parent, int16_t y)  // 定义卡片创建辅助函数，接收父对象和卡片的纵向位置。
{  // 开始当前函数的代码块。
    lv_obj_t *card = lv_obj_create(parent);  // 创建普通 LVGL 对象，作为卡片及其内部控件的容器。
    lv_obj_remove_style_all(card);  // 移除卡片的默认样式，随后手动配置背景、边框和圆角。
    lv_obj_set_pos(card, 12, y);  // 设置卡片横坐标为 12，纵坐标使用传入的 y。
    lv_obj_set_size(card, 216, 96);  // 将卡片尺寸设为宽 216 像素、高 96 像素。
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);  // 禁止卡片滚动，使其作为固定位置的内容容器。
    lv_obj_set_style_bg_color(card, lv_color_hex(0x172338), 0);  // 将卡片默认状态的背景设为深蓝色。
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);  // 将卡片背景设为完全不透明。
    lv_obj_set_style_radius(card, 12, 0);  // 将卡片圆角半径设为 12 像素。
    lv_obj_set_style_border_color(card, lv_color_hex(0x293B53), 0);  // 设置卡片边框颜色。
    lv_obj_set_style_border_width(card, 1, 0);  // 将卡片边框宽度设为 1 像素。
    return card;  // 返回卡片容器指针，供调用者在内部创建标签和进度条。
}  // 结束当前函数或控制流程的代码块。

static void demo_update(lv_timer_t *timer)  // 定义动态数据更新回调，由 LVGL 定时器周期调用。
{  // 开始当前函数的代码块。
    (void)timer;  // 当前不使用定时器参数，显式忽略它以避免未使用参数警告。
    uint32_t elapsed = lv_tick_elaps(demo_start_tick);  // 计算从示例开始到现在经过的毫秒数，LVGL 会处理计数回绕。
    uint32_t seconds = elapsed / 1000U;  // 将经过的毫秒数转换为整数秒。
    int32_t progress = (elapsed / 100U) % 101U;  // 每经过 100 毫秒推进 1%，并让进度在 0～100 之间循环。

    if (progress != last_progress) {  // 只有百分比发生变化时才更新控件，避免无意义的重复刷新。
        last_progress = progress;  // 保存这次进度值，供下一次回调比较。
        lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);  // 直接设置进度条数值，不使用补间动画。
        lv_label_set_text_fmt(progress_label, "%ld%%", (long)progress);  // 格式化百分比文本；%ld 输出整数，%% 输出一个百分号。
    }  // 结束当前函数或控制流程的代码块。

    if (seconds != last_seconds) {  // 只有整数秒发生变化时才更新计时文本和虚拟状态灯。
        last_seconds = seconds;  // 记录已经显示的秒数，避免同一秒内重复更新。
        lv_label_set_text_fmt(uptime_label, "%02lu:%02lu:%02lu",  // 按 时:分:秒 格式更新标签，各项至少显示两位，不足时补零。
                             (unsigned long)(seconds / 3600U),  // 将总秒数除以 3600 得到小时，并转换为格式字符串要求的类型。
                             (unsigned long)((seconds / 60U) % 60U),  // 先换算为分钟，再对 60 取余，得到当前分钟数。
                             (unsigned long)(seconds % 60U));  // 对 60 取余得到当前秒数，并结束这次格式化调用。
        if (seconds & 1U) lv_led_off(status_led);  // 当前秒数为奇数时，让界面上的虚拟状态灯变暗。
        else lv_led_on(status_led);  // 当前秒数为偶数时，让虚拟状态灯点亮，实现每秒切换明暗。
    }  // 结束当前函数或控制流程的代码块。
}  // 结束当前函数或控制流程的代码块。

static void demo_create(void)  // 定义示例界面创建函数，在 LVGL 和显示接口初始化后调用。
{  // 开始当前函数的代码块。
    lv_obj_t *screen = lv_scr_act();  // 获取默认显示器当前活动的屏幕对象，作为顶层父容器。
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);  // 禁止整个屏幕滚动，保持所有控件的位置固定。
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0B1220), 0);  // 将屏幕背景设置为深色。
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);  // 将屏幕背景设为完全不透明。

    demo_label(screen, "LVGL + ST7789", 16, 18, 0xF1F5F9, &lv_font_montserrat_20);  // 在屏幕坐标 (16,18) 创建浅色标题，使用 20 号字体。
    demo_label(screen, "STM32F407 | 240 x 320", 16, 48, 0x94A3B8, LV_FONT_DEFAULT);  // 在标题下方显示单片机型号和屏幕分辨率，使用默认字体。

    status_led = lv_led_create(screen);  // 创建界面虚拟状态灯，并保存指针以便定时切换明暗。
    lv_obj_set_size(status_led, 10, 10);  // 将虚拟状态灯设为 10×10 像素。
    lv_obj_set_pos(status_led, 215, 24);  // 把虚拟状态灯放到屏幕右上方的 (215,24) 位置。
    lv_led_set_color(status_led, lv_color_hex(0x22C55E));  // 将虚拟状态灯的发光颜色设置为绿色。

    lv_obj_t *uptime_card = demo_card(screen, 76);  // 在纵坐标 76 处创建运行时间卡片。
    demo_label(uptime_card, "UPTIME", 12, 10, 0x94A3B8, LV_FONT_DEFAULT);  // 在运行时间卡片内创建 UPTIME 标题，即运行时长。
    uptime_label = demo_label(uptime_card, "00:00:00", 12, 34, 0xF1F5F9,  // 创建运行时间数值标签，初始文本为 00:00:00，并保存指针。
                              &lv_font_montserrat_28);  // 继续指定运行时间数值使用 28 号字体，结束标签创建调用。
    demo_label(uptime_card, "HAL tick / LVGL timer", 12, 74, 0x94A3B8, LV_FONT_DEFAULT);  // 在卡片下方说明计时来源：HAL 毫秒时钟和 LVGL 定时器。

    lv_obj_t *progress_card = demo_card(screen, 184);  // 在纵坐标 184 处创建进度展示卡片。
    demo_label(progress_card, "PROGRESS", 12, 12, 0x94A3B8, LV_FONT_DEFAULT);  // 在进度卡片内创建 PROGRESS 标题，即进度。
    progress_label = demo_label(progress_card, "0%", 160, 12, 0x38BDF8, LV_FONT_DEFAULT);  // 在卡片右上方创建蓝色百分比标签，并保存其指针。
    lv_obj_set_width(progress_label, 42);  // 固定百分比标签宽度为 42 像素，使不同位数的数值可以统一对齐。
    lv_obj_set_style_text_align(progress_label, LV_TEXT_ALIGN_RIGHT, 0);  // 让百分比文字在标签内部靠右对齐。

    progress_bar = lv_bar_create(progress_card);  // 在进度卡片中创建进度条，并保存控件指针。
    lv_obj_set_pos(progress_bar, 12, 46);  // 将进度条放在卡片内容区域的 (12,46) 位置。
    lv_obj_set_size(progress_bar, 190, 14);  // 设置进度条宽 190 像素、高 14 像素。
    lv_bar_set_range(progress_bar, 0, 100);  // 设置进度条允许的数值范围为 0～100。
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x293B53), LV_PART_MAIN);  // 设置进度条主体轨道的背景颜色。
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);  // 设置进度条已完成部分的填充颜色为亮蓝色。
    demo_label(progress_card, "Automatic 0 - 100% loop", 12, 72, 0x94A3B8, LV_FONT_DEFAULT);  // 在进度条下方显示说明：自动在 0～100% 之间循环。

    lv_obj_t *footer = demo_label(screen, "Auto demo | no touch input", 0, 296,  // 在屏幕底部创建说明标签：自动演示，无需触摸输入。
                                  0x94A3B8, LV_FONT_DEFAULT);  // 继续指定底部说明文字的颜色和默认字体，结束标签创建调用。
    lv_obj_set_width(footer, ST7789_WIDTH);  // 让底部说明标签与屏幕等宽，便于将文字整体居中。
    lv_obj_set_style_text_align(footer, LV_TEXT_ALIGN_CENTER, 0);  // 让底部说明文字在标签内部水平居中。

    demo_start_tick = lv_tick_get();  // 保存当前毫秒计数，作为界面运行时间的起点。
    demo_update(NULL);  // 立即更新一次初始数值和状态灯，避免等待第一次定时回调。
    lv_timer_create(demo_update, 100, NULL);  // 创建每 100 毫秒执行一次的数据更新定时器，不传入用户数据。
}  // 结束当前函数或控制流程的代码块。

void user_main(void)  // 定义应用入口，由 main.c 完成时钟和外设初始化后调用。
{  // 开始当前函数的代码块。
    lv_init();  // 初始化 LVGL 核心、内存管理和内部定时器等模块。
    lv_port_disp_init();  // 初始化 ST7789，并向 LVGL 注册显示器、绘图缓冲区和刷新回调。
    demo_create();  // 创建示例界面中的标签、卡片、进度条和状态灯。

    while (1) {  // 进入应用主循环，持续处理 LVGL 任务。
        lv_timer_handler();  // 处理到期的 LVGL 定时器、动画和待刷新的显示区域。
        HAL_Delay(50);  // 按当前设置至少等待约 50 毫秒，再进行下一轮 LVGL 任务处理。
    }  // 结束当前函数或控制流程的代码块。
}  // 结束当前函数或控制流程的代码块。
