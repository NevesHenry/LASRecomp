@echo off
rem Issue #8 test: 9 nan by bits
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_NAN_BITS=1
start "" "%~dp0lasrecomp.exe"
