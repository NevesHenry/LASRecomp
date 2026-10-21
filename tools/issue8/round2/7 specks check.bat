@echo off
rem Issue #8 test (round 2): 7 specks check
cd /d "%~dp0"
set LARECOMP_VK_DIAG_LOG=1
set LARECOMP_VK_DEPTH_SCALED_COPY=0
set LARECOMP_VK_EXACT_H2F=1
set LARECOMP_VK_KIL=2
set LARECOMP_VK_SPIRV15=1
set LARECOMP_VK_NAN_BITS=1
set LARECOMP_VK_EQUAL_TOL=1
set LARECOMP_VK_QUAD=1
start "" "%~dp0lasrecomp.exe"
