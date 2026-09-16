#include "volume.h"
#include "event.h"
#include <CoreAudio/CoreAudio.h>
#include <stdlib.h>

static AudioObjectPropertyAddress kHardwareDevicePropertyAddress = {
    kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain
};

static AudioObjectPropertyAddress kVolumeMainPropertyAddress = {
    kAudioDevicePropertyVolumeScalar,
    kAudioObjectPropertyScopeOutput,
    kAudioObjectPropertyElementMain
};

static AudioObjectPropertyAddress kVolumeLeftPropertyAddress = {
    kAudioDevicePropertyVolumeScalar,
    kAudioObjectPropertyScopeOutput,
    1
};

static AudioObjectPropertyAddress kMuteMainPropertyAddress = {
    kAudioDevicePropertyMute,
    kAudioObjectPropertyScopeOutput,
    kAudioObjectPropertyElementMain
};

static AudioObjectPropertyAddress kMuteLeftPropertyAddress = {
    kAudioDevicePropertyMute,
    kAudioObjectPropertyScopeOutput,
    1
};

static float g_last_volume = -1.0f;

static OSStatus handler(AudioObjectID id, uint32_t address_count,
                        const AudioObjectPropertyAddress *addresses,
                        void *context) {
    (void)address_count; (void)addresses; (void)context;

    float volume = 0.0f;

    uint32_t muted_main = 0;
    uint32_t size = sizeof(muted_main);
    AudioObjectGetPropertyData(id, &kMuteMainPropertyAddress, 0, NULL,
                               &size, &muted_main);

    uint32_t muted_left = 0;
    size = sizeof(muted_left);
    AudioObjectGetPropertyData(id, &kMuteLeftPropertyAddress, 0, NULL,
                               &size, &muted_left);

    size = sizeof(float);
    float volume_main = 0.0f;
    AudioObjectGetPropertyData(id, &kVolumeMainPropertyAddress, 0, NULL,
                               &size, &volume_main);

    size = sizeof(float);
    float volume_left = 0.0f;
    AudioObjectGetPropertyData(id, &kVolumeLeftPropertyAddress, 0, NULL,
                               &size, &volume_left);

    if (volume_left > 0.0f) {
        volume = (muted_left || muted_main) ? 0.0f : volume_left;
    } else {
        volume = muted_main ? 0.0f : volume_main;
    }

    if (volume > g_last_volume + 1e-2f || volume < g_last_volume - 1e-2f) {
        g_last_volume = volume;
        int percentage = (int)(volume * 100.0f);
        struct event event = {
            .type = EVENT_VOLUME_CHANGED,
            .arg1 = (uint64_t)(uint32_t)percentage
        };
        event_post(&event);
    }

    return KERN_SUCCESS;
}

static AudioObjectID g_audio_id = 0;

static OSStatus device_changed(AudioObjectID id, uint32_t address_count,
                               const AudioObjectPropertyAddress *addresses,
                               void *context) {
    (void)addresses;

    AudioObjectID new_id = 0;
    uint32_t size = sizeof(AudioObjectID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &kHardwareDevicePropertyAddress, 0, NULL,
                               &size, &new_id);

    if (g_audio_id) {
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kMuteMainPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kMuteLeftPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kVolumeMainPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kVolumeLeftPropertyAddress,
                                          handler, NULL);
    }

    AudioObjectAddPropertyListener(new_id, &kMuteMainPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(new_id, &kMuteLeftPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(new_id, &kVolumeMainPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(new_id, &kVolumeLeftPropertyAddress,
                                   handler, NULL);

    g_last_volume = -1.0f;
    g_audio_id = new_id;

    handler(g_audio_id, address_count, addresses, context);
    return KERN_SUCCESS;
}

void forced_volume_event(void) {
    g_last_volume = -1.0f;
    handler(g_audio_id, 0, NULL, NULL);
}

static bool g_volume_events = false;

void volume_begin(void) {
    if (g_volume_events) return;
    g_volume_events = true;

    AudioObjectID id = 0;
    uint32_t size = sizeof(AudioObjectID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &kHardwareDevicePropertyAddress, 0, NULL,
                               &size, &id);

    g_audio_id = id;

    AudioObjectAddPropertyListener(id, &kMuteLeftPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(id, &kMuteMainPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(id, &kVolumeLeftPropertyAddress,
                                   handler, NULL);
    AudioObjectAddPropertyListener(id, &kVolumeMainPropertyAddress,
                                   handler, NULL);

    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                   &kHardwareDevicePropertyAddress,
                                   device_changed, NULL);
}

void volume_end(void) {
    if (g_audio_id) {
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kMuteMainPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kMuteLeftPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kVolumeMainPropertyAddress,
                                          handler, NULL);
        AudioObjectRemovePropertyListener(g_audio_id,
                                          &kVolumeLeftPropertyAddress,
                                          handler, NULL);
        g_audio_id = 0;
    }
    AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
                                      &kHardwareDevicePropertyAddress,
                                      device_changed, NULL);
}

static int g_percentage_cache = -1;

int volume_get_percentage(void) {
    if (g_audio_id) {
        Float32 volume = 0.0f;
        UInt32 size = sizeof(volume);
        OSStatus status = AudioObjectGetPropertyData(g_audio_id,
                                                     &kVolumeMainPropertyAddress,
                                                     0, NULL, &size, &volume);
        if (status == noErr) {
            g_percentage_cache = (int)(volume * 100.0f);
            return g_percentage_cache;
        }
    }
    return g_percentage_cache > 0 ? g_percentage_cache : 0;
}