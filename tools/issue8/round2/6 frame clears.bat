@echo off
rem Issue #8 test (round 2): 6 frame clears
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_NO_FRAME_CLEAR=0
start "" "%~dp0lasrecomp.exe"
