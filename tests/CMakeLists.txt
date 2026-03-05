# tests/CMakeLists.txt — Catch2 test targets for Virtuoso Audio
# Requires: -DVIRTUOSO_BUILD_TESTS=ON (set in parent CMakeLists.txt)

cmake_minimum_required(VERSION 3.24)

# ---------------------------------------------------------------------------
# Unit Tests
# ---------------------------------------------------------------------------
set(VIRTUOSO_UNIT_TESTS
    unit/CryptoBackendTest.cpp
    unit/WavHrirParserTest.cpp
    unit/TransactionJournalTest.cpp
    unit/UpmixProcessorTest.cpp    # Also covers ChannelLayoutMapper + BassManagement
)

add_executable(VirtuosoUnitTests ${VIRTUOSO_UNIT_TESTS})

target_link_libraries(VirtuosoUnitTests
    PRIVATE
        VirtuosoCore
        Catch2::Catch2WithMain
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_dsp
)

target_include_directories(VirtuosoUnitTests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_compile_options(VirtuosoUnitTests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -fsanitize=address,undefined>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX->
)
target_link_options(VirtuosoUnitTests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address,undefined>
)

include(CTest)
include(Catch)
catch_discover_tests(VirtuosoUnitTests
    TEST_SUFFIX ".unit"
    PROPERTIES TIMEOUT 60
)

# ---------------------------------------------------------------------------
# Integration Tests
# ---------------------------------------------------------------------------
add_executable(VirtuosoIntegrationTests
    integration/OfflineParityTest.cpp
    integration/ParserSubprocessTest.cpp
)

target_link_libraries(VirtuosoIntegrationTests
    PRIVATE
        VirtuosoCore
        Catch2::Catch2WithMain
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_dsp
)

target_include_directories(VirtuosoIntegrationTests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

catch_discover_tests(VirtuosoIntegrationTests
    TEST_SUFFIX ".integration"
    PROPERTIES TIMEOUT 120   # HRIR subprocess has 30s timeout per test
)

# ---------------------------------------------------------------------------
# Performance / benchmark tests — not run by default in CI (use [!benchmark] tag)
# ---------------------------------------------------------------------------
add_executable(VirtuosoPerfTests
    perf/ConvolverBenchmark.cpp
    perf/LatencyHarness.cpp
)

target_link_libraries(VirtuosoPerfTests
    PRIVATE
        VirtuosoCore
        Catch2::Catch2WithMain
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
)

target_include_directories(VirtuosoPerfTests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

catch_discover_tests(VirtuosoPerfTests
    TEST_SUFFIX ".perf"
    PROPERTIES TIMEOUT 300   # Latency harness runs 500 blocks
)

# ---------------------------------------------------------------------------
# Fuzz targets (only when -DVIRTUOSO_BUILD_FUZZ=ON with Clang/libFuzzer)
# ---------------------------------------------------------------------------
if(VIRTUOSO_BUILD_FUZZ AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    foreach(FUZZ_TARGET FuzzWavParser FuzzSofaParser)
        add_executable(${FUZZ_TARGET} fuzz/${FUZZ_TARGET}.cpp)
        target_link_libraries(${FUZZ_TARGET}
            PRIVATE VirtuosoCore juce::juce_core juce::juce_audio_formats)
        target_compile_options(${FUZZ_TARGET}
            PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
        target_link_options(${FUZZ_TARGET}
            PRIVATE -fsanitize=fuzzer,address)
        # Create corpus directory
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpus/${FUZZ_TARGET})
    endforeach()
endif()
