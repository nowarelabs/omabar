#include "power.h"
#include "event.h"
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>

static CFRunLoopSourceRef g_power_source = NULL;

static void power_callback(void *context) {
    (void)context;
    struct event event = { .type = EVENT_POWER_CHANGED };
    event_post(&event);
}

void power_begin(void) {
    g_power_source = IOPSNotificationCreateRunLoopSource(power_callback, NULL);
    if (g_power_source)
        CFRunLoopAddSource(CFRunLoopGetMain(), g_power_source,
                           kCFRunLoopCommonModes);
}

void power_end(void) {
    if (g_power_source) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), g_power_source,
                              kCFRunLoopCommonModes);
        CFRelease(g_power_source);
        g_power_source = NULL;
    }
}

int power_get_charge(void) {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (!list) return 0;

    int capacity = 0;
    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef desc = IOPSGetPowerSourceDescription(info, ps);
        if (!desc) continue;

        CFNumberRef cap = CFDictionaryGetValue(desc, CFSTR(kIOPSCurrentCapacityKey));
        if (cap) CFNumberGetValue(cap, kCFNumberIntType, &capacity);
    }
    CFRelease(list);
    CFRelease(info);
    return capacity;
}

bool power_is_charging(void) {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (!list) return false;

    bool charging = false;
    CFIndex count = CFArrayGetCount(list);
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef desc = IOPSGetPowerSourceDescription(info, ps);
        if (!desc) continue;

        CFStringRef state = CFDictionaryGetValue(desc, CFSTR(kIOPSPowerSourceStateKey));
        if (state && CFStringCompare(state, CFSTR(kIOPSBatteryPowerValue), 0) != kCFCompareEqualTo)
            charging = true;
    }
    CFRelease(list);
    CFRelease(info);
    return charging;
}