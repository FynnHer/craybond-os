/*
kernel/graph/graphic_types.h
This file defines basic graphic types such as color, point, size, and rectangle.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t color;

typedef struct {
    uint32_t x;
    uint32_t y;
}__attribute__((packed)) point;

typedef struct {
    uint32_t width;
    uint32_t height;
}__attribute__((packed)) size;

typedef struct {
    point point;
    size size;
}__attribute__((packed)) rect;

#ifdef __cplusplus
}
#endif