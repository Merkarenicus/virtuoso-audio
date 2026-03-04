// SPDX-License-Identifier: MIT
#include "SingleInstance.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#endif

namespace virtuoso {

SingleInstance::SingleInstance() {
#if defined(_WIN32)
  m_mutex = CreateMutexW(nullptr, TRUE, L"VirtuosoAudio_SingleInstance");
  m_owner = (m_mutex != nullptr) && (GetLastError() != ERROR_ALREADY_EXISTS);
#else
  m_lockFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("virtuoso-audio.lock");
  m_lockFd =
      open(m_lockFile.getFullPathName().toRawUTF8(), O_CREAT | O_RDWR, 0600);
  m_owner = (m_lockFd >= 0 && flock(m_lockFd, LOCK_EX | LOCK_NB) == 0);
#endif
}

SingleInstance::~SingleInstance() {
#if defined(_WIN32)
  if (m_mutex) {
    ReleaseMutex(m_mutex);
    CloseHandle(m_mutex);
  }
#else
  if (m_lockFd >= 0) {
    flock(m_lockFd, LOCK_UN);
    close(m_lockFd);
    m_lockFile.deleteFile();
  }
#endif
}

} // namespace virtuoso
