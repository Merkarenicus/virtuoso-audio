; SPDX-License-Identifier: MIT
; Copyright (c) 2026 Virtuoso Audio Project Contributors
;
; installer/windows/VirtuosoSetup.nsi
; NSIS installer script for Virtuoso Audio on Windows.
; Installs: application binary, driver (.inf + .sys), assets, uninstaller.
; Requirements: NSIS 3.09+, NSIS AccessControl plugin, devcon.exe in PATH or bundled

Unicode True

!define APPNAME    "Virtuoso Audio"
!define APPVERSION "1.0.0"
!define PUBLISHER  "Virtuoso Audio Project"
!define HELPURL    "https://github.com/Merkarenicus/virtuoso-audio"
!define APPEXE     "VirtuosoAudio.exe"
!define DRVNAME    "VirtuosoVAD"
!define REGKEY     "Software\${PUBLISHER}\${APPNAME}"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name           "${APPNAME} ${APPVERSION}"
OutFile        "VirtuosoSetup-${APPVERSION}-win64.exe"
InstallDir     "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "${REGKEY}" "InstallPath"

RequestExecutionLevel admin   ; UAC elevation required for driver install

; Compression
SetCompressor  /SOLID lzma
SetCompressorDictSize 32

; ---------------------------------------------------------------------------
; Pages
; ---------------------------------------------------------------------------
Page license
Page directory
Page components
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

; ---------------------------------------------------------------------------
; Language
; ---------------------------------------------------------------------------
!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------------------
; Sections
; ---------------------------------------------------------------------------
Section "Virtuoso Audio Application" SEC_APP
  SectionIn RO  ; Required

  SetOutPath "$INSTDIR"
  File "..\..\build\vs2022-x64\Release\Standalone\VirtuosoAudio_artefacts\Release\Standalone\${APPEXE}"

  ; Assets
  File /r "..\..\assets\hrir"
  File /r "..\..\assets\fonts"
  File    "..\..\assets\legal\LICENSE"
  File    "..\..\assets\legal\THIRD_PARTY_LICENSES.md"
  File    "..\..\assets\legal\PRIVACY_POLICY.md"

  ; Write registry entries
  WriteRegStr HKLM "${REGKEY}" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "${REGKEY}" "Version"     "${APPVERSION}"

  ; Create Start Menu shortcut
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortcut  "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\${APPEXE}"
  CreateShortcut  "$DESKTOP\${APPNAME}.lnk"               "$INSTDIR\${APPEXE}"

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr  HKLM "${UNINST_KEY}" "DisplayName"     "${APPNAME}"
  WriteRegStr  HKLM "${UNINST_KEY}" "DisplayVersion"  "${APPVERSION}"
  WriteRegStr  HKLM "${UNINST_KEY}" "Publisher"       "${PUBLISHER}"
  WriteRegStr  HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr  HKLM "${UNINST_KEY}" "URLInfoAbout"    "${HELPURL}"
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"       1
  WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"       1
SectionEnd

Section "Virtuoso Virtual Audio Driver (required)" SEC_DRIVER
  SectionIn RO  ; Required

  SetOutPath "$INSTDIR\driver"
  File "..\..\build\vs2022-x64\Release\driver\VirtuosoVAD.sys"
  File "..\..\drivers\windows\VirtuosoVAD.inf"

  ; Install the driver via pnputil (preferred on Win10/11, no devcon needed)
  DetailPrint "Installing Virtuoso virtual audio driver..."
  nsExec::ExecToLog /TIMEOUT=30000 \
    'pnputil /add-driver "$INSTDIR\driver\VirtuosoVAD.inf" /install'
  Pop $0
  ${If} $0 != 0
    MessageBox MB_ICONEXCLAMATION "Driver installation failed (error $0). \
      Audio virtualisation will not work until the driver is reinstalled."
  ${EndIf}
SectionEnd

Section "Launch on Windows startup" SEC_STARTUP
  WriteRegStr HKCU \
    "Software\Microsoft\Windows\CurrentVersion\Run" \
    "${APPNAME}" '"$INSTDIR\${APPEXE}" --minimized'
SectionEnd

; ---------------------------------------------------------------------------
; Uninstaller
; ---------------------------------------------------------------------------
Section "Uninstall"
  ; Remove driver first
  nsExec::ExecToLog /TIMEOUT=30000 \
    'pnputil /delete-driver "$INSTDIR\driver\VirtuosoVAD.inf" /uninstall /force'

  ; Remove files
  RMDir /r "$INSTDIR"

  ; Remove shortcuts
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  RMDir  "$SMPROGRAMS\${APPNAME}"
  Delete "$DESKTOP\${APPNAME}.lnk"

  ; Remove registry
  DeleteRegKey HKLM "${REGKEY}"
  DeleteRegKey HKLM "${UNINST_KEY}"
  DeleteRegValue HKCU \
    "Software\Microsoft\Windows\CurrentVersion\Run" "${APPNAME}"
SectionEnd

; ---------------------------------------------------------------------------
; Descriptions (hover text for component selection)
; ---------------------------------------------------------------------------
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP}     "Virtuoso Audio application and binaural rendering engine."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DRIVER}  "Virtual 7.1 audio device driver — required for system audio routing."
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTUP} "Start Virtuoso automatically with Windows (system tray icon)."
!insertmacro MUI_FUNCTION_DESCRIPTION_END
