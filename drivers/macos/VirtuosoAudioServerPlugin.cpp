// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtuosoAudioServerPlugin.cpp — macOS AudioServerPlugIn (HAL plugin)
//
// Architecture: CoreAudio AudioServerPlugIn — a user-space HAL extension
// that adds "Virtuoso Virtual 7.1" as a selectable audio device in System
// Preferences.
//
// Key design (from ADR-001):
//   1. Load path: /Library/Audio/Plug-Ins/HAL/VirtuosoAudioPlugin.driver/
//   2. Device appears in Audio MIDI Setup as "Virtuoso Virtual 7.1"
//   3. 8-channel (7.1) CoreAudio device at 44.1, 48, 96, 192 kHz
//   4. Zero-copy IOBufferList rendering for minimal latency
//   5. Apple Developer ID signed + notarized (mandatory for Gatekeeper)
//
// Build: xcodebuild with AudioServerPlugIn target (macOS 10.14+)
// Sign:  codesign --sign "Developer ID Application: ..." --deep ...
// Notarize: xcrun notarytool submit ...
//
// This file implements the required AudioServerPlugIn COM/CF dispatch table
// and the driver host interface. Actual audio routing is done in
// VirtuosoPluginDevice.cpp (creates the virtual Clock, Stream, and Device
// objects).
//
// Compile flag requirements: -framework CoreAudio -framework CoreFoundation

#include <CoreAudio/AudioServerPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdint>
#include <cstring>
#include <mach/mach_time.h>
#include <os/log.h>


// ---------------------------------------------------------------------------
// Plugin constants
// ---------------------------------------------------------------------------
static const AudioObjectID kObjectID_PlugIn = kAudioObjectPlugInObject;
static const AudioObjectID kObjectID_Device = 2;
static const AudioObjectID kObjectID_Stream = 3;
static const AudioObjectID kObjectID_Clock = 4;

static const Float64 kSampleRate = 48000.0;
static const UInt32 kNumChannels = 8; // 7.1 surround
static const UInt32 kBytesPerFrame = kNumChannels * sizeof(float);
static const UInt32 kRingBufferFrameCount = 16384;

static os_log_t gLog = OS_LOG_DEFAULT;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static OSStatus
VirtuosoPlugIn_CreateDevice(AudioServerPlugInHostRef host,
                            CFDictionaryRef settings,
                            const AudioServerPlugInClientInfo *clientInfo,
                            AudioObjectID *outDeviceObjectID);

