#ifndef OMABAR_EXTERN_H
#define OMABAR_EXTERN_H

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <dispatch/dispatch.h>
#include <stdint.h>
#include <stdbool.h>

/* ═══════════════════════════════════════════════════════════════
   DisplayServices — /usr/lib/libDisplayServices.dylib
   ═══════════════════════════════════════════════════════════════ */
extern CGError DisplayServicesGetBrightness(uint32_t did, float *brightness);
extern CGError DisplayServicesCanChangeBrightness(uint32_t did);
extern CGError DisplayServicesRegisterForBrightnessChangeNotifications(
    uint32_t did, uint32_t passthrough, void *callback);
extern CGError DisplayServicesUnregisterForBrightnessChangeNotifications(
    uint32_t did, uint32_t passthrough);
extern CGError DisplayServicesRegisterForAmbientLightCompensationNotifications(
    uint32_t did, uint32_t passthrough, void *callback);
extern CGError DisplayServicesUnregisterForAmbientLightCompensationNotifications(
    uint32_t did, uint32_t passthrough);
extern CGError DisplayServicesAmbientLightCompensationEnabled(
    uint32_t did, bool *out);

/* ═══════════════════════════════════════════════════════════════
   SkyLight core — /System/Library/PrivateFrameworks/SkyLight.framework
   (for functions not resolved via private_shim.c dlsym trampolines)
   ═══════════════════════════════════════════════════════════════ */
extern uint32_t SLSMainConnectionID(void);

/* Window creation / lifecycle */
extern CGError SLSNewWindowWithOpaqueShapeAndContext(
    int cid, int type, CFTypeRef region, CFTypeRef opaque_shape,
    int options, uint64_t *tags, float x, float y, int tag_size,
    uint32_t *wid, void *context);
extern CGError SLSNewWindow(
    int cid, int type, float x, float y,
    CFTypeRef region, uint64_t *wid);
extern CGError SLSReleaseWindow(int cid, uint32_t wid);
extern CGError SLSSetWindowResolution(int cid, uint32_t wid, double res);
extern CGContextRef SLWindowContextCreate(
    int cid, uint32_t wid, CFDictionaryRef options);

/* Window properties */
extern CGError SLSSetWindowShape(
    int cid, uint32_t wid, float x_offset, float y_offset,
    CFTypeRef shape);
extern CGError SLSSetWindowOpaqueShape(
    int cid, uint32_t wid, float x_offset, float y_offset,
    CFTypeRef region);
extern CGError SLSSetWindowOpacity(int cid, uint32_t wid, bool isOpaque);
extern CGError SLSSetWindowAlpha(int cid, uint32_t wid, float alpha);
extern CGError SLSSetWindowLevel(int cid, uint32_t wid, int level);
extern CGError SLSSetWindowTags(
    int cid, uint32_t wid, uint64_t *tags, int tag_size);
extern CGError SLSClearWindowTags(
    int cid, uint32_t wid, uint64_t *tags, int tag_size);
extern CGError SLSSetWindowBackgroundBlurRadius(
    int cid, uint32_t wid, uint32_t radius);
extern CGError SLSOrderWindow(int cid, uint32_t wid, int mode,
                              uint32_t relativeToWID);
extern CGError SLSSetMouseEventEnableFlags(
    int cid, uint32_t wid, int flags);
extern CGError SLSSetWindowEventMask(int cid, uint32_t wid, uint32_t mask);
extern CGError SLSSetWindowFloatingLevel(int cid, uint32_t wid, int level);
extern CGError SLSWindowSetShadowProperties(
    uint32_t wid, CFDictionaryRef properties);

/* Window ordering groups */
extern CGError SLSAddWindowToWindowOrderingGroup(
    int cid, uint32_t parent_wid, uint32_t child_wid, int order);
extern CGError SLSRemoveFromOrderingGroup(int cid, uint32_t wid);

/* Window movement / space re-association */
extern CGError SLSMoveWindow(int cid, uint32_t wid, CGPoint *point);
extern CGError SLSMoveWindowWithGroup(int cid, uint32_t wid, CGPoint *point);
extern CGError SLSReassociateWindowsSpacesByGeometry(
    int cid, CFArrayRef wids);
extern CGError SLSMoveWindowsToManagedSpace(
    int cid, CFArrayRef window_list, uint64_t sid);

/* Activation / tracking regions */
extern CGError CGSNewRegionWithRect(CGRect *rect, CFTypeRef *outRegion);
extern CGError SLSAddActivationRegion(uint32_t cid, uint32_t wid,
                                      CFTypeRef region);
