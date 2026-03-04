# cmake/SBOM.cmake
# SPDX SBOM (Software Bill of Materials) generation.
# Generates a machine-readable SBOM after build for compliance and supply-chain auditing.
#
# Usage:
#   virtuoso_generate_sbom(TARGET <target>)

function(virtuoso_generate_sbom)
    cmake_parse_arguments(ARG "" "TARGET" "" ${ARGN})

    find_program(SBOM_TOOL
        NAMES syft sbom-tool
        DOC "SBOM generation tool (syft or Microsoft sbom-tool)"
    )

    set(SBOM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/sbom")
    set(SBOM_FILE "${SBOM_OUTPUT_DIR}/virtuoso-audio-${PROJECT_VERSION}.spdx.json")

    if(SBOM_TOOL)
        add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SBOM_OUTPUT_DIR}"
            COMMAND ${SBOM_TOOL} packages
                --source "${CMAKE_SOURCE_DIR}"
                --output "${SBOM_FILE}"
                --output-file-name "virtuoso-audio-${PROJECT_VERSION}.spdx.json"
            COMMENT "Generating SPDX SBOM → ${SBOM_FILE}"
            VERBATIM
        )
        install(FILES "${SBOM_FILE}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/virtuoso-audio/sbom"
            OPTIONAL
        )
    else()
        message(STATUS
            "[SBOM] No SBOM tool found (install 'syft' or 'sbom-tool'). "
            "SBOM generation skipped. Required for release builds.")
    endif()
endfunction()
