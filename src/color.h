#ifndef OMABAR_COLOR_H
#define OMABAR_COLOR_H

#include <stdbool.h>
#include <stdint.h>

struct color {
    int is_valid;
    float r;
    float g;
    float b;
    float a;
};

static inline bool color_equal(struct color c1, struct color c2) {
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a;
}

#define color_make_uint32(c) \
    (((uint32_t)((c).a * 255.0f)) << 24) | \
    (((uint32_t)((c).r * 255.0f)) << 16) | \
    (((uint32_t)((c).g * 255.0f)) << 8)  | \
    (((uint32_t)((c).b * 255.0f)) << 0)

#define color_is_valid(c) ((c).is_valid != 0)
#define color_is_clear(c) ((c).a <= 0.0f)

static inline struct color color_calloc(void) {
    struct color c = { .r = 0, .g = 0, .b = 0, .a = 0 };
    return c;
}

static inline float color_clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

struct color color_from_hex(uint32_t hex);
struct color color_from_hex_string(const char *hex);
struct color color_from_rgba(float r, float g, float b, float a);
struct color color_with_alpha(struct color color, float alpha);
struct color color_blend(struct color a, struct color b, float progress);

#endif