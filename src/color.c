#include "color.h"
#include <stdlib.h>
#include <string.h>

struct color color_from_hex(uint32_t hex) {
    struct color c = { .is_valid = 1 };
    c.a = (float)((hex >> 24) & 0xff) / 255.0f;
    c.r = (float)((hex >> 16) & 0xff) / 255.0f;
    c.g = (float)((hex >> 8) & 0xff) / 255.0f;
    c.b = (float)(hex & 0xff) / 255.0f;
    return c;
}

struct color color_from_rgba(float r, float g, float b, float a) {
    struct color c = { .is_valid = 1,
                       .r = color_clamp(r, 0.0f, 1.0f),
                       .g = color_clamp(g, 0.0f, 1.0f),
                       .b = color_clamp(b, 0.0f, 1.0f),
                       .a = color_clamp(a, 0.0f, 1.0f) };
    return c;
}

struct color color_with_alpha(struct color color, float alpha) {
    color.a = color_clamp(alpha, 0.0f, 1.0f);
    return color;
}

struct color color_blend(struct color a, struct color b, float progress) {
    progress = color_clamp(progress, 0.0f, 1.0f);
    struct color c = {
        .is_valid = 1,
        .r = a.r + (b.r - a.r) * progress,
        .g = a.g + (b.g - a.g) * progress,
        .b = a.b + (b.b - a.b) * progress,
        .a = a.a + (b.a - a.a) * progress,
    };
    return c;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static struct color color_from_web_hex(const char *hex, size_t len) {
    unsigned char comps[4] = { 0, 0, 0, 255 };
    int n = (int)len;
    size_t count = n / 2;
    if (count < 3 || count > 4) return (struct color){0};

    for (int i = 0; i < n; i++) {
        int d = hex_digit(hex[i]);
        if (d < 0) return (struct color){0};
        comps[i / 2] = (unsigned char)((i % 2 == 0) ? (d << 4) : (comps[i / 2] | d));
    }

    return color_from_rgba(comps[0] / 255.0f,
                           comps[1] / 255.0f,
                           comps[2] / 255.0f,
                           count == 4 ? comps[3] / 255.0f : 1.0f);
}

struct color color_from_hex_string(const char *hex) {
    if (!hex || !*hex) return (struct color){0};
    size_t len = strlen(hex);

    if (hex[0] == '#') {
        const char *digits = hex + 1;
        size_t dlen = len - 1;
        if (dlen == 3 || dlen == 4) {
            char expanded[9];
            int n = 0;
            for (size_t i = 0; i < dlen; i++) {
                expanded[n++] = digits[i];
                expanded[n++] = digits[i];
            }
            expanded[n] = '\0';
            return color_from_web_hex(expanded, n);
        }
        return color_from_web_hex(digits, dlen);
    }

    if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        return color_from_hex((uint32_t)strtoul(hex + 2, NULL, 16));
    }

    size_t plain = len;
    if (plain == 6)
        return color_from_web_hex(hex, plain);
    if (plain == 8)
        return color_from_hex((uint32_t)strtoul(hex, NULL, 16));
    return (struct color){0};
}