call prepare.bat

call :clean

vcbuild /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "Release|Win32" 
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "Release x64|x64"
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "ReleaseA|Win32" 
@if errorlevel 1 goto error
vcbuild /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "ReleaseA x64|x64"
@if errorlevel 1 goto error

call :package PluginW b%build%\7zip-465alt-20-b%build% x86
@if errorlevel 1 goto error
call :package PluginW64 b%build%\7zip-465alt64-20-b%build% x64
@if errorlevel 1 goto error
call :package PluginA b%build%\7zip-465alt-b%build%
@if errorlevel 1 goto error
call :package PluginA64 b%build%\7zip-465alt64-b%build%
@if errorlevel 1 goto error

call :clean

@goto end

:clean
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "Release|Win32" 
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "Release x64|x64"
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "ReleaseA|Win32" 
vcbuild /clean /nologo SRC\CPP\7zip\UI\Far\Far_VC9.vcproj "ReleaseA x64|x64"
@goto end

:package
@mkdir .tmp
@copy SRC\Common\* .tmp\
@copy SRC\%1\7-ZipFar.dll .tmp\
@copy SRC\%1\7-ZipFar.map .tmp\
7z a -mx=9 -tzip %2.zip .\.tmp\*
@if errorlevel 1 goto end
@if %3X==X goto skip_installer
nmake -f SRC\Installer\makefile DISTRIB=%2 INSTALLER=SRC\Installer OUTDIR=.tmp PLATFORM=%3 VERSION=4.65.%build% FAR_VERSION=%min_far% 7ZDLL=SRC\7z.dll\%3\7z.dll
@if errorlevel 1 goto end
:skip_installer
@del /q .tmp\*
@rmdir .tmp
@goto end

:error
@echo TERMINATED WITH ERRORS

:end
