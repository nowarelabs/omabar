#include "misc/extern.h"
#include <dlfcn.h>
#include <stdio.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

#define PRIVATE_FRAMEWORK "/System/Library/PrivateFrameworks/SkyLight.framework/SkyLight"

static void *framework_handle(void) {
    static void *handle = NULL;
    if (!handle) {
        handle = dlopen(PRIVATE_FRAMEWORK, RTLD_LAZY | RTLD_LOCAL);
        if (!handle) fprintf(stderr, "omabar: failed to dlopen SkyLight: %s\n", dlerror());
    }
    return handle;
}

static void *resolve_symbol(const char *name) {
    void *handle = framework_handle();
    if (!handle) return NULL;
    void *sym = dlsym(handle, name);
    if (!sym) fprintf(stderr, "omabar: missing private symbol %s: %s\n", name, dlerror());
    return sym;
}

static void *resolve_symbol_quiet(const char *name) {
    void *handle = framework_handle();
    if (!handle) return NULL;
    return dlsym(handle, name);
}

void SLSSetWindowOrigin(int cid, uint32_t wid, float x, float y) {
    void *sym = resolve_symbol("SLSSetWindowOrigin");
    if (!sym) return;
    ((void (*)(int, uint32_t, float, float))sym)(cid, wid, x, y);
}

void SLSUnbindSurface(int cid, uint32_t wid, uint32_t sid) {
    void *sym = resolve_symbol_quiet("SLSUnbindSurface");
    if (!sym) return;
    ((void (*)(int, uint32_t, uint32_t))sym)(cid, wid, sid);
}

CFNumberRef SLSGetSpaceIDForUUID(int cid, CFStringRef uuid) {
    void *sym = resolve_symbol("SLSGetSpaceIDForUUID");
    if (!sym) return NULL;
    return ((CFNumberRef (*)(int, CFStringRef))sym)(cid, uuid);
}

CGDirectDisplayID SLSGetDisplayIDForSpace(int cid, uint64_t sid) {
    void *sym = resolve_symbol("SLSGetDisplayIDForSpace");
    if (!sym) return 0;
    return ((CGDirectDisplayID (*)(int, uint64_t))sym)(cid, sid);
}

CGError (*SBSLSTransactionAddPostDecodeAction)(CFTypeRef transaction,
                                               void (^block)());

__attribute__((constructor)) static void resolve_optional_symbols(void) {
    void *handle = framework_handle();
    if (!handle) return;
    SBSLSTransactionAddPostDecodeAction =
        dlsym(handle, "SBSLSTransactionAddPostDecodeAction");
}