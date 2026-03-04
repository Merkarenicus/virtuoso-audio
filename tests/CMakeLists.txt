cmake_minimum_required(VERSION 3.24)

# tests/CMakeLists.txt
# All test targets: unit, integration, performance, fuzz.

# ---------------------------------------------------------------------------
# Helper to create a test executable linked against project internals + Catch2
# ---------------------------------------------------------------------------
function(virtuoso_add_test target_name)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})
    add_executable(${target_name} ${ARG_SOURCES})
    target_link_libraries(${target_name} PRIVATE
        VirtuosoAudio          # Main project target (headers + object files)
        Catch2::Catch2WithMain
    )
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src   # Access all internal headers
    )
    catch_discover_tests(${target_name})
endfunction()

# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------
virtuoso_add_test(ConvolverProcessorTest
    SOURCES unit/ConvolverProcessorTest.cpp)

virtuoso_add_test(UpmixProcessorTest
    SOURCES unit/UpmixProcessorTest.cpp)

virtuoso_add_test(ChannelLayoutMapperTest
    SOURCES unit/ChannelLayoutMapperTest.cpp)

virtuoso_add_test(BassManagementTest
    SOURCES unit/BassManagementTest.cpp)

virtuoso_add_test(WavHrirParserTest
    SOURCES unit/WavHrirParserTest.cpp)

virtuoso_add_test(HrirValidatorTest
    SOURCES unit/HrirValidatorTest.cpp)

virtuoso_add_test(TransactionJournalTest
    SOURCES unit/TransactionJournalTest.cpp)

virtuoso_add_test(AudioCommandQueueTest
    SOURCES unit/AudioCommandQueueTest.cpp)

virtuoso_add_test(CryptoBackendTest
    SOURCES unit/CryptoBackendTest.cpp)

# ---------------------------------------------------------------------------
# Integration tests
# ---------------------------------------------------------------------------
virtuoso_add_test(OfflineParityTest
    SOURCES integration/OfflineParityTest.cpp)

virtuoso_add_test(ParserSubprocessTest
    SOURCES integration/ParserSubprocessTest.cpp)

virtuoso_add_test(HeSuViImportTest
    SOURCES integration/HeSuViImportTest.cpp)

# ---------------------------------------------------------------------------
# Performance tests (only in Release; skipped if ASAN is on)
# ---------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT VIRTUOSO_ENABLE_ASAN)
    virtuoso_add_test(ConvolverBench
        SOURCES perf/ConvolverBenchmark.cpp)

    virtuoso_add_test(LatencyHarness
        SOURCES perf/LatencyHarness.cpp)
endif()

# ---------------------------------------------------------------------------
# Fuzz targets (libFuzzer; only on Clang with -fsanitize=fuzzer)
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    foreach(FUZZ_TARGET FuzzWavParser FuzzSofaParser FuzzJsonProfile)
        add_executable(${FUZZ_TARGET} fuzz/${FUZZ_TARGET}.cpp)
        target_link_libraries(${FUZZ_TARGET} PRIVATE VirtuosoAudio)
        target_include_directories(${FUZZ_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
        target_compile_options(${FUZZ_TARGET} PRIVATE -fsanitize=fuzzer,address)
        target_link_options(${FUZZ_TARGET} PRIVATE -fsanitize=fuzzer,address)
    endforeach()
endif()