extern CGError SLSClearActivationRegion(uint32_t cid, uint32_t wid);
extern CGError SLSAddTrackingRect(uint32_t cid, uint32_t wid, CGRect rect);
extern CGError SLSRemoveAllTrackingAreas(uint32_t cid, uint32_t wid);

/* Capture / queries */
extern void SLSCaptureWindowsContentsToRectWithOptions(
    uint32_t cid, uint64_t *wid, bool meh, CGRect bounds,
    uint32_t flags, CGImageRef *image);
extern CGImageRef SLSHWCaptureSpace(int64_t cid, int64_t sid, int64_t flags);
extern int SLSGetScreenRectForWindow(uint32_t cid, uint32_t wid,
                                     CGRect *out);
extern CGError SLSFlushWindowContentRegion(int cid, uint32_t wid,
                                           void *dirty);

/* Transactions (batched window ops — some match our SLSTransactionCreate) */
extern CFTypeRef SLSTransactionCreate(int cid);
extern CGError SLSTransactionCommit(CFTypeRef transaction, bool async);
extern CGError SLSTransactionOrderWindow(
    CFTypeRef transaction, uint32_t wid, int mode, uint32_t relativeToWID);
extern CGError SLSTransactionSetWindowLevel(
    CFTypeRef transaction, uint32_t wid, int level);
extern CGError SLSTransactionSetWindowShape(
    CFTypeRef transaction, uint32_t wid, float x_offset, float y_offset,
    CFTypeRef shape);
extern CGError SLSTransactionSetSurfaceBounds(
    CFTypeRef transaction, uint32_t wid, uint32_t sid, CGRect bounds);
extern CGError SLSTransactionMoveWindowWithGroup(
    CFTypeRef transaction, uint32_t wid, CGPoint point);
extern uint64_t SLSTransactionCreate_full(uint32_t cid);

/* Window queries (iterator API) */
extern CGError SLSGetWindowOwner(int cid, uint32_t wid, int *out_cid);
extern CGError SLSConnectionGetPID(int cid, pid_t *pid);
extern CFArrayRef SLSCopyWindowsWithOptionsAndTags(
    int cid, uint32_t owner, CFArrayRef spaces, uint32_t options,
    uint64_t *set_tags, uint64_t *clear_tags);
extern CFTypeRef SLSWindowQueryWindows(
    int cid, CFArrayRef windows, uint32_t options);
extern CFTypeRef SLSWindowQueryResultCopyWindows(CFTypeRef window_query);
extern int SLSWindowIteratorGetCount(CFTypeRef iterator);
extern bool SLSWindowIteratorAdvance(CFTypeRef iterator);
extern uint32_t SLSWindowIteratorGetParentID(CFTypeRef iterator);
extern uint32_t SLSWindowIteratorGetWindowID(CFTypeRef iterator);
extern uint64_t SLSWindowIteratorGetTags(CFTypeRef iterator);
extern uint64_t SLSWindowIteratorGetAttributes(CFTypeRef iterator);

/* ── Region support (CoreGraphicsPrivate) ── */
extern CFTypeRef CGRegionCreateEmptyRegion(void);

/* ═══════════════════════════════════════════════════════════════
   SkyLight — Surface system
   ═══════════════════════════════════════════════════════════════ */
extern uint32_t SLSAddSurface(int cid, uint32_t wid, uint32_t *outSID);
extern CGError SLSRemoveSurface(int cid, uint32_t wid, uint32_t sid);
extern CGError SLSBindSurface(
    int cid, uint32_t wid, uint32_t sid, int param1, int param2,
    unsigned int context_id);
extern CGError SLSSetSurfaceBounds(
    int cid, uint32_t wid, uint32_t sid, CGRect bounds);
extern CGError SLSSetSurfaceOpacity(
    int cid, uint32_t wid, uint32_t sid, bool opaque);
extern CGError SLSOrderSurface(
    int cid, uint32_t wid, uint32_t surface, int mode,
    uint32_t other_surface);
extern CGError SLSSetSurfaceResolution(
    int cid, uint32_t wid, uint32_t sid, double scale);
extern CGError SLSFlushSurface(
    int cid, uint32_t wid, uint32_t surface, int param);
extern CGError SLSSetSurfaceColorSpace(
    int cid, uint32_t wid, uint32_t surface, CGColorSpaceRef color_space);

/* ═══════════════════════════════════════════════════════════════
   SkyLight — Update batching
   ═══════════════════════════════════════════════════════════════ */
extern CGError SLSDisableUpdate(int cid);
extern CGError SLSReenableUpdate(int cid);

/* ═══════════════════════════════════════════════════════════════
   SkyLight — Space system
   ═══════════════════════════════════════════════════════════════ */
