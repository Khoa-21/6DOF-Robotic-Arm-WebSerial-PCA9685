@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ========================================================
echo       TOOL ĐẨY DỰ ÁN CÁNH TAY ROBOT 6-DOF LÊN GITHUB
echo ========================================================
echo.
echo Thu muc hien tai: %CD%
echo.
set /p REPO_URL="Dán link GitHub Repository của bạn vào đây (ví dụ: https://github.com/.../....git): "

if "%REPO_URL%"=="" (
    echo Bạn chưa nhập link! Vui lòng chạy lại file.
    pause
    exit /b
)

git branch -M main
git remote remove origin >nul 2>&1
git remote add origin %REPO_URL%
echo.
echo Đang đẩy code và hình ảnh lên GitHub...
git push -u origin main

echo.
echo ========================================================
echo Hoàn tất! Hãy mở link GitHub trên trình duyệt để kiểm tra nhé.
echo ========================================================
pause
