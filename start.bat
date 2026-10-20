@echo off
echo =========================================
echo  SQL Query Compiler Studio
echo =========================================
echo.

:: Check Python
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python is not installed or not in PATH.
    echo Please install Python 3.8+ from https://python.org
    pause
    exit /b 1
)

:: Install dependencies
echo [1/2] Installing Python dependencies...
pip install -r requirements.txt --quiet
if errorlevel 1 (
    echo [ERROR] Failed to install dependencies.
    pause
    exit /b 1
)

:: Start server
echo [2/2] Starting Flask server...
echo.
echo  Open your browser at: http://localhost:5000
echo  Press Ctrl+C to stop the server.
echo.
python backend/app.py
pause
