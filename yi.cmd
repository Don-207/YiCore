@echo off
rem File: yi.cmd
rem Function: Launch the unified YiCore command-line interface.
rem Author: Don
rem Date: 2026-07-28
rem Version: 1.0.0
setlocal

python "%~dp0scripts\yi_cli.py" %*
exit /b %ERRORLEVEL%
