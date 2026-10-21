@echo off
rem Issue #8 test (round 2): 5 no compression
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_VK_LEAN_USAGE=0
start "" "%~dp0lasrecomp.exe"
