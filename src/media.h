#ifndef OMABAR_MEDIA_H
#define OMABAR_MEDIA_H

#include <stdbool.h>

struct media_info {
    char *app;
    char *title;
    char *artist;
    char *album;
    bool playing;
};

void forced_media_event(void);
void media_begin(void);
void media_end(void);
struct media_info *media_get_info(void);
void media_free_info(struct media_info *info);

#endif