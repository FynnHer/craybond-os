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
#include "kstring.h"
#include "ram_e.h"
#include "exception_handler.h"

int abs(int n) {
    return n < 0 ? -n : n;
}

int lerp(int step, int a, int b) {
    return (a + step * (a < b ? 1 : -1));
}

static uint64_t randomNumber = 0;

__attribute__((section(".text.kbootscreen")))
void boot_draw_name(point screen_middle, int xoffset, int yoffset) {
    const char* name = "Craybond OS - Crayons are for losers - %i%";
    uint64_t *i = &randomNumber;
    kstring s = string_format_args(name, i, 1);
    int scale = 2;
    uint32_t char_size = gpu_get_char_size(scale);
    int mid_offset = (s.length/2) * char_size;
    int xo = screen_middle.x - mid_offset + xoffset;
    int yo = screen_middle.y + yoffset;
    gpu_fill_rect((rect){xo,yo, char_size * s.length, char_size}, 0x0);
    gpu_draw_string(s, (point){xo, yo}, scale, 0xFFFFFF);
    temp_free(s.data, 256);
}

__attribute__((section(".text.kbootscreen"))) // Code section for bootscreen process
void bootscreen() {
    /*
    This function implements the bootscreen process that runs in the kernel.
    It clears the screen and draws a series of lines radiating from the center
    of the screen to create a simple graphical effect. It runs in an infinite loop.
    Example usage: This function is invoked as a kernel process to display the bootscreen.
    */
   /*
    disable_visual();
    while (1) {
        gpu_clear(0);
        size screen_size = gpu_get_screen_size();
        point screen_middle = {screen_size.width / 2, screen_size.height / 2};
        int sizes[4] = {30, screen_size.width/5, screen_size.height/3, 40};
        int padding = 10;
        int yoffset = 50;
        boot_draw_name(screen_middle, 0, padding + sizes[2] + 10);
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
    */
   disable_visual();
   while (1) {
        gpu_clear(0);
        size screen_size = gpu_get_screen_size();
        point screen_middle = {screen_size.width / 2, screen_size.height / 2};
        
        // preserve original name drawing position
        int sizes_height = screen_size.height / 3;
        int padding = 10;

        // --- Crayon C Drawing Logic ---

        int radius = screen_size.height / 4;
        // Start angle: -45 degrees (Top right)
        // using fixed point math (scale 1024) to avoid flaoting point math in kernel
        // start pos realtive to center: x = r*cos(-45), y = r*sin(-45)
        // cose(-45) ~= 0.707, sin(-45) ~= -0.707
        // 0.707 * 1024 = 724
        int x = (radius * 724) / 1024;
        int y = (radius * -724) / 1024;

        // Rotation step per iteration: approx -1.5 degrees (clockwise movement)
        // cos(1.5) ~= 0.99966 -> 1023
        // sin(1.5) ~= 0.02618 -> 27
        int cos_step = 1023;
        int sin_step = 27;

        // draw from top right -> top -> left -> bottom -> bottom right
        // total sweep is ~270 degrees. 270 / 1.5 = 180 steps
        for (int i = 0; i < 180; i++) {
            point current_point = {screen_middle.x + x, screen_middle.y + y};

            // draw crayon tip
            int thickness = 5;
            for (int bx = -thickness; bx <= thickness; bx++) {
                for (int by = -thickness; by <= thickness; by++) {
                    // check if pixel is withing the brush radius
                    if ((bx * bx + by * by) <= (thickness * thickness)) {
                        point brush_pixel = {current_point.x + bx, current_point.y + by};
                        // draw using a crayon like color (orange-red: 0xFF4500)
                        gpu_draw_pixel(brush_pixel, 0xFF4500);
                    }
                    boot_draw_name(screen_middle,0,padding + (screen_size.height / 3) + 10);
                }

            }
            // Apply fixed pint rotation matrix for next point
            // rotate cw: x' = x*cos + y*sin, y' = -x*sin + y*cos
            int next_x = (x * cos_step + y * sin_step) / 1024;
            int next_y = (y * cos_step - x * sin_step) / 1024;
            x = next_x;
            y = next_y;

            randomNumber += 1;
            // randomNumber %= 100;
            if (randomNumber > 100) {
                panic_with_info("Failed to load", 0x12345);
            }

            // animation delay to show the "forming" of the crayon C
            for (int k = 0; k > 500000; k++) {
                // simple delay loop
            }
        }
        // pause with the finished drawing before restarting
        for (int k = 0; k < 100000000000; k++) {}
   }
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