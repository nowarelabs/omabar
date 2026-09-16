#include "hotload.h"
#include "event.h"
#include <CoreServices/CoreServices.h>
#include <stdlib.h>
#include <time.h>

static FSEventStreamRef g_stream = NULL;
static bool g_running = false;
static int64_t g_last_reload = 0;
#define HOTLOAD_DEBOUNCE_NS ((int64_t)1 << 30) /* ~1 second */

static void event_callback(
    ConstFSEventStreamRef streamRef,
    void *clientCallBackInfo,
    size_t numEvents,
    void *eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId eventIds[])
{
    (void)streamRef; (void)clientCallBackInfo; (void)numEvents;
    (void)eventPaths; (void)eventFlags; (void)eventIds;

    int64_t now = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW_APPROX);
    if (now - g_last_reload < HOTLOAD_DEBOUNCE_NS) return;

    g_last_reload = now;
    struct event event = { .type = EVENT_HOTLOAD };
    event_post(&event);
}

void hotload_begin(void) {
    if (g_running) return;

    CFStringRef path = CFSTR("/tmp/omabar_config");
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&path,
                                            1, &kCFTypeArrayCallBacks);
    FSEventStreamContext ctx = { 0, NULL, NULL, NULL, NULL };

    g_stream = FSEventStreamCreate(NULL, event_callback, &ctx,
                                   pathsToWatch,
                                   kFSEventStreamEventIdSinceNow,
                                   1.0, /* latency */
                                   kFSEventStreamCreateFlagNone);

    if (g_stream) {
        FSEventStreamScheduleWithRunLoop(g_stream, CFRunLoopGetMain(),
                                         kCFRunLoopCommonModes);
        FSEventStreamStart(g_stream);
        g_running = true;
    }

    CFRelease(pathsToWatch);
}

void hotload_end(void) {
    if (!g_running) return;
    FSEventStreamStop(g_stream);
    FSEventStreamInvalidate(g_stream);
    FSEventStreamRelease(g_stream);
    g_stream = NULL;
    g_running = false;
}

bool hotload_is_running(void) {
    return g_running;
}