IF DEFINED VS140COMNTOOLS (
  SET VCVARSALL="%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
)
IF NOT DEFINED VCVARSALL (
  FOR /F %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft ^| FINDSTR "VisualStudio_"') DO CALL :FINDVS "%%~I" 2019
)
IF NOT DEFINED VCVARSALL (
  FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7 /v 15.0 ^| FINDSTR "15.0"') DO SET VCVARSALL="%%~KVC\Auxiliary\Build\vcvarsall.bat"
)
SET target_lib_suffix=_msvc14
IF NOT DEFINED VCVARSALL (
  ECHO Can not find VC2015 or VC2017 or VC2019 installed!
  GOTO ERROR
)
CALL %VCVARSALL% x86
@ECHO ON
PUSHD "%~dp0"
SET tmpdir=%~d0\tmp_3dstool_openssl
SET target=windows_x86_32
SET prefix=%tmpdir%\%target%
SET openssldir=%prefix%\ssl
SET /P version=<version.txt
RD /S /Q "%tmpdir%\%version%"
MD "%tmpdir%\%version%"
XCOPY "..\%version%" "%tmpdir%\%version%" /S /Y
PUSHD "%tmpdir%\%version%"
perl Configure VC-WIN32 no-shared no-asm --prefix="%prefix%" --openssldir="%openssldir%"
nmake
nmake install
POPD
MD "..\..\include\%target%"
XCOPY "%prefix%\include" "..\..\include\%target%" /S /Y
MD "..\..\lib\%target%%target_lib_suffix%"
COPY /Y "%prefix%\lib\*.lib" "..\..\lib\%target%%target_lib_suffix%"
POPD
RD /S /Q "%tmpdir%"
GOTO :EOF

:FINDVS
SET vsid=%~1
SET vsid=%vsid:~63%
REG QUERY %~1\Capabilities | FINDSTR /R "ApplicationName.*REG_SZ.*Microsoft.Visual.Studio.%~2" && FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\%vsid% ^| FINDSTR "InstallLocation"') DO SET VCVARSALL="%%~K\VC\Auxiliary\Build\vcvarsall.bat"
GOTO :EOF

:ERROR
PAUSE
