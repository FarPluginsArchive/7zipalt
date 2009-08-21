set build=20

rem you can setup environment for compilation
rem example for Visual Studio 2005
rem place it in setenv.bat: "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
if exist setenv.bat call setenv.bat

rem SRC\Common\FILE_ID.DIZ
rem SRC\CPP\7zip\MyVersion.h

call :clean

vcbuild /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "Release|Win32" 
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "Release x64|x64"
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "ReleaseA|Win32" 
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "ReleaseA x64|x64"
@if errorlevel 1 goto error

call :package PluginW b%build%\7zip-465alt-20-b%build%.zip
call :package PluginW64 b%build%\7zip-465alt64-20-b%build%.zip
call :package PluginA b%build%\7zip-465alt-b%build%.zip
call :package PluginA64 b%build%\7zip-465alt64-b%build%.zip

call :clean

@goto end

:clean
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "Release|Win32" 
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "Release x64|x64"
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "ReleaseA|Win32" 
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far.vcproj "ReleaseA x64|x64"
@goto end

:package
@mkdir .tmp
@copy SRC\Common\* .tmp\
@copy SRC\%1\* .tmp\
7z a -mx=9 -tzip %2 .\.tmp\*
@del /q .tmp\*
@rmdir .tmp
@goto end

:error
@echo TERMINATED WITH ERRORS

:end
