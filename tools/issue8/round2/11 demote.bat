@echo off
rem Issue #8 test: 11 demote
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_KIL=2
start "" "%~dp0lasrecomp.exe"
