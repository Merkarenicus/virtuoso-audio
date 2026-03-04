# cmake/CodeSigning.cmake
# EV code-signing helpers for Windows driver (.sys) and installer (.exe).
#
# REQUIREMENTS:
#   Windows driver (.sys):
#     - EV Code Signing Certificate (DigiCert, Sectigo, etc.) (~$400–800/yr)
#     - Microsoft Partner Center account for attestation signing
#     - signtool.exe from Windows SDK
#
#   macOS:
#     - Apple Developer ID Application certificate ($99/yr)
#     - Notarization via xcrun notarytool
#
# Usage:
#   virtuoso_sign_binary(TARGET <target> FILE <path>)

function(virtuoso_sign_binary)
    cmake_parse_arguments(ARG "" "TARGET;FILE" "" ${ARGN})

    if(WIN32)
        # ----------------------------------------------------------------
        # Windows: signtool with EV certificate
        # Set VIRTUOSO_WIN_SIGN_CERT_THUMBPRINT in your CI environment.
        # ----------------------------------------------------------------
        find_program(SIGNTOOL signtool
            HINTS
                "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
                "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
        )
        if(SIGNTOOL AND DEFINED ENV{VIRTUOSO_WIN_SIGN_CERT_THUMBPRINT})
            add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
                COMMAND ${SIGNTOOL} sign
                    /sha1 "$ENV{VIRTUOSO_WIN_SIGN_CERT_THUMBPRINT}"
                    /tr http://timestamp.digicert.com
                    /td SHA256
                    /fd SHA256
                    "${ARG_FILE}"
                COMMENT "EV code-signing ${ARG_FILE}"
            )
        else()
            message(WARNING
                "[CodeSigning] Windows signing skipped. "
                "Set VIRTUOSO_WIN_SIGN_CERT_THUMBPRINT env var and install Windows SDK.")
        endif()

    elseif(APPLE)
        # ----------------------------------------------------------------
        # macOS: codesign + notarytool
        # Set VIRTUOSO_APPLE_SIGN_IDENTITY in your CI environment.
        # ----------------------------------------------------------------
        find_program(CODESIGN codesign)
        if(CODESIGN AND DEFINED ENV{VIRTUOSO_APPLE_SIGN_IDENTITY})
            add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
                COMMAND ${CODESIGN}
                    --deep --force --options runtime
                    --sign "$ENV{VIRTUOSO_APPLE_SIGN_IDENTITY}"
                    "${ARG_FILE}"
                COMMENT "codesign ${ARG_FILE}"
            )
        else()
            message(WARNING
                "[CodeSigning] macOS signing skipped. "
                "Set VIRTUOSO_APPLE_SIGN_IDENTITY env var.")
        endif()
    endif()
endfunction()
