@echo off
rem Issue #8 test: 7 discard after loop
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_KIL=1
start "" "%~dp0lasrecomp.exe"
