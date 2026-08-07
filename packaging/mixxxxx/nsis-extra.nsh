; Mixxxxx NSIS hooks — fleet OSC launcher shortcuts + mixxxxx-osc.cmd

!macro customInit
  DetailPrint "Stopping running Mixxx / Mixxxxx instances..."
  nsExec::ExecToLog 'taskkill /F /IM mixxx.exe /T'
  Sleep 2000
!macroend

!macro customInstall
  DetailPrint "Mixxxxx installed — creating OSC fleet shortcuts..."
  ; mixxxxx-osc.cmd is installed via CMake (packaging/mixxxxx/mixxxxx-osc.cmd)
  CreateShortCut "$SMPROGRAMS\Mixxxxx\Mixxxxx (OSC fleet).lnk" "$INSTDIR\mixxxxx-osc.cmd" "" "$INSTDIR\mixxx.exe" 0
  CreateShortCut "$SMPROGRAMS\Mixxxxx\Mixxxxx.lnk" "$INSTDIR\mixxx.exe" "" "$INSTDIR\mixxx.exe" 0
  CreateShortCut "$DESKTOP\Mixxxxx (OSC fleet).lnk" "$INSTDIR\mixxxxx-osc.cmd" "" "$INSTDIR\mixxx.exe" 0
  DetailPrint "OSC defaults: listen 11119, feedback to 127.0.0.1:11118"
!macroend

!macro customUnInstall
  Delete "$DESKTOP\Mixxxxx (OSC fleet).lnk"
  DetailPrint "Removing Mixxxxx..."
!macroend
