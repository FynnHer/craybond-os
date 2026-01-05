/*
kernel/graph/drivers/ramfb_driver/ramfb_driver.c
This file implements a simple RAM framebuffer driver. It provides functions to initialize
the framebuffer, clear the screen, draw pixels, rectangles, lines, and characters.
It uses a basic 32-bit XRGB8888 pixel format.
Ramfb stands for RAM Framebuffer.
*/
#include "console/kio.h"
#include "ram_e.h"
#include "string.h"
#include "fw/fw_cfg.h"
#include "graph/font8x8_basic.h"

#define RGB_FORMAT_XRGB8888 ((uint32_t)('X') | ((uint32_t)('R') << 8) | ((uint32_t)('2') << 16) | ((uint32_t)('4') << 24))

typedef struct {
    /*
    This structure represents the framebuffer configuration for the RAM framebuffer.
    It includes fields for the framebuffer address, pixel format (fourcc), flags,
    width, height, and stride (number of bytes per row).
    */
    uint64_t addr;
    uint32_t fourcc;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
}__attribute__((packed)) fb_structure;

uint64_t fb_ptr;

uint32_t width;
uint32_t height;
uint32_t bpp;
uint32_t stride;

uint32_t fix_rgb(uint32_t color) {
    /*
    This function converts a color from RGB format to XRGB format.
    It rearranges the color components to match the XRGB8888 pixel format.
    Example usage: uint32_t xrgb_color = fix_rgb(0xFF0000); // Converts red color to XRGB format.
    */
    return (color & 0xFF0000) >> 16 
         | (color & 0x00FF00)       
         | (color & 0x0000FF) << 16;
}

void rfb_clear(uint32_t color){
    /*
    This function clears the entire framebuffer by filling it with a specified color.
    It calculates the total number of pixels and sets each pixel to the provided color.
    Example usage: rfb_clear(0x00000000); would clear the screen to black.
    */
    volatile uint32_t* fb = (volatile uint32_t*)fb_ptr;
    uint32_t pixels = width * height;
    for (uint32_t i = 0; i < pixels; i++) {
        fb[i] = fix_rgb(color);
    }
}

void rfb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    /*
    This function draws a single pixel at the specified (x, y) coordinates with the given color.
    It first checks if the coordinates are within the framebuffer bounds before setting the pixel value.
    Example usage: rfb_draw_pixel(10, 20, 0xFF0000); would draw a red pixel at coordinates (10, 20).
    */
    if (x >= width || y >= height) return;
    volatile uint32_t* fb = (volatile uint32_t*)fb_ptr;
    fb[y * (stride / 4) + x] = color;
}

void rfb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    /*
    This function fills a rectangle defined by the top-left corner (x, y),
    width (w), and height (h) with the specified color.
    It iterates over the rectangle area and sets each pixel to the provided color.
    Example usage: rfb_fill_rect(10, 20, 100, 50, 0x00FF00); would fill a green rectangle at (10,20) with width 100 and height 50.
    */
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            rfb_draw_pixel(x + dx, y + dy, color);
        }
    }
}

void rfb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    /*
    This function draws a line from point (x0, y0) to point (x1, y1) using Bresenham's line algorithm.
    It calculates the necessary steps to interpolate between the two points and sets the pixels along the line to the specified color.
    Example usage: rfb_draw_line(10, 20, 100, 200, 0x0000FF); would draw a blue line from (10,20) to (100,200).
    */
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    for (;;) {
        rfb_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void rfb_draw_char(uint32_t x, uint32_t y, char c, uint32_t scale, uint32_t color) {
    /*
    This function draws a character at the specified (x, y) coordinates using an 8x8 pixel font.
    It retrieves the glyph for the character from a predefined font array and sets the corresponding pixels to the specified color.
    Example usage: rfb_draw_char(10, 20, 'A', 0xFFFFFF); would draw the character 'A' in white at coordinates (10, 20).
    */
    const uint8_t* glyph = font8x8_basic[(uint8_t)c];
    for (uint32_t row = 0; row < (8 * scale); row++) {
        uint8_t bits = glyph[row / scale];
        for (uint32_t col = 0; col < (8 * scale); col++) {
            if (bits & (1 << (7 - col))) {
                rfb_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

bool rfb_init(uint32_t w, uint32_t h) {
    /*
    This function initializes the RAM framebuffer with the specified width (w) and height (h).
    It allocates memory for the framebuffer, configures the framebuffer settings,
    and writes the configuration to the firmware using fw_cfg.
    If successful, it returns true; otherwise, it returns false.
    Example usage: bool success = rfb_init(800, 600); would initialize the framebuffer for an 800x600 display.
    */

    width = w;
    height = h;

    bpp = 4;
    stride = bpp * width;
    
    struct fw_cfg_file file;
    fw_find_file(string_l("etc/ramfb"), &file);
    
    if (file.selector == 0x0){
        kprintf("Ramfb not found");
        return false;
    }

    fb_ptr = palloc(width * height * bpp);

    fb_structure fb = {
        .addr = __builtin_bswap64(fb_ptr),
        .width = __builtin_bswap32(width),
        .height = __builtin_bswap32(height),
        .fourcc = __builtin_bswap32(RGB_FORMAT_XRGB8888),
        .flags = __builtin_bswap32(0),
        .stride = __builtin_bswap32(stride),
    };

    fw_cfg_dma_write(&fb, sizeof(fb), file.selector);
    
    kprintf("ramfb configured");

    return true;
}

void rfb_flush(){
    
}