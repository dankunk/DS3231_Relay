@echo off
REM Double-click to set the relay timer clock from this computer.
REM Options can be added when run from a terminal, e.g.  sync_rtc.bat --check
where py >NUL 2>NUL || (echo Python is not installed. Get it from https://www.python.org/downloads/ & pause & exit /b 1)
py -c "import serial" 2>NUL || py -m pip install --user pyserial
py "%~dp0sync_rtc.py" %*
echo.
pause
