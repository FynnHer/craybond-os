/*
kernel/graph/graphics.c
This file provides a unified graphics interface that abstracts different GPU drivers.
It allows initialization, drawing operations, and screen management regardless of the underlying GPU.
Current supported GPUs are VirtIO GPU PCI and RAM Framebuffer (ramfb).
Features are selected based on the available GPU during initialization.
*/
#include "graphics.h"
#include "console/kio.h"

#include "graph/drivers/virtio_gpu_pci/virtio_gpu_pci_driver.h"
#include "graph/drivers/ramfb_driver/ramfb_driver.h"

static size screen_size;
static bool _gpu_ready;

typedef enum {
    NONE,
    VIRTIO_GPU_PCI,
    RAMFB
} SupportedGPU;

SupportedGPU chosen_GPU;

void gpu_init(size preferred_screen_size){
    /*
    This function initializes the graphics subsystem by detecting and initializing
    the available GPU. It attempts to initialize the VirtIO GPU PCI driver first,
    and if that fails, it falls back to the RAM framebuffer driver.
    The preferred screen size is used during initialization.
    Once a GPU is successfully initialized, it sets the screen size and marks the GPU as ready.
    Example usage: gpu_init((size){1024, 768}) would initialize the GPU with a screen size of 1024x768.
    */
    if (vgp_init(preferred_screen_size.width,preferred_screen_size.height))
        chosen_GPU = VIRTIO_GPU_PCI;
    else if (rfb_init(preferred_screen_size.width,preferred_screen_size.height))
        chosen_GPU = RAMFB;
    screen_size = preferred_screen_size;
    _gpu_ready = true;
    kprintf("Selected and initialized GPU %i",chosen_GPU);
}

bool gpu_ready(){
    /*
    This function checks if the graphics subsystem is ready for use.
    It returns true if a supported GPU has been initialized and is ready, false otherwise.
    Example usage: if (gpu_ready()) { ... } would check if the GPU is ready before performing graphics operations.
    */
    return chosen_GPU != NONE && _gpu_ready;
}

void gpu_flush(){
    /*
    This function flushes the current graphics operations to the screen.
    It calls the appropriate flush function based on the initialized GPU driver.
    Example usage: gpu_flush() would update the display with any pending graphics changes.
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_flush();
            break;
        case RAMFB:
            rfb_flush();
            break;
        default:
            break;
    }
}
void gpu_clear(color color){
    /*
    This function clears the screen to a specified color.
    It calls the appropriate clear function based on the initialized GPU driver.
    Example usage: gpu_clear(0x000000) would clear the screen to black.
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_clear(color);
            break;
        case RAMFB:
            rfb_clear(color);
        default:
            break;
    }
}

void gpu_draw_pixel(point p, color color){
    /*
    This function draws a pixel at the specified point with the given color.
    It calls the appropriate draw_pixel function based on the initialized GPU driver.
    Example usage: gpu_draw_pixel((point){10, 20}, 0xFFFF00) would draw a green pixel at coordinates (10, 20).
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_draw_pixel(p.x,p.y,color);
        break;
        case RAMFB:
            rfb_draw_pixel(p.x,p.y,color);
        break;
        default:
        break;
    }
}

void gpu_fill_rect(rect r, color color){
    /*
    This function fills a rectangle defined by the given rect structure with the specified color.
    It calls the appropriate fill_rect function based on the initialized GPU driver.
    Example usage: gpu_fill_rect((rect){{10, 20}, {100, 50}}, 0xFF0000) would fill a rectangle at (10,20) with width 100 and height 50 in red color.
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_fill_rect(r.point.x,r.point.y,r.size.width,r.size.height,color);
        break;
        case RAMFB:
            rfb_fill_rect(r.point.x,r.point.y,r.size.width,r.size.height,color);
        break;
        default:
        break;
    }
}

void gpu_draw_line(point p0, point p1, uint32_t color){
    /*
    This function draws a line from point p0 to point p1 with the specified color.
    It calls the appropriate draw_line function based on the initialized GPU driver.
    Example usage: gpu_draw_line((point){10, 20}, (point){100, 200}, 0x0000FF) 
    would draw a blue line from (10,20) to (100,200).
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_draw_line(p0.x,p0.y,p1.x,p1.y,color);
        break;
        case RAMFB:
            rfb_draw_line(p0.x,p0.y,p1.x,p1.y,color);
        break;
        default:
        break;
    }
}
void gpu_draw_char(point p, char c, uint32_t scale, uint32_t color){
    /*
    This function draws a character at the specified point with the given color.
    It calls the appropriate draw_char function based on the initialized GPU driver.
    Example usage: gpu_draw_char((point){50, 50}, 'A', 0xFFFFFF) 
    would draw the character 'A' in white color at coordinates (50, 50).
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case VIRTIO_GPU_PCI:
            vgp_draw_char(p.x,p.y,c,color);
        break;
        case RAMFB:
            rfb_draw_char(p.x,p.y,c,scale,color);
        break;
        default:
        break;
    }
}

void gpu_draw_string(kstring s, point p, uint32_t scale, uint32_t color) {
    /*
    This function draws a string at the specified point with the given scale.
    It calls the appropriate draw_string function based on the initialized GPU driver.
    Example usage: gpu_draw_string(string_l("Hello"), (point){100, 100}, 1) 
    would draw the string "Hello" at coordinates (100, 100) with a scale of 1.
    */
    if (!gpu_ready())
        return;
    switch (chosen_GPU) {
        case RAMFB:
            rfb_draw_string(s, p.x, p.y, scale, color);
        break;
        default:
        break;
    }
}

uint32_t gpu_get_char_size(uint32_t scale) {
    if (!gpu_ready())
        return 0;
    switch (chosen_GPU) {
        case RAMFB:
            return rfb_get_char_size(scale);
        break;
        default:
        break;
    }
}

size gpu_get_screen_size(){
    /*
    This function retrieves the current screen size of the initialized GPU.
    It returns a size structure containing the width and height of the screen.
    Example usage: size s = gpu_get_screen_size(); would get the current screen dimensions.
    */
    if (!gpu_ready())
        return (size){0,0};
    return screen_size;
}