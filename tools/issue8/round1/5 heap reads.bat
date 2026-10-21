@echo off
rem Issue #8 test: 5 heap reads
cd /d "%~dp0"
set LARECOMP_VK_FLOAT_PROBE=1
set LARECOMP_VK_GMEM_FS_MISS=1
start "" "%~dp0lasrecomp.exe"
