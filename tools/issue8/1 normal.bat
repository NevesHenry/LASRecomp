@echo off
rem Issue #8 test: 1 normal
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
start "" "%~dp0lasrecomp.exe"
