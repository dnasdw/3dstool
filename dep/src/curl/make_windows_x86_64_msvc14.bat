IF DEFINED VS140COMNTOOLS (
  SET GENERATOR="Visual Studio 14 Win64"
) ELSE (
  FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7 /v 15.0 ^| FIND "15.0"') DO SET GENERATOR="Visual Studio 15 Win64"
)
SET target_lib_suffix=_msvc14
IF NOT DEFINED GENERATOR (
  ECHO Can not find VC2015 or VC2017 installed!
  GOTO ERROR
)
SET cwdir=%CD%
SET rootdir=%~dp0
SET target=windows_x86_64
SET prefix=%rootdir%%target%
SET /P version=<"%rootdir%version.txt"
RD /S /Q "%rootdir%%version%"
MD "%rootdir%%version%"
XCOPY "%rootdir%..\%version%" "%rootdir%%version%" /S /Y
RD /S /Q "%rootdir%build"
MD "%rootdir%build"
CD /D "%rootdir%build"
cmake -C "%rootdir%CMakeLists-MSVC.txt" -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DCURL_STATICLIB=ON -DCURL_DISABLE_LDAP=ON -DCURL_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="%prefix%" -G %GENERATOR% "%rootdir%%version%"
cmake "%rootdir%%version%"
cmake --build . --target install --config Release --clean-first
MD "%rootdir%..\..\include\%target%"
XCOPY "%prefix%\include" "%rootdir%..\..\include\%target%" /S /Y
MD "%rootdir%..\..\lib\%target%%target_lib_suffix%"
COPY /Y "%prefix%\lib\libcurl.lib" "%rootdir%..\..\lib\%target%%target_lib_suffix%"
CD /D "%cwdir%"
RD /S /Q "%rootdir%%version%"
RD /S /Q "%rootdir%build"
RD /S /Q "%prefix%"
GOTO :EOF

:ERROR
PAUSE
