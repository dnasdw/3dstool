IF DEFINED VS140COMNTOOLS (
  SET VCVARSALL="%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE (
  FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7 /v 15.0 ^| FIND "15.0"') DO SET VCVARSALL="%%~KVC\Auxiliary\Build\vcvarsall.bat"
)
SET target_lib_suffix=_msvc14
IF NOT DEFINED VCVARSALL (
  ECHO Can not find VC2015 or VC2017 installed!
  GOTO ERROR
)
CALL %VCVARSALL% x86
@ECHO ON
SET cwdir=%CD%
SET rootdir=%~dp0
SET tmpdir=%~d0\tmp_3dstool_openssl\
SET target=windows_x86_32
SET prefix=%tmpdir%%target%
SET openssldir=%prefix%\ssl
SET /P version=<"%rootdir%version.txt"
RD /S /Q "%tmpdir%%version%"
MD "%tmpdir%%version%"
XCOPY "%rootdir%..\%version%" "%tmpdir%%version%" /S /Y
CD /D "%tmpdir%%version%"
perl Configure VC-WIN32 no-shared no-asm no-dso --prefix="%prefix%" --openssldir="%openssldir%"
CALL ms\do_ms.bat
nmake -f ms\nt.mak
nmake -f ms\nt.mak install
MD "%rootdir%..\..\include\%target%"
XCOPY "%prefix%\include" "%rootdir%..\..\include\%target%" /S /Y
MD "%rootdir%..\..\lib\%target%%target_lib_suffix%"
COPY /Y "%prefix%\lib\libeay32.lib" "%rootdir%..\..\lib\%target%%target_lib_suffix%"
CD /D "%cwdir%"
RD /S /Q "%tmpdir%"
RD /S /Q "%prefix%"
GOTO :EOF

:ERROR
PAUSE
