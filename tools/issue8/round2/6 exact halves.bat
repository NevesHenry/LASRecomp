@echo off
rem Issue #8 test: 6 exact halves
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_EXACT_H2F=1
start "" "%~dp0lasrecomp.exe"
