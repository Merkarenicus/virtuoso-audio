cmake_minimum_required(VERSION 3.24)
project(VirtuosoAudio VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# C++20 everywhere
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Build type
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(VIRTUOSO_BUILD_TESTS  "Build unit, integration, perf, and fuzz tests" ON)
option(VIRTUOSO_BUILD_VST3   "Build VST3 plugin target (Phase 5+)"           OFF)
option(VIRTUOSO_BUILD_AU     "Build AudioUnit target (Phase 5+, macOS only)" OFF)
option(VIRTUOSO_ENABLE_ASAN  "Enable AddressSanitizer"                        OFF)
option(VIRTUOSO_ENABLE_UBSAN "Enable UBSanitizer"                             OFF)

# ---------------------------------------------------------------------------
# CMake helpers
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)
include(cmake/CodeSigning.cmake)
include(cmake/SBOM.cmake)

# ---------------------------------------------------------------------------
# Sanitizer flags (development)
# ---------------------------------------------------------------------------
if(VIRTUOSO_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(VIRTUOSO_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

# ---------------------------------------------------------------------------
# Platform format list
# NOTE: VST3/AU are gated behind options and excluded from MVP builds.
# ---------------------------------------------------------------------------
set(VIRTUOSO_FORMATS Standalone)
if(VIRTUOSO_BUILD_VST3)
    list(APPEND VIRTUOSO_FORMATS VST3)
endif()
if(VIRTUOSO_BUILD_AU AND APPLE)
    list(APPEND VIRTUOSO_FORMATS AU)
endif()

# ---------------------------------------------------------------------------
# JUCE plugin / standalone target
# ---------------------------------------------------------------------------
juce_add_plugin(VirtuosoAudio
    COMPANY_NAME                    "Virtuoso Audio Project"
    COMPANY_WEBSITE                 "https://github.com/virtuoso-audio/virtuoso-audio"
    PLUGIN_MANUFACTURER_CODE        Virt
    PLUGIN_CODE                     Vaud
    FORMATS                         ${VIRTUOSO_FORMATS}
    PRODUCT_NAME                    "Virtuoso Audio"
    BUNDLE_ID                       "audio.virtuoso.app"
    PLUGIN_NAME                     "Virtuoso Audio"
    DESCRIPTION                     "7.1-to-binaural headphone virtualizer"
    IS_SYNTH                        FALSE
    NEEDS_MIDI_INPUT                FALSE
    NEEDS_MIDI_OUTPUT               FALSE
    IS_MIDI_EFFECT                  FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS     TRUE
    COPY_PLUGIN_AFTER_BUILD         FALSE
    VST3_CATEGORIES                 "Fx"
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(VirtuosoAudio
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0     # Requires JUCE commercial license
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_PLUGINHOST_VST3=0
    PRIVATE
        VIRTUOSO_VERSION_STRING="${PROJECT_VERSION}"
)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
target_sources(VirtuosoAudio
    PRIVATE
        # Entry & application shell
        src/Main.cpp
        src/StandaloneApp.h
        src/StandaloneApp.cpp

        # Audio engine
        src/audio/AudioEngine.h
        src/audio/AudioEngine.cpp
        src/audio/AudioCommandQueue.h
        src/audio/AudioCommandQueue.cpp
        src/audio/VirtualDeviceCapture.h
        src/audio/VirtualDeviceCapture.cpp
        src/audio/platform/WasapiCapture.h
        src/audio/platform/WasapiCapture.cpp
        src/audio/platform/CoreAudioCapture.h
        src/audio/platform/CoreAudioCapture.cpp
        src/audio/platform/PipeWireCapture.h
        src/audio/platform/PipeWireCapture.cpp
        src/audio/DeviceWatchdog.h
        src/audio/DeviceWatchdog.cpp

        # DSP processors
        src/audio/ConvolverProcessor.h
        src/audio/ConvolverProcessor.cpp
        src/audio/UpmixProcessor.h
        src/audio/UpmixProcessor.cpp
        src/audio/ChannelLayoutMapper.h
        src/audio/ChannelLayoutMapper.cpp
        src/audio/BassManagement.h
        src/audio/BassManagement.cpp
        src/audio/EqProcessor.h
        src/audio/EqProcessor.cpp
        src/audio/ResamplerProcessor.h
        src/audio/ResamplerProcessor.cpp
        src/audio/LimiterProcessor.h
        src/audio/LimiterProcessor.cpp

        # HRIR pipeline
        src/hrir/HrirManager.h
        src/hrir/HrirManager.cpp
        src/hrir/HrirParserWorker.h
        src/hrir/HrirParserWorker.cpp
        src/hrir/WavHrirParser.h
        src/hrir/WavHrirParser.cpp
        src/hrir/SofaHrirParser.h
        src/hrir/SofaHrirParser.cpp
        src/hrir/HrirValidator.h
        src/hrir/HrirValidator.cpp
        src/hrir/HrirResampler.h
        src/hrir/HrirResampler.cpp
        src/hrir/HeSuViImporter.h
        src/hrir/HeSuViImporter.cpp

        # Offline export
        src/export/OfflineRenderer.h
        src/export/OfflineRenderer.cpp
        src/export/ExportProgressModel.h
        src/export/ExportProgressModel.cpp

        # Profile management
        src/profile/ProfileManager.h
        src/profile/ProfileManager.cpp
        src/profile/ProfileSchema.h
        src/profile/ProfileEncryption.h
        src/profile/ProfileEncryption.cpp

        # Safety
        src/safety/TransactionJournal.h
        src/safety/TransactionJournal.cpp
        src/safety/DriverHealthCheck.h
        src/safety/DriverHealthCheck.cpp
        src/safety/RepairTool.h
        src/safety/RepairTool.cpp

        # Logging
        src/logging/Logger.h
        src/logging/Logger.cpp
        src/logging/SupportBundle.h
        src/logging/SupportBundle.cpp

        # Utilities
        src/util/PlatformDetect.h
        src/util/SingleInstance.h
        src/util/SingleInstance.cpp
        src/util/Crypto.h
        src/util/Crypto.cpp

        # GUI (Phase 3 — stubs for now)
        src/gui/MainWindow.h
        src/gui/MainWindow.cpp
        src/gui/LookAndFeel.h
        src/gui/LookAndFeel.cpp
        src/gui/components/MasterToggle.h
        src/gui/components/MasterToggle.cpp
        src/gui/components/HrirCarousel.h
        src/gui/components/HrirCarousel.cpp
        src/gui/components/LatencyMonitor.h
        src/gui/components/LatencyMonitor.cpp
        src/gui/components/AboutPanel.h
        src/gui/components/AboutPanel.cpp
        src/gui/SystemTray.h
        src/gui/SystemTray.cpp
)

# ---------------------------------------------------------------------------
# JUCE module / library links
# ---------------------------------------------------------------------------
target_link_libraries(VirtuosoAudio
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        # Third-party (fetched by Dependencies.cmake)
        mysofa
        samplerate
        nlohmann_json::nlohmann_json
        sodium
)

# ---------------------------------------------------------------------------
# Platform-specific OS libraries
# ---------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(VirtuosoAudio PRIVATE
        mmdevapi ole32 uuid winmm ksuser avrt)
elseif(APPLE)
    target_link_libraries(VirtuosoAudio PRIVATE
        "-framework CoreAudio"
        "-framework CoreFoundation"
        "-framework AudioToolbox"
        "-framework Security"  # for Keychain
    )
elseif(UNIX)
    # PipeWire and libsecret are found via pkg-config in Dependencies.cmake
    target_link_libraries(VirtuosoAudio PRIVATE
        PkgConfig::PipeWire
        PkgConfig::LibSecret)
endif()

# ---------------------------------------------------------------------------
# JUCE binary resources (icons, etc.)
# ---------------------------------------------------------------------------
juce_add_binary_data(VirtuosoAssets
    SOURCES
        assets/fonts/Inter-Variable.ttf
        assets/hrir/ASSET_MANIFEST.json
)
target_link_libraries(VirtuosoAudio PRIVATE VirtuosoAssets)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(VIRTUOSO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# SBOM generation (post-build)
# ---------------------------------------------------------------------------
virtuoso_generate_sbom(TARGET VirtuosoAudio)

# ---------------------------------------------------------------------------
# Install rules (used by CPack / packaging scripts)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS VirtuosoAudio
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_PREFIX}/Applications
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY assets/hrir/ DESTINATION ${CMAKE_INSTALL_DATADIR}/virtuoso-audio/hrir)
install(DIRECTORY assets/eq/   DESTINATION ${CMAKE_INSTALL_DATADIR}/virtuoso-audio/eq)
install(FILES assets/legal/LICENSE
              assets/legal/THIRD_PARTY_LICENSES.md
              assets/legal/PRIVACY_POLICY.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/virtuoso-audio/legal)
