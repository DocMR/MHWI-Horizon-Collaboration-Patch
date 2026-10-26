@echo off
chcp 65001 >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Export-Feedback.ps1"
if errorlevel 1 echo Feedback export failed. Please include the error when contacting the author.
pause
