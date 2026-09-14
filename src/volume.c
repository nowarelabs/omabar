#include "volume.h"
#include "event.h"
#include <CoreAudio/CoreAudio.h>
#include <stdlib.h>

static AudioObjectPropertyAddress g_address = {
    kAudioDevicePropertyVolumeScalar,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMain
};

static OSStatus volume_listener(AudioObjectID in_object_id,
                                UInt32 in_num_addresses,
                                const AudioObjectPropertyAddress *in_addresses,
                                void *in_client_data) {
    (void)in_object_id; (void)in_num_addresses; (void)in_addresses; (void)in_client_data;

    struct event event = { .type = EVENT_VOLUME_CHANGED };
    event_post(&event);
    return noErr;
}

void volume_begin(void) {
    AudioObjectAddPropertyListener(kAudioObjectSystemObject, &g_address,
                                   volume_listener, NULL);
}

void volume_end(void) {
    AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &g_address,
                                      volume_listener, NULL);
}

int volume_get_percentage(void) {
    Float32 volume = 0;
    UInt32 size = sizeof(volume);
    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                 &g_address, 0, NULL,
                                                 &size, &volume);
    if (status != noErr) return 0;
    return (int)(volume * 100);
}