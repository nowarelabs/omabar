#include "power.h"
#include "event.h"
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <string.h>

#define POWER_AC_KEY      CFSTR(kIOPMACPowerKey)
#define POWER_BATTERY_KEY CFSTR(kIOPMBatteryPowerKey)
#define POWER_UPS_KEY     CFSTR(kIOPMUPSPowerKey)

#define POWER_AC      1
#define POWER_BATTERY 2

static uint32_t g_power_source = 0;
static CFRunLoopSourceRef g_power_source_ref = NULL;

static int power_get_capacity(void) {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (!list) return 0;

    int capacity = 0;
    int count = 0;
    CFIndex n = CFArrayGetCount(list);
    for (CFIndex i = 0; i < n; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef desc = IOPSGetPowerSourceDescription(info, ps);
        if (!desc) continue;

        CFNumberRef cap = CFDictionaryGetValue(desc,
                                               CFSTR(kIOPSCurrentCapacityKey));
        if (cap) {
            int v = 0;
            if (CFNumberGetValue(cap, kCFNumberIntType, &v)) {
                capacity += v;
                count++;
            }
        }
    }
    CFRelease(list);
    CFRelease(info);
    return count > 0 ? capacity / count : 0;
}

static bool power_is_finishing_charge(void) {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (!list) return false;

    bool charging = false;
    CFIndex n = CFArrayGetCount(list);
    for (CFIndex i = 0; i < n; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef desc = IOPSGetPowerSourceDescription(info, ps);
        if (!desc) continue;

        CFStringRef is_present = CFDictionaryGetValue(desc,
                                                      CFSTR(kIOPSIsPresentKey));
        if (is_present && CFBooleanGetValue((CFBooleanRef)is_present)) {
            CFStringRef state = CFDictionaryGetValue(desc,
                                                     CFSTR(kIOPSPowerSourceStateKey));
            if (state &&
                CFStringCompare(state, CFSTR(kIOPSBatteryPowerValue), 0)
                    != kCFCompareEqualTo) {
                charging = true;
                break;
            }
        }
    }
    CFRelease(list);
    CFRelease(info);
    return charging;
}

static void power_handler(void *context) {
    (void)context;

    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    CFStringRef type = IOPSGetProvidingPowerSourceType(info);
    uint32_t source = 0;
    if (type && CFStringCompare(type, POWER_AC_KEY, 0) == kCFCompareEqualTo)
        source = POWER_AC;
    else if (type && CFStringCompare(type, POWER_BATTERY_KEY, 0) == kCFCompareEqualTo)
        source = POWER_BATTERY;
    else if (type && CFStringCompare(type, POWER_UPS_KEY, 0) == kCFCompareEqualTo)
        source = POWER_BATTERY;

    if (info) CFRelease(info);

    if (source == 0) return;

    if (source != g_power_source) {
        g_power_source = source;
    }

    int percentage = power_get_capacity();
    bool charging = (source == POWER_AC) || power_is_finishing_charge();

    struct event event = {
        .type = EVENT_POWER_CHANGED,
        .arg1 = (uint64_t)(uint32_t)percentage,
        .arg2 = (uint64_t)(uint32_t)(charging ? 1 : 0)
    };
    event_post(&event);
}

void forced_power_event(void) {
    power_handler(NULL);
}

static bool g_power_events = false;

void power_begin(void) {
    if (g_power_events) return;
    g_power_events = true;

    g_power_source_ref = IOPSNotificationCreateRunLoopSource(power_handler, NULL);
    if (g_power_source_ref)
        CFRunLoopAddSource(CFRunLoopGetMain(), g_power_source_ref,
                           kCFRunLoopCommonModes);
    forced_power_event();
}

void power_end(void) {
    if (g_power_source_ref) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), g_power_source_ref,
                              kCFRunLoopCommonModes);
        CFRelease(g_power_source_ref);
        g_power_source_ref = NULL;
    }
}

int power_get_charge(void) {
    return power_get_capacity();
}

bool power_is_charging(void) {
    return g_power_source == POWER_AC || power_is_finishing_charge();
}