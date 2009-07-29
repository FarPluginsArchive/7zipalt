del /Q src\output\*.*
copy src\common\*.* src\output\
copy src\pluginW\*.* src\output\
cd src\output
7za a -mx=9 -tzip ..\..\b%1\7zip-465alt-20-b%1.zip *.*
cd ..\..

del /Q src\output\*.*
copy src\common\*.* src\output\
copy src\pluginW64\*.* src\output\
cd src\output
7za a -mx=9 -tzip ..\..\b%1\7zip-465alt64-20-b%1.zip *.*
cd ..\..