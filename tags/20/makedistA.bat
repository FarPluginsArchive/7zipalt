del /Q src\output\*.*
copy src\common\*.* src\output\
copy src\pluginA\*.* src\output\
cd src\output
7za a -mx=9 -tzip ..\..\b%1\7zip-465alt-b%1.zip *.*
cd ..\..

del /Q src\output\*.*
copy src\common\*.* src\output\
copy src\pluginA64\*.* src\output\
cd src\output
7za a -mx=9 -tzip ..\..\b%1\7zip-465alt64-b%1.zip *.*
cd ..\..