// ---------------------------------------------------------------------------
// Plugin interface dispatch table (required by CoreAudio HAL)
// ---------------------------------------------------------------------------
static AudioServerPlugInDriverInterface gAudioServerPlugInDriverInterface = {
    .QueryInterface = [](void *, CFUUIDBytes iid, LPVOID *out) -> HRESULT {
      // Only respond to IAudioServerPlugInDriver
      CFUUIDRef requestedIID =
          CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
      HRESULT result = E_NOINTERFACE;
      if (CFEqual(requestedIID, kAudioServerPlugInDriverInterfaceID)) {
        *out = &gAudioServerPlugInDriverInterface;
        result = S_OK;
      }
      CFRelease(requestedIID);
      return result;
    },
    .AddRef = [](void *) -> ULONG { return 1; },
    .Release = [](void *) -> ULONG { return 1; },

    .Initialize = [](AudioServerPlugInDriverRef driverRef,
                     AudioServerPlugInHostRef hostRef) -> OSStatus {
      os_log(gLog, "VirtuosoPlugIn: Initialize");
      // Phase 4b: register the virtual device with HAL
      AudioObjectID deviceID = kAudioObjectUnknown;
      return VirtuosoPlugIn_CreateDevice(hostRef, nullptr, nullptr, &deviceID);
    },

    .CreateDevice = [](AudioServerPlugInDriverRef, AudioServerPlugInHostRef h,
                       CFDictionaryRef s, const AudioServerPlugInClientInfo *c,
                       AudioObjectID *out) -> OSStatus {
      return VirtuosoPlugIn_CreateDevice(h, s, c, out);
    },

    .DestroyDevice = [](AudioServerPlugInDriverRef, AudioObjectID) -> OSStatus {
      return kAudioHardwareNoError;
    },

    .AddDeviceClient = [](AudioServerPlugInDriverRef, AudioObjectID,
                          const AudioServerPlugInClientInfo *) -> OSStatus {
      return kAudioHardwareNoError;
    },
    .RemoveDeviceClient = [](AudioServerPlugInDriverRef, AudioObjectID,
                             const AudioServerPlugInClientInfo *) -> OSStatus {
      return kAudioHardwareNoError;
    },

    .PerformDeviceConfigurationChange =
        [](AudioServerPlugInDriverRef, AudioObjectID, UInt64,
           void *) -> OSStatus { return kAudioHardwareNoError; },
    .AbortDeviceConfigurationChange =
        [](AudioServerPlugInDriverRef, AudioObjectID, UInt64,
           void *) -> OSStatus { return kAudioHardwareNoError; },

    // Property I/O stubs (Phase 4b: implement
    // kAudioDevicePropertyBufferFrameSize, kAudioStreamPropertyVirtualFormat,
    // kAudioDevicePropertyNominalSampleRate, etc.)
    .HasProperty = [](AudioServerPlugInDriverRef, AudioObjectID, pid_t,
                      const AudioObjectPropertyAddress *addr) -> Boolean {
      if (!addr)
        return false;
      switch (addr->mSelector) {
      case kAudioObjectPropertyName:
      case kAudioDevicePropertyDeviceUID:
      case kAudioDevicePropertyModelUID:
      case kAudioStreamPropertyVirtualFormat:
      case kAudioDevicePropertyNominalSampleRate:
      case kAudioDevicePropertyAvailableNominalSampleRates:
      case kAudioDevicePropertyBufferFrameSize:
        return true;
      default:
        return false;
      }
    },

    .IsPropertySettable = [](AudioServerPlugInDriverRef, AudioObjectID, pid_t,
                             const AudioObjectPropertyAddress *,
                             Boolean *outIsSettable) -> OSStatus {
      if (outIsSettable)
        *outIsSettable = false;
      return kAudioHardwareNoError;
    },

    .GetPropertyDataSize = [](AudioServerPlugInDriverRef, AudioObjectID, pid_t,
                              const AudioObjectPropertyAddress *addr, UInt32,
                              const void *, UInt32 *outSize) -> OSStatus {
      if (!addr || !outSize)
        return kAudioHardwareBadObjectError;
      switch (addr->mSelector) {
      case kAudioObjectPropertyName:
      case kAudioDevicePropertyDeviceUID:
      case kAudioDevicePropertyModelUID:
        *outSize = sizeof(CFStringRef);
        break;
      case kAudioStreamPropertyVirtualFormat:
        *outSize = sizeof(AudioStreamBasicDescription);
        break;
      case kAudioDevicePropertyNominalSampleRate:
        *outSize = sizeof(Float64);
        break;
      case kAudioDevicePropertyBufferFrameSize:
        *outSize = sizeof(UInt32);
        break;
      default:
        return kAudioHardwareUnknownPropertyError;
      }
      return kAudioHardwareNoError;
    },

    .GetPropertyData = [](AudioServerPlugInDriverRef, AudioObjectID objectID,
                          pid_t, const AudioObjectPropertyAddress *addr, UInt32,
                          const void *, UInt32 *ioSize, void *out) -> OSStatus {
      if (!addr || !out)
        return kAudioHardwareBadObjectError;

      switch (addr->mSelector) {
      case kAudioObjectPropertyName: {
        CFStringRef *str = static_cast<CFStringRef *>(out);
        *str = CFSTR("Virtuoso Virtual 7.1");
        *ioSize = sizeof(CFStringRef);
        break;
      }
      case kAudioDevicePropertyDeviceUID: {
        CFStringRef *str = static_cast<CFStringRef *>(out);
        *str = CFSTR("audio.virtuoso.app.virtual71");
        *ioSize = sizeof(CFStringRef);
        break;
      }
      case kAudioStreamPropertyVirtualFormat: {
        auto *asbd = static_cast<AudioStreamBasicDescription *>(out);
        asbd->mSampleRate = kSampleRate;
        asbd->mFormatID = kAudioFormatLinearPCM;
        asbd->mFormatFlags = kAudioFormatFlagIsFloat |
                             kAudioFormatFlagIsPacked |
                             kAudioFormatFlagIsNonInterleaved;
        asbd->mBytesPerPacket = sizeof(float);
        asbd->mFramesPerPacket = 1;
        asbd->mBytesPerFrame = sizeof(float);
        asbd->mChannelsPerFrame = kNumChannels;
        asbd->mBitsPerChannel = 32;
        *ioSize = sizeof(AudioStreamBasicDescription);
        break;
      }
      case kAudioDevicePropertyNominalSampleRate:
        *static_cast<Float64 *>(out) = kSampleRate;
        *ioSize = sizeof(Float64);
        break;
      case kAudioDevicePropertyBufferFrameSize:
        *static_cast<UInt32 *>(out) = 512;
        *ioSize = sizeof(UInt32);
        break;
      default:
        return kAudioHardwareUnknownPropertyError;
      }
      return kAudioHardwareNoError;
    },

    .SetPropertyData = [](AudioServerPlugInDriverRef, AudioObjectID, pid_t,
                          const AudioObjectPropertyAddress *, UInt32,
                          const void *, UInt32, const void *) -> OSStatus {
      return kAudioHardwareUnsupportedOperationError;
    },

    // IO cycle — Phase 4b: return ring-buffer data to the HAL
    .StartIO = [](AudioServerPlugInDriverRef, AudioObjectID,
                  UInt32) -> OSStatus { return kAudioHardwareNoError; },
    .StopIO = [](AudioServerPlugInDriverRef, AudioObjectID) -> OSStatus {
      return kAudioHardwareNoError;
    },

    .GetZeroTimeStamp = [](AudioServerPlugInDriverRef, AudioObjectID,
                           UInt32 clientID, Float64 *outSampleTime,
                           UInt64 *outHostTime, UInt64 *outSeed) -> OSStatus {
      *outSampleTime = 0.0;
      *outHostTime = mach_absolute_time();
      *outSeed = 1;
      return kAudioHardwareNoError;
    },

    .WillDoIOOperation = [](AudioServerPlugInDriverRef, AudioObjectID, UInt32,
                            AudioServerPlugInIOOperationID op, Boolean *out,
                            Boolean *outSync) -> OSStatus {
      *out = (op == kAudioServerPlugInIOOperationWriteMix);
      *outSync = false;
      return kAudioHardwareNoError;
    },

    .BeginIOOperation = [](AudioServerPlugInDriverRef, AudioObjectID, UInt32,
                           AudioServerPlugInIOOperationID, UInt32,
                           const AudioServerPlugInIOCycleInfo *) -> OSStatus {
      return kAudioHardwareNoError;
    },

    .DoIOOperation =
        [](AudioServerPlugInDriverRef driverRef, AudioObjectID deviceObjectID,
           AudioObjectID streamObjectID, UInt32 clientID,
           AudioServerPlugInIOOperationID operationID, UInt32 ioBufferFrameSize,
           const AudioServerPlugInIOCycleInfo *ioCycleInfo, void *ioMainBuffer,
           void *ioSecondaryBuffer) -> OSStatus {
      // Phase 4b: copy from ring buffer to ioMainBuffer for
      // kAudioServerPlugInIOOperationWriteMix
      if (operationID == kAudioServerPlugInIOOperationWriteMix &&
          ioMainBuffer) {
        // Zero output until real routing is implemented
        memset(ioMainBuffer, 0, ioBufferFrameSize * kBytesPerFrame);
      }
      return kAudioHardwareNoError;
    },

    .EndIOOperation = [](AudioServerPlugInDriverRef, AudioObjectID, UInt32,
                         AudioServerPlugInIOOperationID, UInt32,
                         const AudioServerPlugInIOCycleInfo *) -> OSStatus {
      return kAudioHardwareNoError;
    },
};

// ---------------------------------------------------------------------------
// Plugin factory — called by CoreAudio HAL when loading the plugin bundle
// ---------------------------------------------------------------------------
extern "C" void *VirtuosoPlugIn_Create(CFAllocatorRef, CFUUIDRef factoryID) {
  gLog = os_log_create("audio.virtuoso.app", "plugin");
  os_log(gLog, "VirtuosoPlugIn: Create");
  return &gAudioServerPlugInDriverInterface;
}

static OSStatus VirtuosoPlugIn_CreateDevice(AudioServerPlugInHostRef host,
                                            CFDictionaryRef,
                                            const AudioServerPlugInClientInfo *,
                                            AudioObjectID *outDeviceObjectID) {
  if (!outDeviceObjectID)
    return kAudioHardwareBadObjectError;
  *outDeviceObjectID = kObjectID_Device;

  // Phase 4b: call host->PropertiesChanged to register kObjectID_Device
  // with the HAL so it appears in System Preferences
  os_log(gLog, "VirtuosoPlugIn: CreateDevice (objectID=%u)", kObjectID_Device);
  return kAudioHardwareNoError;
}
