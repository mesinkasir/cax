@echo off
setlocal
echo.
echo CAX SSG - C STATIC SITE GENERATOR
echo BY AXCORA TECHNOLOGY
echo ------------------------------------------
echo.

if "%1"=="" (
    echo Usage:
    echo cax init [name] - Init new project
    echo cax build - Build site to /site
    echo cax start - Start server at :8080
    echo cax serve --watch - Dev mode + LiveReload
    echo.
    exit /b 0
)

cax.exe %*