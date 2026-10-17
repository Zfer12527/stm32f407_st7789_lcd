#include "lv_port_disp.h"
#include "LCD.h"
#include <stdbool.h>

#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 1
#error "ST7789 display port requires RGB565 with LV_COLOR_16_SWAP=1"
#endif

// One partial buffer: 240 * 20 * 2 = 9600 bytes in normal SRAM.
#define DISP_BUFFER_LINES 20

static lv_color_t draw_pixels[ST7789_WIDTH * DISP_BUFFER_LINES];
static lv_disp_draw_buf_t draw_buffer;
static lv_disp_drv_t display_driver;
static bool updates_enabled = true;

static void disp_flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    if (updates_enabled) {
        // LVGL provides a tightly packed, screen-clipped rectangle.
        // LV_COLOR_16_SWAP already puts its bytes in the ST7789 wire order.
        ST7789_Blit(area->x1, area->y1, area->x2, area->y2, (const uint8_t *)pixels);
    }

    // Blocking SPI has finished using the buffer; LVGL may reuse it now.
    lv_disp_flush_ready(driver);
}

void lv_port_disp_init(void)
{
    ST7789_Init();
    lv_disp_draw_buf_init(&draw_buffer, draw_pixels, NULL,
                         ST7789_WIDTH * DISP_BUFFER_LINES);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = ST7789_WIDTH;
    display_driver.ver_res = ST7789_HEIGHT;
    display_driver.draw_buf = &draw_buffer;
    display_driver.flush_cb = disp_flush;
    // full_refresh remains disabled: LVGL redraws only invalidated areas.
    lv_disp_drv_register(&display_driver);
}

void disp_enable_update(void)
{
    updates_enabled = true;
    // Request a redraw as buffers may have been discarded while updates paused.
    lv_obj_invalidate(lv_scr_act());
}

void disp_disable_update(void)
{
    updates_enabled = false;
}
