#ifndef OMABAR_FONT_H
#define OMABAR_FONT_H

#include <CoreText/CoreText.h>
#include <stdbool.h>

struct font {
    CTFontRef ct_font;
    bool font_changed;
    double size;
    char *family;
    char *style;
    char *features;
};

struct font *font_create(const char *family, const char *style, double size);
struct font *font_create_descriptor(CTFontDescriptorRef descriptor);
void font_init(struct font *font);
void font_destroy(struct font *font);
bool font_equal(struct font *a, struct font *b);
bool font_set(struct font *font, char *font_string, bool forced);
bool font_set_family(struct font *font, char *family, bool forced);
bool font_set_style(struct font *font, char *style, bool forced);
bool font_set_size(struct font *font, float size);
bool font_set_features(struct font *font, char *features);
void font_create_ctfont(struct font *font);
void font_register(char *font_path);
void font_clear_pointers(struct font *font);

#endif