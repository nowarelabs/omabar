#include "media.h"
#include "event.h"
#include "misc/extern.h"
#include <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <stdlib.h>

static NSObject *g_media_observer = NULL;

static void media_info_callback(CFDictionaryRef info) {
    (void)info;
    struct event event = { .type = EVENT_MEDIA_CHANGED };
    event_post(&event);
}

void media_begin(void) {
    g_media_observer = [[NSObject alloc] init];

    MRMediaRemoteRegisterForNowPlayingNotifications(media_info_callback);

    [[NSDistributedNotificationCenter defaultCenter] addObserver:g_media_observer
            selector:@selector(mediaChanged:)
                name:@"kMRMediaRemoteNowPlayingInfoDidChangeNotification"
              object:nil];
}

void media_end(void) {
    if (g_media_observer) {
        [[NSDistributedNotificationCenter defaultCenter] removeObserver:g_media_observer];
        g_media_observer = NULL;
    }
}

struct media_info *media_get_info(void) {
    struct media_info *info = calloc(1, sizeof(*info));
    if (!info) return NULL;

    CFDictionaryRef meta = MRMediaRemoteGetNowPlayingInfo();
    if (!meta) { free(info); return NULL; }

    CFStringRef title = CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoTitle);
    if (title) {
        info->title = (char *)CFStringGetCStringPtr(title, kCFStringEncodingUTF8);
    }
    CFStringRef artist = CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoArtist);
    if (artist) {
        info->artist = (char *)CFStringGetCStringPtr(artist, kCFStringEncodingUTF8);
    }
    CFStringRef album = CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoAlbum);
    if (album) {
        info->album = (char *)CFStringGetCStringPtr(album, kCFStringEncodingUTF8);
    }

    CFNumberRef state = CFDictionaryGetValue(meta, kMRMediaRemoteNowPlayingInfoPlaybackRate);
    if (state) {
        float rate = 0;
        CFNumberGetValue(state, kCFNumberFloatType, &rate);
        info->playing = rate > 0;
    }

    CFRelease(meta);
    return info;
}

void media_free_info(struct media_info *info) {
    if (info) free(info);
}