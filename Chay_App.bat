@echo off
chcp 65001 >nul
title Bang Dieu Khien Robot

cd /d "%~dp0"

echo ===================================================
echo     DANG KHOI DONG BANG DIEU KHIEN ROBOT
echo ===================================================
echo.
echo Dang mo Google Chrome tai: http://localhost:8000
echo Luu y: Giu cua so nay de web hoat dong.
echo ===================================================
echo.

:: Mo Google Chrome chinh xac theo duong dan tren may
if exist "C:\Program Files\Google\Chrome\Application\chrome.exe" (
    start "" "C:\Program Files\Google\Chrome\Application\chrome.exe" "http://localhost:8000"
) else if exist "%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe" (
    start "" "%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe" "http://localhost:8000"
) else (
    start chrome "http://localhost:8000"
)

:: Chay may chu Web bang Python
python -m http.server 8000
pause
