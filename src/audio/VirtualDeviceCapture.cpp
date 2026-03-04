// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// VirtualDeviceCapture.cpp — Factory implementation

#include "VirtualDeviceCapture.h"

#if defined(_WIN32)
#include "platform/WasapiCapture.h"
#elif defined(__APPLE__)
#include "platform/CoreAudioCapture.h"
#else
#include "platform/PipeWireCapture.h"
#endif

namespace virtuoso {

std::unique_ptr<VirtualDeviceCapture> VirtualDeviceCapture::create() {
#if defined(_WIN32)
  return std::make_unique<WasapiCapture>();
#elif defined(__APPLE__)
  return std::make_unique<CoreAudioCapture>();
#else
  return std::make_unique<PipeWireCapture>();
#endif
}

} // namespace virtuoso
