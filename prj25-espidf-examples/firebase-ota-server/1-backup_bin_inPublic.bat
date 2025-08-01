@echo off
setlocal enabledelayedexpansion

:: �۾� ���丮 ����
set targetdir=public

:: ��¥ ����
for /f "tokens=1-3 delims=/.- " %%a in ("%date%") do (
    set part1=%%a
    set part2=%%b
    set part3=%%c
)

:: ����/��/�� �Ǻ�
set /a test=1%part1% - 10000 >nul 2>&1
if !errorlevel! == 0 (
    set yy=!part1:~2,2!
    set mm=!part2!
    set dd=!part3!
) else (
    set yy=!part3:~2,2!
    set mm=!part2!
    set dd=!part1!
)

:: �ð� ����
set hh=%time:~0,2%
set mn=%time:~3,2%
set ss=%time:~6,2%
if "%hh:~0,1%"==" " set hh=0%hh:~1,1%

:: Ÿ�ӽ����� ����
set timestamp=%yy%%mm%%dd%_%hh%%mn%%ss%

:: ��� ���� ���
set src=%targetdir%\firmware.bin
set dst=%targetdir%\backup_firmware_%timestamp%.bin

:: ���� ���� Ȯ�� �� �̸� ����
if exist "%src%" (
    ren "%src%" "backup_firmware_%timestamp%.bin"
    echo [SUCCESS] %src% �� %dst%
) else (
    echo [ERROR] %src% ������ �������� �ʽ��ϴ�.
)

pause
