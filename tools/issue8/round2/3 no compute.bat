@echo off
rem Issue #8 test (round 2): 3 no compute
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_SKIP_CS=all
start "" "%~dp0lasrecomp.exe"
