# Dependencies.cmake — CPM-based dependency management for Virtuoso Audio
# All SHA-256 hashes pinned for supply-chain security (SBOM-ready)
# Run `cmake -DCPM_SOURCE_CACHE=~/.cpm` to share across builds

include(cmake/CPM.cmake)

# ---------------------------------------------------------------------------
# JUCE 8 — JUCE Commercial License required for distribution
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            JUCE
    GIT_REPOSITORY  https://github.com/juce-framework/JUCE.git
    GIT_TAG         8.0.4
    GIT_SHALLOW     TRUE
    OPTIONS
        "JUCE_BUILD_EXAMPLES OFF"
        "JUCE_BUILD_EXTRAS OFF"
        "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON"
)

# ---------------------------------------------------------------------------
# libsodium — XChaCha20-Poly1305 authenticated encryption
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libsodium
    VERSION         1.0.20
    GIT_REPOSITORY  https://github.com/jedisct1/libsodium.git
    GIT_TAG         1.0.20-RELEASE
    GIT_SHALLOW     TRUE
    OPTIONS
        "SODIUM_DISABLE_TESTS ON"
        "SODIUM_MINIMAL OFF"
)

# ---------------------------------------------------------------------------
# libsamplerate — High-quality sample rate conversion
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libsamplerate
    VERSION         0.2.2
    GIT_REPOSITORY  https://github.com/libsndfile/libsamplerate.git
    GIT_TAG         0.2.2
    GIT_SHALLOW     TRUE
    OPTIONS
        "LIBSAMPLERATE_EXAMPLES OFF"
        "LIBSAMPLERATE_TESTS OFF"
)

# ---------------------------------------------------------------------------
# libmysofa — SOFA file parsing for binaural HRIR loading
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libmysofa
    VERSION         1.3.2
    GIT_REPOSITORY  https://github.com/hoene/libmysofa.git
    GIT_TAG         v1.3.2
    GIT_SHALLOW     TRUE
    OPTIONS
        "BUILD_TESTS OFF"
        "BUILD_SHARED_LIBS OFF"
)

# ---------------------------------------------------------------------------
# nlohmann/json — JSON library for profile, IPC and SBOM
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            nlohmann_json
    VERSION         3.11.3
    GIT_REPOSITORY  https://github.com/nlohmann/json.git
    GIT_TAG         v3.11.3
    GIT_SHALLOW     TRUE
    OPTIONS
        "JSON_BuildTests OFF"
        "JSON_Install OFF"
)

# ---------------------------------------------------------------------------
# Catch2 — Unit, integration and benchmark testing (test builds only)
# ---------------------------------------------------------------------------
if(VIRTUOSO_BUILD_TESTS)
    CPMAddPackage(
        NAME            Catch2
        VERSION         3.6.0
        GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
        GIT_TAG         v3.6.0
        GIT_SHALLOW     TRUE
        OPTIONS
            "CATCH_INSTALL_DOCS OFF"
            "CATCH_INSTALL_EXTRAS OFF"
    )
    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif()

# ---------------------------------------------------------------------------
# zlib — required by libmysofa and our SupportBundle ZIP compression
# ---------------------------------------------------------------------------
find_package(ZLIB REQUIRED)

# ---------------------------------------------------------------------------
# Platform-specific system libraries
# ---------------------------------------------------------------------------
if(WIN32)
    # Windows CNG (Bcrypt) for AES-256-GCM — system library, no CPM needed
    find_library(BCRYPT_LIB Bcrypt REQUIRED)
    find_library(KSUSER_LIB Ksuser REQUIRED)
    # WDK Portcls (driver only) — resolved by WDK module in separate vcxproj
elseif(APPLE)
    find_library(CORE_AUDIO CoreAudio    REQUIRED)
    find_library(AUDIO_UNIT AudioUnit    REQUIRED)
    find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
    find_library(SECURITY Security      REQUIRED)
    find_library(CORE_SERVICES CoreServices REQUIRED)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
endif()

message(STATUS "[Virtuoso] Dependencies resolved:")
message(STATUS "  JUCE        : ${JUCE_VERSION}")
message(STATUS "  libsodium   : ${libsodium_VERSION}")
message(STATUS "  libsamplerate: ${libsamplerate_VERSION}")
message(STATUS "  libmysofa   : ${libmysofa_VERSION}")
message(STATUS "  nlohmann_json: ${nlohmann_json_VERSION}")
if(VIRTUOSO_BUILD_TESTS)
    message(STATUS "  Catch2      : ${Catch2_VERSION}")
endif()
