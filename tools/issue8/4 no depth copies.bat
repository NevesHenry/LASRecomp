@echo off
rem Issue #8 test: 4 no depth copies
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_DEPTH_SCALED_COPY=0
start "" "%~dp0lasrecomp.exe"
