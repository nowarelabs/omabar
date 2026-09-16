#include "wifi.h"
#include "event.h"
#include <CoreWLAN/CoreWLAN.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <stdlib.h>
#include <string.h>

static void update_ssid(SCDynamicStoreRef store, CFArrayRef keys, void *info) {
    (void)store; (void)keys; (void)info;

    @autoreleasepool {
        char *ssid = strdup(wifi_get_ssid() ? wifi_get_ssid() : "null");
        struct event event = { .type = EVENT_WIFI_CHANGED, .data = ssid };
        event_post(&event);
    }
}

static SCDynamicStoreRef g_wifi_store = NULL;
static CFRunLoopSourceRef g_wifi_loop_source = NULL;

void forced_network_event(void) {
    update_ssid(NULL, NULL, NULL);
}

void wifi_begin(void) {
    if (g_wifi_store) return;

    SCDynamicStoreContext context = { 0, NULL, NULL, NULL, NULL };
    g_wifi_store = SCDynamicStoreCreate(NULL, CFSTR("network"),
                                        update_ssid, &context);
    if (!g_wifi_store) return;

    const void *values[] = { CFSTR(".*/Network/Global/IPv4") };
    CFArrayRef keys = CFArrayCreate(NULL, values, 1, &kCFTypeArrayCallBacks);
    SCDynamicStoreSetNotificationKeys(g_wifi_store, NULL, keys);
    CFRelease(keys);

    g_wifi_loop_source =
        SCDynamicStoreCreateRunLoopSource(NULL, g_wifi_store, 0);
    if (g_wifi_loop_source)
        CFRunLoopAddSource(CFRunLoopGetMain(), g_wifi_loop_source,
                           kCFRunLoopCommonModes);
}

void wifi_end(void) {
    if (g_wifi_loop_source) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), g_wifi_loop_source,
                              kCFRunLoopCommonModes);
        CFRelease(g_wifi_loop_source);
        g_wifi_loop_source = NULL;
    }
    if (g_wifi_store) {
        CFRelease(g_wifi_store);
        g_wifi_store = NULL;
    }
}

const char *wifi_get_ssid(void) {
    CWInterface *interface = [[CWWiFiClient sharedWiFiClient] interface];
    if (!interface) return NULL;

    NSString *ssid = interface.ssid;
    if (!ssid) return NULL;
    return [ssid UTF8String];
}