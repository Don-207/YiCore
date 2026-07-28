@echo off
rem File: create-test.cmd
rem Function: Add a board-test image to an existing YiCore product.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_create_product.py" test %*
exit /b %ERRORLEVEL%
