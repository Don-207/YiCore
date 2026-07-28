@echo off
rem File: create-boot.cmd
rem Function: Add a bootloader image to an existing YiCore product.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_create_product.py" boot %*
exit /b %ERRORLEVEL%
