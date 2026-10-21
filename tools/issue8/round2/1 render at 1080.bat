@echo off
rem Issue #8 test (round 2): 1 render at 1080
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_SCALE=1
start "" "%~dp0lasrecomp.exe"
