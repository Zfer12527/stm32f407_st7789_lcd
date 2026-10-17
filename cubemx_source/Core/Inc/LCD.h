#ifndef __LCD_H
#define __LCD_H
#include "main.h"

// Confirmed module: 2.0-inch ST7789, 240x320, four-wire SPI.
#define ST7789_WIDTH        240
#define ST7789_HEIGHT       320
#define ST7789_X_OFFSET     0
#define ST7789_Y_OFFSET     0
#define ST7789_MADCTL       0x00 // Portrait, RGB; set bit 3 for BGR panels.
#define ST7789_INVERSION_ON 1    // Common IPS setting; change to 0 if needed.
#define ST7789_BACKLIGHT_PERCENT 50U // PA1/TIM2_CH2: default 50% duty at 5 kHz.

#define ST7789_COLOR_BLACK  0x0000
#define ST7789_COLOR_RED    0xF800
#define ST7789_COLOR_GREEN  0x07E0
#define ST7789_COLOR_BLUE   0x001F

#if (ST7789_WIDTH < 1) || (ST7789_HEIGHT < 1) || \
    (ST7789_X_OFFSET + ST7789_WIDTH > 240) || \
    (ST7789_Y_OFFSET + ST7789_HEIGHT > 320)
#error "ST7789 portrait dimensions and offsets exceed controller RAM"
#endif

// SPI1: PA5=SCL/SCK, PB5=SDA/MOSI; PA1=BL (active high).
#define ST7789_CS_PIN    LCD_CS_Pin
#define ST7789_CS_PORT   LCD_CS_GPIO_Port
#define ST7789_DC_PIN    LCD_DC_Pin
#define ST7789_DC_PORT   LCD_DC_GPIO_Port
#define ST7789_RST_PIN   LCD_RST_Pin
#define ST7789_RST_PORT  LCD_RST_GPIO_Port
#define ST7789_BL_PIN    LCD_BL_Pin
#define ST7789_BL_PORT   LCD_BL_GPIO_Port

#define ST7789_CS_L()    HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
#define ST7789_CS_H()    HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)
#define ST7789_DC_L()    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define ST7789_DC_H()    HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)
#define ST7789_RST_L()   HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_H()   HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)
#define ST7789_BL_ON()   LCD_Backlight_SetDuty(ST7789_BACKLIGHT_PERCENT)
#define ST7789_BL_OFF()  LCD_Backlight_SetDuty(0U)

void ST7789_Init(void);
void ST7789_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ST7789_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void ST7789_FillScreen(uint16_t color);
// Packed RGB565 bytes, high byte first; the rectangle endpoints are inclusive.
void ST7789_Blit(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                 const uint8_t *pixels);
extern volatile HAL_StatusTypeDef ST7789_LastSpiStatus;

#endif
