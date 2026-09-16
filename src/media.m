#include "media.h"
#include "event.h"
#include "misc/extern.h"
#include <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static id g_token_info = nil;
static id g_token_playing = nil;
static bool g_media_events = false;

static void post_media_changed(void) {
    @autoreleasepool {
        struct media_info *minfo = media_get_info();
        if (!minfo) {
            struct event event = { .type = EVENT_MEDIA_CHANGED };
            event_post(&event);
            return;
        }

        size_t len = strlen(minfo->app) + strlen(minfo->title)
                   + strlen(minfo->artist) + strlen(minfo->album) + 256;
        char *json = calloc(1, len);
        if (!json) { media_free_info(minfo); return; }

        snprintf(json, len,
                 "{\n"
                 "\t\"state\": \"%s\",\n"
                 "\t\"title\": \"%s\",\n"
                 "\t\"album\": \"%s\",\n"
                 "\t\"artist\": \"%s\",\n"
                 "\t\"app\": \"%s\"\n}",
                 minfo->playing ? "playing" : "paused",
                 minfo->title, minfo->album, minfo->artist, minfo->app);

        struct event event = { .type = EVENT_MEDIA_CHANGED, .data = json };
        event_post(&event);
        media_free_info(minfo);
    }
}

void forced_media_event(void) {
    post_media_changed();
}

void media_begin(void) {
    if (g_media_events) return;
    g_media_events = true;

    MRMediaRemoteRegisterForNowPlayingNotifications(NULL);

    g_token_info =
        [[NSNotificationCenter defaultCenter]
            addObserverForName:@"kMRMediaRemoteNowPlayingInfoDidChangeNotification"
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *note) {
                        (void)note;
                        post_media_changed();
                    }];

    g_token_playing =
        [[NSNotificationCenter defaultCenter]
            addObserverForName:@"kMRMediaRemoteNowPlayingApplicationIsPlayingDidChangeNotification"
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification *note) {
                        (void)note;
                        post_media_changed();
                    }];

    forced_media_event();
}

void media_end(void) {
    if (g_token_info) {
        [[NSNotificationCenter defaultCenter] removeObserver:g_token_info];
        g_token_info = nil;
    }
    if (g_token_playing) {
        [[NSNotificationCenter defaultCenter] removeObserver:g_token_playing];
        g_token_playing = nil;
    }
}

static char *copy_cf_string(CFStringRef str) {
    if (!str) return strdup("");
    CFIndex length = CFStringGetLength(str);
    CFIndex capacity = CFStringGetMaximumSizeForEncoding(
        length, kCFStringEncodingUTF8) + 1;
    char *buffer = calloc(1, (size_t)capacity);
    if (!buffer) return strdup("");
    if (!CFStringGetCString(str, buffer, capacity, kCFStringEncodingUTF8)) {
        free(buffer);
        return strdup("");
    }
    return buffer;
}

struct media_info *media_get_info(void) {
    struct media_info *info = calloc(1, sizeof(*info));
    if (!info) return NULL;

    info->app = strdup("unknown");
    info->title = strdup("");
    info->artist = strdup("");
    info->album = strdup("");
    info->playing = false;

    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    __block CFDictionaryRef meta = NULL;

    MRMediaRemoteGetNowPlayingInfo(dispatch_get_global_queue(
                                       DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                                   ^(NSDictionary *dict) {
                                       meta = (CFDictionaryRef)dict;
                                       if (meta)
                                           CFRetain(meta);
                                       dispatch_semaphore_signal(sem);
                                   });

    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW,
                                               (int64_t)(5 * NSEC_PER_SEC)));
    dispatch_release(sem);

    if (!meta) {
        /* No media session active — return empty info. */
        return info;
    }

    CFStringRef title =
        CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoTitle);
    if (title) {
        free(info->title);
        info->title = copy_cf_string(title);
    }

    CFStringRef artist =
        CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoArtist);
    if (artist) {
        free(info->artist);
        info->artist = copy_cf_string(artist);
    }

    CFStringRef album =
        CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoAlbum);
    if (album) {
        free(info->album);
        info->album = copy_cf_string(album);
    }

    CFNumberRef state =
        CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoPlaybackRate);
    if (state) {
        float rate = 0.0f;
        CFNumberGetValue(state, kCFNumberFloatType, &rate);
        info->playing = rate > 0.0f;
    }

    NSString *display_name = CFDictionaryGetValue(
        meta, kMRMediaRemoteNowPlayingApplicationDisplayNameUserInfoKey);
    if (display_name && [display_name isKindOfClass:[NSString class]]) {
        const char *utf8 = [display_name UTF8String];
        if (utf8) {
            free(info->app);
            info->app = strdup(utf8);
        }
    }

    CFRelease(meta);
    return info;
}

void media_free_info(struct media_info *info) {
    if (!info) return;
    free(info->app);
    free(info->title);
    free(info->artist);
    free(info->album);
    free(info);
}