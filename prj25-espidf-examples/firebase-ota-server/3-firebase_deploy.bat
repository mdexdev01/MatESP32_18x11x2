@echo off

:: ���� ��θ� public���� �̵�
cd public

:: deploy ����
call firebase deploy

:: ��������� ���� ����� ����� �� pause ����
echo ================================
echo [INFO] ���� ����� ����Ǿ����ϴ�.
echo â�� �������� �ƹ� Ű�� ��������.
pause
