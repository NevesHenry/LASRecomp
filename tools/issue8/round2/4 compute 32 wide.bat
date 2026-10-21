@echo off
rem Issue #8 test (round 2): 4 compute 32 wide
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_VK_COMPUTE_SG32=1
start "" "%~dp0lasrecomp.exe"
