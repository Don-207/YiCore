@echo off
setlocal

python "%~dp0scripts\yi_create_board.py" %*
exit /b %ERRORLEVEL%
