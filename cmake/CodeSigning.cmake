# CodeSigning.cmake — Platform-specific code signing helpers for Virtuoso Audio
# Applied post-build in Release/RelWithDebInfo configurations.
# Usage: include(cmake/CodeSigning.cmake) in root CMakeLists.txt after targets defined.

option(VIRTUOSO_CODESIGN "Enable post-build code signing" OFF)
option(VIRTUOSO_NOTARIZE  "Enable macOS notarization (requires --apple-id, --team-id)" OFF)

if(NOT VIRTUOSO_CODESIGN)
    message(STATUS "[CodeSigning] Code signing disabled. Pass -DVIRTUOSO_CODESIGN=ON to enable.")
    return()
endif()

# ---------------------------------------------------------------------------
# Windows — signtool with EV certificate
# ---------------------------------------------------------------------------
if(WIN32)
    find_program(SIGNTOOL signtool
        PATHS
            "C:/Program Files (x86)/Windows Kits/10/bin/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
        REQUIRED
    )

    set(VIRTUOSO_WIN_CERT_SUBJECT "" CACHE STRING
        "Windows EV certificate subject name (e.g. 'Virtuoso Audio Project')")
    set(VIRTUOSO_TIMESTAMP_URL "http://timestamp.digicert.com"
        CACHE STRING "RFC 3161 timestamp server URL")

    if(VIRTUOSO_WIN_CERT_SUBJECT STREQUAL "")
        message(FATAL_ERROR "[CodeSigning] Set -DVIRTUOSO_WIN_CERT_SUBJECT='CN of your EV cert'")
    endif()

    function(virtuoso_sign_target TARGET)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND "${SIGNTOOL}" sign
                /v
                /fd SHA256
                /n "${VIRTUOSO_WIN_CERT_SUBJECT}"
                /tr "${VIRTUOSO_TIMESTAMP_URL}"
                /td SHA256
                "$<TARGET_FILE:${TARGET}>"
            COMMENT "Signing ${TARGET} with EV certificate"
            VERBATIM
        )
    endfunction()

# ---------------------------------------------------------------------------
# macOS — codesign + optional notarytool
# ---------------------------------------------------------------------------
elseif(APPLE)
    find_program(CODESIGN codesign REQUIRED)
    find_program(XCRUN    xcrun    REQUIRED)

    set(VIRTUOSO_MAC_CERT "Developer ID Application: Virtuoso Audio Project"
        CACHE STRING "macOS Developer ID Application certificate name")
    set(VIRTUOSO_APPLE_ID "" CACHE STRING "Apple ID for notarization")
    set(VIRTUOSO_TEAM_ID  "" CACHE STRING "Apple Team ID for notarization")
    set(VIRTUOSO_KEYCHAIN_PROFILE "virtuoso-notarytool-password"
        CACHE STRING "Keychain profile for notarytool credentials")

    function(virtuoso_sign_target TARGET)
        get_target_property(_type ${TARGET} TYPE)
        set(_entitlements "${CMAKE_SOURCE_DIR}/installer/macos/VirtuosoAudio.entitlements")

        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND "${CODESIGN}"
                --sign "${VIRTUOSO_MAC_CERT}"
                --options runtime
                --entitlements "${_entitlements}"
                --timestamp
                --deep
                --force
                "$<TARGET_BUNDLE_DIR:${TARGET}>"
            COMMENT "Signing ${TARGET} for macOS distribution"
            VERBATIM
        )

        if(VIRTUOSO_NOTARIZE AND VIRTUOSO_APPLE_ID AND VIRTUOSO_TEAM_ID)
            add_custom_command(TARGET ${TARGET} POST_BUILD APPEND
                COMMAND ${CMAKE_COMMAND} -E echo "Notarizing ${TARGET}..."
                COMMAND "${XCRUN}" notarytool submit
                    "$<TARGET_BUNDLE_DIR:${TARGET}>"
                    --apple-id  "${VIRTUOSO_APPLE_ID}"
                    --team-id   "${VIRTUOSO_TEAM_ID}"
                    --keychain-profile "${VIRTUOSO_KEYCHAIN_PROFILE}"
                    --wait
                COMMAND "${XCRUN}" stapler staple "$<TARGET_BUNDLE_DIR:${TARGET}>"
                COMMENT "Notarizing and stapling ${TARGET}"
                VERBATIM
            )
        endif()
    endfunction()

# ---------------------------------------------------------------------------
# Linux — GPG detached signature (.asc)
# ---------------------------------------------------------------------------
elseif(UNIX)
    find_program(GPG gpg QUIET)
    set(VIRTUOSO_GPG_KEY_ID "" CACHE STRING "GPG key fingerprint for Linux package signing")

    function(virtuoso_sign_target TARGET)
        if(GPG AND NOT VIRTUOSO_GPG_KEY_ID STREQUAL "")
            add_custom_command(TARGET ${TARGET} POST_BUILD
                COMMAND "${GPG}"
                    --batch --yes
                    --local-user "${VIRTUOSO_GPG_KEY_ID}"
                    --detach-sign --armor
                    "$<TARGET_FILE:${TARGET}>"
                COMMENT "GPG-signing ${TARGET}"
                VERBATIM
            )
        else()
            message(STATUS "[CodeSigning] GPG or key not set — Linux signing skipped")
        endif()
    endfunction()
endif()
