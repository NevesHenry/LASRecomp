@echo off
rem Issue #8 test: 10 depth tolerance
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_EQUAL_TOL=1
start "" "%~dp0lasrecomp.exe"