extern CFArrayRef SLSCopyManagedDisplays(int cid);
extern CFArrayRef SLSCopyManagedDisplaySpaces(int cid);
extern uint64_t SLSGetActiveSpace(int cid);
extern CFStringRef SLSCopyManagedDisplayForSpace(int cid, uint64_t sid);
extern CFStringRef SLSManagedDisplayGetCurrentSpace(
    int cid, CGDirectDisplayID did);
extern uint64_t SLSSpaceCreate(int cid, int one, int zero);
extern CGError SLSShowSpaces(int cid, CFArrayRef space_list);
extern CGError SLSHideSpaces(int cid, CFArrayRef space_list);
extern CGError SLSSpaceAddWindowsAndRemoveFromSpaces(
    int cid, int sid, CFArrayRef array, int seven);
extern CGError SLSSpaceSetAbsoluteLevel(int cid, int sid, int level);
extern int SLSSpaceGetType(int cid, uint64_t sid);

/* ═══════════════════════════════════════════════════════════════
   SkyLight — Display / Menu bar queries
   ═══════════════════════════════════════════════════════════════ */
extern CFStringRef SLSCopyBestManagedDisplayForRect(int cid, CGRect rect);
extern CFStringRef SLSCopyBestManagedDisplayForPoint(int cid, CGPoint point);
extern CFStringRef SLSCopyActiveMenuBarDisplayIdentifier(int cid);
extern CGError SLSGetMenuBarAutohideEnabled(int cid, int *enabled);
extern CGError SLSGetRevealedMenuBarBounds(CGRect *rect, int cid,
                                            uint64_t sid);
extern CGError SLSSetMenuBarVisibilityOverrideOnDisplay(
    int cid, int did, bool override);
extern CGError SLSSetMenuBarAutohideEnabled(int cid, bool enabled);
extern CGError SLSGetCurrentCursorLocation(int cid, CGPoint *point);

/* ═══════════════════════════════════════════════════════════════
   SkyLight — Notifications
   ═══════════════════════════════════════════════════════════════ */
extern CGError SLSRegisterNotifyProc(void *callback, uint32_t event,
                                     void *context);
extern CGError SLSRequestNotificationsForWindows(
    int cid, uint32_t *wid_list, uint32_t list_count);

/* ═══════════════════════════════════════════════════════════════
   CoreGraphicsDisplay — UUID helpers
   ═══════════════════════════════════════════════════════════════ */
extern CFUUIDRef CGDisplayCreateUUIDFromDisplayID(uint32_t did);

/* ═══════════════════════════════════════════════════════════════
   MediaRemote — private MediaRemote.framework (ObjC translation
   units only; the block-based API needs NSDictionary/NSString)
   ═══════════════════════════════════════════════════════════════ */
#ifdef __OBJC__
extern void MRMediaRemoteRegisterForNowPlayingNotifications(
    dispatch_queue_t queue);
extern void MRMediaRemoteUnregisterForNowPlayingNotifications(void);
extern void MRMediaRemoteGetNowPlayingInfo(
    dispatch_queue_t queue, void (^block)(NSDictionary *dict));
extern CFStringRef kMRMediaRemoteNowPlayingInfoTitle;
extern CFStringRef kMRMediaRemoteNowPlayingInfoArtist;
extern CFStringRef kMRMediaRemoteNowPlayingInfoAlbum;
extern CFStringRef kMRMediaRemoteNowPlayingInfoPlaybackRate;
extern NSString *kMRMediaRemoteNowPlayingApplicationDisplayNameUserInfoKey;
#endif /* __OBJC__ */

/* ═══════════════════════════════════════════════════════════════
   Private symbols that exist on some macOS releases but are not
   linkable on all — resolved at runtime via dlsym in
   src/private_shim.c and declared here for all call sites.
   ═══════════════════════════════════════════════════════════════ */
extern void SLSSetWindowOrigin(int cid, uint32_t wid, float x, float y);
extern void SLSUnbindSurface(int cid, uint32_t wid, uint32_t sid);
extern CFNumberRef SLSGetSpaceIDForUUID(int cid, CFStringRef uuid);
extern CGDirectDisplayID SLSGetDisplayIDForSpace(int cid, uint64_t sid);
extern CGError (*SBSLSTransactionAddPostDecodeAction)(
    CFTypeRef transaction, void (^block)());

/* ═══════════════════════════════════════════════════════════════
   Global CGS connection — initialised in window.c via
   SLSMainConnectionID() on first use.
   ═══════════════════════════════════════════════════════════════ */
extern uint32_t g_connection;

#endif