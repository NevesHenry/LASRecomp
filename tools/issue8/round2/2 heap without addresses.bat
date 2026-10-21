@echo off
rem Issue #8 test (round 2): 2 heap without addresses
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_VK_GMEM_LAZY=0
start "" "%~dp0lasrecomp.exe"
