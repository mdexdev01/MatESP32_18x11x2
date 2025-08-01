@echo off
setlocal enabledelayedexpansion

:: 작업 디렉토리 설정
set targetdir=public

:: 날짜 추출
for /f "tokens=1-3 delims=/.- " %%a in ("%date%") do (
    set part1=%%a
    set part2=%%b
    set part3=%%c
)

:: 연도/월/일 판별
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

:: 시간 추출
set hh=%time:~0,2%
set mn=%time:~3,2%
set ss=%time:~6,2%
if "%hh:~0,1%"==" " set hh=0%hh:~1,1%

:: 타임스탬프 생성
set timestamp=%yy%%mm%%dd%_%hh%%mn%%ss%

:: 대상 파일 경로
set src=%targetdir%\firmware.bin
set dst=%targetdir%\backup_firmware_%timestamp%.bin

:: 파일 존재 확인 후 이름 변경
if exist "%src%" (
    ren "%src%" "backup_firmware_%timestamp%.bin"
    echo [SUCCESS] %src% → %dst%
) else (
    echo [ERROR] %src% 파일이 존재하지 않습니다.
)

pause
