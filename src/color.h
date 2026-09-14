#ifndef OMABAR_COLOR_H
#define OMABAR_COLOR_H

#include <stdint.h>

struct color {
    int is_valid;
    float r;
    float g;
    float b;
    float a;
};

struct color color_from_hex(uint32_t hex);
struct color color_from_hex_string(const char *hex);
struct color color_from_rgba(float r, float g, float b, float a);

#endif