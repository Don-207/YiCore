@echo off
rem File: create-app.cmd
rem Function: Create a thin board-independent YiCore application.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_create_app.py" %*
exit /b %ERRORLEVEL%
