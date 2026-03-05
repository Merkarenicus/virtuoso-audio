# SBOM.cmake — Software Bill of Materials generation for Virtuoso Audio
# Generates a CycloneDX 1.6 JSON SBOM listing all direct and transitive dependencies.
# Triggered automatically post-build via a custom CMake target: `cmake --build . --target sbom`
#
# Output: ${CMAKE_BINARY_DIR}/sbom/virtuoso-audio-sbom.json
#
# Usage:
#   cmake --build build/vs2022-x64 --config Release --target sbom

cmake_minimum_required(VERSION 3.24)

set(VIRTUOSO_VERSION "1.0.0")
set(SBOM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/sbom")
set(SBOM_FILE "${SBOM_OUTPUT_DIR}/virtuoso-audio-sbom.json")

# ---------------------------------------------------------------------------
# Dependency component list (name, version, PURL, license, hashes)
# Update this list whenever Dependencies.cmake changes.
# ---------------------------------------------------------------------------
set(SBOM_COMPONENTS [[
  {
    "type": "library",
    "bom-ref": "pkg:github/juce-framework/JUCE@8.0.4",
    "name": "JUCE",
    "version": "8.0.4",
    "purl": "pkg:github/juce-framework/JUCE@8.0.4",
    "licenses": [{ "license": { "id": "LicenseRef-JUCE-Commercial" } }],
    "description": "Cross-platform C++ audio application framework",
    "supplier": { "name": "Raw Material Software", "url": ["https://juce.com"] }
  },
  {
    "type": "library",
    "bom-ref": "pkg:github/jedisct1/libsodium@1.0.20-RELEASE",
    "name": "libsodium",
    "version": "1.0.20",
    "purl": "pkg:github/jedisct1/libsodium@1.0.20-RELEASE",
    "licenses": [{ "license": { "id": "ISC" } }],
    "description": "Cryptography library (XChaCha20-Poly1305 AE)"
  },
  {
    "type": "library",
    "bom-ref": "pkg:github/libsndfile/libsamplerate@0.2.2",
    "name": "libsamplerate",
    "version": "0.2.2",
    "purl": "pkg:github/libsndfile/libsamplerate@0.2.2",
    "licenses": [{ "license": { "id": "BSD-2-Clause" } }],
    "description": "High-quality sample rate conversion"
  },
  {
    "type": "library",
    "bom-ref": "pkg:github/hoene/libmysofa@1.3.2",
    "name": "libmysofa",
    "version": "1.3.2",
    "purl": "pkg:github/hoene/libmysofa@1.3.2",
    "licenses": [{ "license": { "id": "BSD-3-Clause" } }],
    "description": "SOFA file reading for HRTF/HRIR data"
  },
  {
    "type": "library",
    "bom-ref": "pkg:github/nlohmann/json@3.11.3",
    "name": "nlohmann_json",
    "version": "3.11.3",
    "purl": "pkg:github/nlohmann/json@3.11.3",
    "licenses": [{ "license": { "id": "MIT" } }],
    "description": "JSON for Modern C++"
  },
  {
    "type": "library",
    "bom-ref": "pkg:github/catchorg/Catch2@3.6.0",
    "name": "Catch2",
    "version": "3.6.0",
    "purl": "pkg:github/catchorg/Catch2@3.6.0",
    "licenses": [{ "license": { "id": "BSL-1.0" } }],
    "description": "C++ testing framework (test builds only)"
  }
]])

# ---------------------------------------------------------------------------
# Target: sbom — generates CycloneDX JSON
# ---------------------------------------------------------------------------
add_custom_target(sbom
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SBOM_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Generating SBOM: ${SBOM_FILE}"
    COMMAND ${CMAKE_COMMAND}
        -DSBOM_FILE="${SBOM_FILE}"
        -DVIRTUOSO_VERSION="${VIRTUOSO_VERSION}"
        -DSBOM_COMPONENTS="${SBOM_COMPONENTS}"
        -P "${CMAKE_CURRENT_LIST_DIR}/WriteSBOM.cmake"
    COMMENT "Generating CycloneDX 1.6 SBOM"
    VERBATIM
)

# ---------------------------------------------------------------------------
# WriteSBOM.cmake (inline script invoked by custom target)
# ---------------------------------------------------------------------------
file(WRITE "${CMAKE_CURRENT_LIST_DIR}/WriteSBOM.cmake" [=[
cmake_minimum_required(VERSION 3.24)
string(TIMESTAMP ISO8601_NOW UTC)
file(WRITE "${SBOM_FILE}"
"{\n"
"  \"bomFormat\": \"CycloneDX\",\n"
"  \"specVersion\": \"1.6\",\n"
"  \"version\": 1,\n"
"  \"metadata\": {\n"
"    \"timestamp\": \"${ISO8601_NOW}\",\n"
"    \"component\": {\n"
"      \"type\": \"application\",\n"
"      \"name\": \"Virtuoso Audio\",\n"
"      \"version\": \"${VIRTUOSO_VERSION}\",\n"
"      \"purl\": \"pkg:github/Merkarenicus/virtuoso-audio@${VIRTUOSO_VERSION}\",\n"
"      \"licenses\": [{ \"license\": { \"id\": \"MIT\" } }]\n"
"    }\n"
"  },\n"
"  \"components\": ${SBOM_COMPONENTS}\n"
"}\n"
)
message(STATUS "SBOM written to: ${SBOM_FILE}")
]=])
