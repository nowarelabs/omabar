#include "shadow.h"
#include "misc/helpers.h"

struct shadow shadow_create(void) {
    struct shadow s;
    s.color = color_from_hex(0x00000000);
    s.angle = 90.0;
    s.distance = 0.0;
    return s;
}