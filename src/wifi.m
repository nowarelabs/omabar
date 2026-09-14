#include "wifi.h"
#include "event.h"
#include <CoreWLAN/CoreWLAN.h>
#include <objc/runtime.h>

@interface OmabarWifiObserver : NSObject
- (void)wifiDidChange:(NSNotification *)note;
@end

@implementation OmabarWifiObserver
- (void)wifiDidChange:(NSNotification *)note {
    (void)note;
    struct event event = { .type = EVENT_WIFI_CHANGED };
    event_post(&event);
}
@end

static NSObject *g_wifi_observer = NULL;

void wifi_begin(void) {
    g_wifi_observer = [[OmabarWifiObserver alloc] init];
    [[NSNotificationCenter defaultCenter] addObserver:g_wifi_observer
            selector:@selector(wifiDidChange:)
                name:@"com.apple.airport.wifinetworkchange"
              object:nil];
}

void wifi_end(void) {
    if (g_wifi_observer) {
        [[NSNotificationCenter defaultCenter] removeObserver:g_wifi_observer];
        g_wifi_observer = NULL;
    }
}

const char *wifi_get_ssid(void) {
    CWInterface *interface = [CWInterface interface];
    if (!interface) return NULL;

    NSString *ssid = interface.ssid;
    if (!ssid) return NULL;
    return [ssid UTF8String];
}