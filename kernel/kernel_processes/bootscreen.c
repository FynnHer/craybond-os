/*
kernel/kernel_processes/bootscreen.c
This file implements a simple bootscreen process that displays graphics on the screen.
It initializes a bootscreen process that runs in the kernel, drawing graphics
to the framebuffer in a loop with a delay.
*/
#include "bootscreen.h"
#include "process/kprocess_loader.h"
#include "console/kio.h"
#include "graph/graphics.h"

int abs(int n) {
    return n < 0 ? -n : n;
}

int lerp(int step, int a, int b) {
    return (a + step * (a < b ? 1 : -1));
}

__attribute__((section(".text.kbootscreen"))) // Code section for bootscreen process
void bootscreen() {
    /*
    This function implements the bootscreen process that runs in the kernel.
    It clears the screen and draws a series of lines radiating from the center
    of the screen to create a simple graphical effect. It runs in an infinite loop.
    Example usage: This function is invoked as a kernel process to display the bootscreen.
    */
    disable_visual();
    while (1) {
        gpu_clear(0);
        size screen_size = gpu_get_screen_size();
        point screen_middle = {screen_size.width / 2, screen_size.height / 2};
        int sizes[4] = {30, screen_size.width/5, screen_size.height/3, 40};
        int padding = 10;
        int yoffset = 50;
        point current_point = {screen_middle.x-padding-sizes[1], screen_middle.y-padding-yoffset-sizes[0]};
        for (int i = 0; i < 12; i++) {
            int ys = i > 5 ? -1 : 1; // y sign -> if i > 5, go up, else go down
            bool ui = (i % 6) != 0 && (i % 6) != 5; // is u or i -> if i is not 0 or 5 mod 6, it's u or i
            bool ul = (i/2) % 2 == 0; // is u or l -> if i/2 is even, it's u or l
            bool xn = (i/3) % 3 == 0; // x negative -> if i/3 is 0 mod 3, x is negative
            if (i >= 6) {
                ul = !ul;
            }
            int xs = xn ? -1 : 1; // x sign
            int xloc = padding + (ui ? sizes[3] : sizes[1]);
            int yloc = padding + (ul ? sizes[0] : sizes[2]);
            point next_point = {screen_middle.x + (xs * xloc), screen_middle.y + (ys * yloc) - (ul ? yoffset : 0)};
            int xlength = abs(current_point.x - next_point.x);
            int ylength = abs(current_point.y - next_point.y);
            for (int x = 0; x <= xlength; x++) {
                for (int y = 0; y <= ylength; y++) {
            //          Draw the line interpolating
                    point interpolated = {lerp(x, current_point.x, next_point.x),
                                          lerp(y, current_point.y, next_point.y)};
                    gpu_draw_pixel(interpolated, 0xFFFFFF);
                    for (int k = 0; k < 1000000; k++) {} // Simple delay
                }
            }
            current_point = next_point;
        }
    }
        while (1) {}
}

void start_bootscreen() {
    /*
    This function starts the bootscreen process by creating a kernel process
    that runs the bootscreen function. It uses the kernel process loader to
    create the process with a specified stack size.
    Example usage: start_bootscreen(); would initialize and run the bootscreen process.
    */

    extern uint8_t kbootscreen_start;
    extern uint8_t kbootscreen_end;
    // extern uint8_t kbootscreen_rodata_start;
    // extern uint8_t kbootscreen_rodata_end;
    create_kernel_process(bootscreen,
        (uint64_t)&kbootscreen_end - (uint64_t)&kbootscreen_start/*,
        (uint64_t)&kbootscreen_start, (void*)0x0, 0*/);
}