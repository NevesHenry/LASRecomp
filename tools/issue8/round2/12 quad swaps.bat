@echo off
rem Issue #8 test: 12 quad swaps
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_QUAD=1
start "" "%~dp0lasrecomp.exe"
