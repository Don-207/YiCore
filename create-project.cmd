@echo off
setlocal

python "%~dp0scripts\yi_create_project.py" %*
exit /b %ERRORLEVEL%
