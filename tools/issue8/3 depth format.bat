@echo off
rem Issue #8 test: 3 depth format
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_D24=2
start "" "%~dp0lasrecomp.exe"
