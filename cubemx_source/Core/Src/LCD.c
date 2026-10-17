#include "LCD.h"
#include "stm32f4xx_hal_spi.h"

volatile HAL_StatusTypeDef ST7789_LastSpiStatus = HAL_OK;

static void ST7789_Transmit(const uint8_t *data, uint16_t size)
{
    // HAL keeps a mutable TX pointer internally but only reads these bytes.
    ST7789_LastSpiStatus = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, 1000);
    if (ST7789_LastSpiStatus != HAL_OK) {
        ST7789_CS_H();
        // A local SPI failure is indicated by repeated groups of three flashes.
        while (1) {
            for (uint8_t i = 0; i < 3; ++i) {
                ST7789_BL_ON();
                HAL_Delay(100);
                ST7789_BL_OFF();
                HAL_Delay(100);
            }
            HAL_Delay(700);
        }
    }
}

static void ST7789_WriteRegister(uint8_t command, const uint8_t *data, uint16_t size)
{
    ST7789_DC_L();
    ST7789_CS_L();
    ST7789_Transmit(&command, 1);
    if (size > 0) {
        ST7789_DC_H();
        ST7789_Transmit(data, size);
    }
    ST7789_CS_H();
}

// 设置窗口：x1,y1起点；x2,y2终点
void ST7789_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    if (x1 > x2 || y1 > y2 || x2 >= ST7789_WIDTH || y2 >= ST7789_HEIGHT) return;
    x1 += ST7789_X_OFFSET;
    x2 += ST7789_X_OFFSET;
    y1 += ST7789_Y_OFFSET;
    y2 += ST7789_Y_OFFSET;

    uint8_t columns[4] = {x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF};
    uint8_t rows[4] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    ST7789_WriteRegister(0x2A, columns, sizeof(columns)); // CASET
    ST7789_WriteRegister(0x2B, rows, sizeof(rows));       // RASET
    ST7789_WriteRegister(0x2C, NULL, 0);                 // RAMWR
}

// 核心：画点函数
void ST7789_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    if(x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return; // 越界保护
    ST7789_SetWindow(x,y,x,y); // 窗口仅单个像素
    ST7789_CS_L();
    ST7789_DC_H();
    // RGB565：高字节先发
    uint8_t buf[2] = {color >> 8, color & 0xFF};
    ST7789_Transmit(buf, 2);
    ST7789_CS_H();
}

void ST7789_Blit(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                 const uint8_t *pixels)
{
    if (pixels == NULL || x1 > x2 || y1 > y2 ||
        x2 >= ST7789_WIDTH || y2 >= ST7789_HEIGHT) return;

    uint32_t bytes = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1) * 2U;
    ST7789_SetWindow(x1, y1, x2, y2);
    ST7789_DC_H();
    ST7789_CS_L();
    while (bytes > 0) {
        // HAL's transfer length is uint16_t. Keep every chunk pixel-aligned.
        uint16_t chunk = bytes > 65534U ? 65534U : (uint16_t)bytes;
        ST7789_Transmit(pixels, chunk);
        pixels += chunk;
        bytes -= chunk;
    }
    ST7789_CS_H();
}

void ST7789_FillScreen(uint16_t color)
{
    // One row uses 480 bytes; do not allocate a full framebuffer on the stack.
    uint8_t row[ST7789_WIDTH * 2];
    for (uint16_t x = 0; x < ST7789_WIDTH; ++x) {
        row[x * 2] = color >> 8;
        row[x * 2 + 1] = color & 0xFF;
    }

    ST7789_SetWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    ST7789_CS_L();
    ST7789_DC_H();
    for (uint16_t y = 0; y < ST7789_HEIGHT; ++y) {
        ST7789_Transmit(row, sizeof(row));
    }
    ST7789_CS_H();
}

// ST7789初始化序列
void ST7789_Init(void)
{
    // Generic ST7789 SPI initialization. Keep factory panel voltage/gamma values.
    uint8_t pixel_format = 0x55; // RGB565, 16 bits per pixel.
    uint8_t memory_access = ST7789_MADCTL;
    ST7789_LastSpiStatus = HAL_OK;
    ST7789_BL_OFF();
    ST7789_CS_H();
    ST7789_DC_H();
    ST7789_RST_H();
    HAL_Delay(10);
    ST7789_RST_L();
    HAL_Delay(20);
    ST7789_RST_H();
    HAL_Delay(120);

    ST7789_WriteRegister(0x01, NULL, 0); // SWRESET
    HAL_Delay(150);
    ST7789_WriteRegister(0x11, NULL, 0); // SLPOUT
    HAL_Delay(120);

    ST7789_WriteRegister(0x3A, &pixel_format, 1); // COLMOD
    HAL_Delay(10);
    ST7789_WriteRegister(0x36, &memory_access, 1); // MADCTL
#if ST7789_INVERSION_ON
    ST7789_WriteRegister(0x21, NULL, 0); // INVON, common IPS panel setting.
#else
    ST7789_WriteRegister(0x20, NULL, 0); // INVOFF
#endif
    HAL_Delay(10);
    ST7789_WriteRegister(0x13, NULL, 0); // NORON
    HAL_Delay(10);

    ST7789_FillScreen(ST7789_COLOR_BLACK);
    ST7789_WriteRegister(0x29, NULL, 0); // DISPON
    HAL_Delay(20);
    ST7789_BL_ON();
}
