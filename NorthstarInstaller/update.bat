@echo off
ping 127.0.0.1 -n 2  nul
xcopy /s/e/y Data\temp\install .\
start NorthstarInstaller.exe
exit