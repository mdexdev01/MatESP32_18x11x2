@echo off

:: 현재 경로를 public으로 이동
cd public

:: deploy 실행
call firebase deploy

:: 명시적으로 배포 명령이 종료된 후 pause 실행
echo ================================
echo [INFO] 배포 명령이 종료되었습니다.
echo 창을 닫으려면 아무 키나 누르세요.
pause
