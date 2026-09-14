#include "color.h"
#include "misc/helpers.h"

struct color color_from_hex(uint32_t hex) {
    struct color c = { .is_valid = 1 };
    c.a = (float)((hex >> 24) & 0xff) / 255.0f;
    c.r = (float)((hex >> 16) & 0xff) / 255.0f;
    c.g = (float)((hex >> 8) & 0xff) / 255.0f;
    c.b = (float)(hex & 0xff) / 255.0f;
    return c;
}

struct color color_from_rgba(float r, float g, float b, float a) {
    struct color c = { .is_valid = 1, .r = r, .g = g, .b = b, .a = a };
    return c;
}

struct color color_from_hex_string(const char *hex) {
    if (!hex) return (struct color){0};
    uint32_t value = (uint32_t)strtoul(hex, NULL, 16);
    return color_from_hex(value);
}