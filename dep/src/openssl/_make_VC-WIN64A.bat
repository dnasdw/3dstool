IF DEFINED VS120COMNTOOLS (
  SET VCVARSALL="%VS120COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE IF DEFINED VS110COMNTOOLS (
  SET VCVARSALL="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
) ELSE IF DEFINED VS100COMNTOOLS (
  SET VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
)
IF NOT DEFINED VCVARSALL (
  ECHO Can not find VC2010 or VC2012 or VC2013 installed!
  GOTO ERROR
)
CALL %VCVARSALL% amd64
SET rootdir=%~dp0
SET target=WIN64
SET prefix=%rootdir%%target%
SET openssldir=%prefix%\ssl
SET /P openssl_version=<"%rootdir%openssl.txt"
RD /S /Q "%rootdir%%openssl_version%"
MD "%rootdir%%openssl_version%"
XCOPY "%rootdir%..\%openssl_version%" "%rootdir%%openssl_version%" /S /Y
CD /D "%rootdir%..\%openssl_version%"
perl Configure VC-WIN64A --prefix="%prefix%" --openssldir="%openssldir%"
CALL ms\do_win64a.bat
nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak install
RD /S /Q "%rootdir%..\..\include\%target%\openssl"
XCOPY "%prefix%\include" "%rootdir%..\..\include\%target%" /S /Y
COPY /Y "%prefix%\lib\libeay32.lib" "%rootdir%..\..\lib\%target%"
CD /D "%rootdir%"
RD /S /Q "%rootdir%%openssl_version%"
GOTO EOF

:ERROR
PAUSE

:EOF
