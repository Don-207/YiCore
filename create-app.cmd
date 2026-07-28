@echo off
rem File: create-app.cmd
rem Function: Launch the YiCore product application creator.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_create_product.py" app %*
exit /b %ERRORLEVEL%
