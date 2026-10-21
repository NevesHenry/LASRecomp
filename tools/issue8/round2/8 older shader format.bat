@echo off
rem Issue #8 test: 8 older shader format
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_SPIRV15=1
start "" "%~dp0lasrecomp.exe"
