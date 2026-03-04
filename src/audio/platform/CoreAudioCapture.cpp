// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// CoreAudioCapture.cpp

#include "CoreAudioCapture.h"

#if defined(__APPLE__)
namespace virtuoso {

bool CoreAudioCapture::start(const std::string &deviceName,
                             CaptureCallback callback) {
  if (m_running.load())
    return true;
  m_callback = std::move(callback);

  m_deviceID = findVirtuosoDevice(deviceName);
  if (m_deviceID == kAudioObjectUnknown) {
    // Fall back to default output device
    AudioObjectPropertyAddress pa{kAudioHardwarePropertyDefaultOutputDevice,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain};
    UInt32 sz = sizeof(AudioDeviceID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &pa, 0, nullptr, &sz,
                               &m_deviceID);
  }

  // Get sample rate
  AudioObjectPropertyAddress rateAddr{kAudioDevicePropertyNominalSampleRate,
                                      kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
  Float64 rate = 48000.0;
  UInt32 sz = sizeof(Float64);
  AudioObjectGetPropertyData(m_deviceID, &rateAddr, 0, nullptr, &sz, &rate);
  m_sampleRate = rate;

  // Register IO proc
  OSStatus status =
      AudioDeviceCreateIOProcID(m_deviceID, audioIOProc, this, &m_ioProcID);
  if (status != noErr)
    return false;

  status = AudioDeviceStart(m_deviceID, m_ioProcID);
  if (status != noErr) {
    AudioDeviceDestroyIOProcID(m_deviceID, m_ioProcID);
    return false;
  }

  m_running.store(true, std::memory_order_release);
  return true;
}

void CoreAudioCapture::stop() {
  if (!m_running.load())
    return;
  m_running.store(false, std::memory_order_release);
  if (m_deviceID != kAudioObjectUnknown && m_ioProcID) {
    AudioDeviceStop(m_deviceID, m_ioProcID);
    AudioDeviceDestroyIOProcID(m_deviceID, m_ioProcID);
    m_ioProcID = nullptr;
  }
}

OSStatus CoreAudioCapture::audioIOProc(AudioDeviceID, const AudioTimeStamp *,
                                       const AudioBufferList *inData,
                                       const AudioTimeStamp *,
                                       AudioBufferList *,
                                       const AudioTimeStamp *, void *refCon) {
  auto *self = static_cast<CoreAudioCapture *>(refCon);
  if (!self->m_running.load() || !inData || !self->m_callback)
    return noErr;

  // Find channel count and convert non-interleaved float to juce::AudioBuffer
  const int numBuffers = static_cast<int>(inData->mNumberBuffers);
  int totalCh = 0;
  for (int b = 0; b < numBuffers; ++b)
    totalCh += static_cast<int>(inData->mBuffers[b].mNumberChannels);

  const int numFrames =
      (numBuffers > 0 && inData->mBuffers[0].mDataByteSize > 0)
          ? static_cast<int>(inData->mBuffers[0].mDataByteSize / sizeof(float) /
                             inData->mBuffers[0].mNumberChannels)
          : 0;

  if (numFrames == 0)
    return noErr;

  juce::AudioBuffer<float> buf(std::max(8, totalCh), numFrames);
  buf.clear();

  int outCh = 0;
  for (int b = 0; b < numBuffers; ++b) {
    const float *src = static_cast<const float *>(inData->mBuffers[b].mData);
    const int nCh = static_cast<int>(inData->mBuffers[b].mNumberChannels);
    for (int ch = 0; ch < nCh && outCh < 8; ++ch, ++outCh)
      for (int i = 0; i < numFrames; ++i)
        buf.getWritePointer(outCh)[i] = src[i * nCh + ch];
  }

  self->m_callback(buf, self->m_sampleRate);
  return noErr;
}

AudioDeviceID
CoreAudioCapture::findVirtuosoDevice(const std::string &name) const {
  if (name.empty())
    return kAudioObjectUnknown;

  AudioObjectPropertyAddress pa{kAudioHardwarePropertyDevices,
                                kAudioObjectPropertyScopeGlobal,
                                kAudioObjectPropertyElementMain};
  UInt32 sz = 0;
  AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &pa, 0, nullptr,
                                 &sz);
  const int count = sz / sizeof(AudioDeviceID);
  std::vector<AudioDeviceID> devices(count);
  AudioObjectGetPropertyData(kAudioObjectSystemObject, &pa, 0, nullptr, &sz,
                             devices.data());

  for (AudioDeviceID dev : devices) {
    AudioObjectPropertyAddress namePa{kAudioDevicePropertyDeviceName,
                                      kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
    char buf[256] = {};
    UInt32 bsz = sizeof(buf);
    AudioObjectGetPropertyData(dev, &namePa, 0, nullptr, &bsz, buf);
    if (std::string(buf).find(name) != std::string::npos)
      return dev;
  }
  return kAudioObjectUnknown;
}

} // namespace virtuoso
#endif // __APPLE__
