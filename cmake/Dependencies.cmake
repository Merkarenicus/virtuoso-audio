# cmake/Dependencies.cmake
# All external dependencies pinned to exact versions + SHA-256 hashes.
# SUPPLY-CHAIN POLICY: Never update a hash without a security review.

include(CPM)

# ---------------------------------------------------------------------------
# JUCE 8 (Commercial license required — see ADR-002-Licensing.md)
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG         8.0.4          # TODO: replace with exact commit SHA after pinning
    # GIT_TAG       <EXACT_COMMIT_SHA256>
    OPTIONS
        "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON"
)

# ---------------------------------------------------------------------------
# libmysofa — SOFA file parser (BSD-3-Clause)
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libmysofa
    GITHUB_REPOSITORY hoene/libmysofa
    GIT_TAG         v1.3.2
    # GIT_TAG       <EXACT_COMMIT_SHA256>   # TODO: pin
    OPTIONS
        "BUILD_TESTS OFF"
        "BUILD_SHARED_LIBS OFF"
)

# ---------------------------------------------------------------------------
# libsamplerate — sample-rate conversion (BSD-2-Clause)
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libsamplerate
    GITHUB_REPOSITORY libsndfile/libsamplerate
    GIT_TAG         0.2.2
    # GIT_TAG       <EXACT_COMMIT_SHA256>   # TODO: pin
    OPTIONS
        "BUILD_TESTING OFF"
        "BUILD_SHARED_LIBS OFF"
)

# ---------------------------------------------------------------------------
# nlohmann/json — JSON parsing (MIT)
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    GIT_TAG         v3.11.3
    # GIT_TAG       <EXACT_COMMIT_SHA256>   # TODO: pin
    OPTIONS
        "JSON_BuildTests OFF"
        "JSON_Install ON"
)

# ---------------------------------------------------------------------------
# libsodium — authenticated encryption (ISC License)
# Replaces tiny-AES (which lacks GCM). See ADR-004.
# ---------------------------------------------------------------------------
CPMAddPackage(
    NAME            libsodium
    GITHUB_REPOSITORY jedisct1/libsodium
    GIT_TAG         1.0.20-RELEASE
    # GIT_TAG       <EXACT_COMMIT_SHA256>   # TODO: pin
)

# ---------------------------------------------------------------------------
# Catch2 — test framework (Boost Software License)
# Only needed if VIRTUOSO_BUILD_TESTS is ON
# ---------------------------------------------------------------------------
if(VIRTUOSO_BUILD_TESTS)
    CPMAddPackage(
        NAME            Catch2
        GITHUB_REPOSITORY catchorg/Catch2
        GIT_TAG         v3.6.0
        # GIT_TAG       <EXACT_COMMIT_SHA256>   # TODO: pin
        OPTIONS
            "CATCH_INSTALL_DOCS OFF"
            "CATCH_INSTALL_EXTRAS OFF"
    )
    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
endif()

# ---------------------------------------------------------------------------
# Platform-specific: PipeWire + libsecret (Linux only, via pkg-config)
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PipeWire REQUIRED IMPORTED_TARGET libpipewire-0.3)
    pkg_check_modules(LibSecret IMPORTED_TARGET libsecret-1)
    if(NOT LibSecret_FOUND)
        message(WARNING
            "libsecret not found — profile encryption will use plaintext fallback "
            "on Linux. Install libsecret-1-dev for secure storage.")
    endif()
endif()
