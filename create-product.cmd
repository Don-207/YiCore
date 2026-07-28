@echo off
rem File: create-product.cmd
rem Function: Create the legacy standalone product layout.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_create_product.py" app %*
exit /b %ERRORLEVEL%
