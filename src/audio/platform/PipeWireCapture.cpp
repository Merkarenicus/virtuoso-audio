// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Virtuoso Audio Project Contributors
//
// PipeWireCapture.cpp — Linux PipeWire capture from Virtuoso virtual sink

#include "PipeWireCapture.h"

#if defined(__linux__)

namespace virtuoso {

PipeWireCapture::PipeWireCapture() { pw_init(nullptr, nullptr); }

bool PipeWireCapture::start(const std::string &deviceName,
                            CaptureCallback callback) {
  if (m_running.load())
    return true;
  m_callback = std::move(callback);

  m_pwThread = std::thread([this, deviceName] {
    m_loop = pw_main_loop_new(nullptr);
    if (!m_loop)
      return;

    // Build stream properties targeting the Virtuoso virtual sink
    auto *props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "DSP", PW_KEY_TARGET_OBJECT,
        deviceName.empty() ? "VirtuosoVirtual71" : deviceName.c_str(), nullptr);

    m_stream =
        pw_stream_new_simple(pw_main_loop_get_loop(m_loop), "virtuoso-capture",
                             props, &kStreamEvents, this);

    if (!m_stream) {
      pw_main_loop_destroy(m_loop);
      m_loop = nullptr;
      return;
    }

    uint8_t buffer[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    spa_audio_info_raw info = {
        .format = SPA_AUDIO_FORMAT_F32,
        .rate = static_cast<uint32_t>(m_sampleRate),
        .channels = static_cast<uint32_t>(m_numChannels),
    };
    const spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

    pw_stream_connect(m_stream, PW_DIRECTION_INPUT, PW_ID_ANY,
                      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                   PW_STREAM_FLAG_MAP_BUFFERS),
                      params, 1);

    m_running.store(true, std::memory_order_release);
    pw_main_loop_run(m_loop);

    pw_stream_destroy(m_stream);
    m_stream = nullptr;
    pw_main_loop_destroy(m_loop);
    m_loop = nullptr;
    m_running.store(false, std::memory_order_release);
  });

  // Give the thread a moment to initialise
  for (int i = 0; i < 20 && !m_running.load(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

  return m_running.load();
}

void PipeWireCapture::stop() {
  if (m_loop)
    pw_main_loop_quit(m_loop);
  if (m_pwThread.joinable())
    m_pwThread.join();
}

void PipeWireCapture::onProcess(void *userdata) {
  auto *self = static_cast<PipeWireCapture *>(userdata);
  pw_buffer *pwBuf = pw_stream_dequeue_buffer(self->m_stream);
  if (!pwBuf)
    return;

  spa_buffer *spaBuf = pwBuf->buffer;
  if (!spaBuf || !spaBuf->datas[0].data) {
    pw_stream_queue_buffer(self->m_stream, pwBuf);
    return;
  }

  const float *src = static_cast<const float *>(spaBuf->datas[0].data);
  const uint32_t nFrames =
      spaBuf->datas[0].chunk->size / sizeof(float) / self->m_numChannels;

  if (nFrames > 0 && self->m_callback) {
    juce::AudioBuffer<float> buf(self->m_numChannels,
                                 static_cast<int>(nFrames));
    for (uint32_t i = 0; i < nFrames; ++i)
      for (int ch = 0; ch < self->m_numChannels; ++ch)
        buf.getWritePointer(ch)[i] = src[i * self->m_numChannels + ch];
    self->m_callback(buf, self->m_sampleRate);
  }

  pw_stream_queue_buffer(self->m_stream, pwBuf);
}

const pw_stream_events PipeWireCapture::kStreamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = PipeWireCapture::onProcess,
};

} // namespace virtuoso
#endif // __linux__
