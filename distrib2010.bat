call prepare.bat

call :clean

msbuild /nologo /p:Configuration="Release" /p:Platform="Win32" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
@if errorlevel 1 goto error
msbuild /nologo /p:Configuration="Release x64" /p:Platform="x64" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
@if errorlevel 1 goto error
msbuild /nologo /p:Configuration="ReleaseA" /p:Platform="Win32" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
@if errorlevel 1 goto error
msbuild /nologo /p:Configuration="ReleaseA x64" /p:Platform="x64" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
@if errorlevel 1 goto error

call :package PluginW b%build%\7zip-465alt-20-b%build%.zip
call :package PluginW64 b%build%\7zip-465alt64-20-b%build%.zip
call :package PluginA b%build%\7zip-465alt-b%build%.zip
call :package PluginA64 b%build%\7zip-465alt64-b%build%.zip

call :clean

@goto end

:clean
msbuild /nologo /t:clean /p:Configuration="Release" /p:Platform="Win32" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
msbuild /nologo /t:clean /p:Configuration="Release x64" /p:Platform="x64" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
msbuild /nologo /t:clean /p:Configuration="ReleaseA" /p:Platform="Win32" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
msbuild /nologo /t:clean /p:Configuration="ReleaseA x64" /p:Platform="x64" SRC\CPP\7zip\UI\Far\Far_VC10.vcxproj
@goto end

:package
@mkdir .tmp
@copy SRC\Common\* .tmp\
@copy SRC\%1\7-ZipFar.dll .tmp\
@copy SRC\%1\7-ZipFar.map .tmp\
7z a -mx=9 -tzip %2 .\.tmp\*
@del /q .tmp\*
@rmdir .tmp
@goto end

:error
@echo TERMINATED WITH ERRORS

:end
