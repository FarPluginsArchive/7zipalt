rd /S /Q SRC\CPP\7zip\UI\Far\Release
rd /S /Q SRC\CPP\7zip\UI\Far\ReleaseA
rd /S /Q SRC\CPP\7zip\UI\Far\Debug
rd /S /Q SRC\CPP\7zip\UI\Far\DebugA
rd /S /Q SRC\CPP\7zip\UI\Far\Release64
rd /S /Q SRC\CPP\7zip\UI\Far\ReleaseA64
rd /S /Q SRC\CPP\7zip\UI\Far\Debug64
rd /S /Q SRC\CPP\7zip\UI\Far\DebugA64
rd /S /Q SRC\CPP\7zip\UI\Far\x64
del /Q SRC\*.ncb
del /Q SRC\*.suo
del /Q SRC\CPP\7zip\UI\Far\*.aps
del /Q src\output\*.*
del /Q src\PluginA\*.*
del /Q src\PluginW\*.*
del /Q src\PluginA64\*.*
del /Q src\PluginW64\*.*
copy dummy.txt src\output
copy dummy.txt src\PluginA
copy dummy.txt src\PluginW
copy dummy.txt src\PluginA64
copy dummy.txt src\PluginW64
7za a -mx=9 -tzip -r b%1\7zip-465alt-src-b%1.zip src/*.*
7za a -mx=9 -tzip b%1\7zip-465alt-src-b%1.zip *.bat
7za a -mx=9 -tzip b%1\7zip-465alt-src-b%1.zip dummy.txt
del /Q src\output\*.*
del /Q src\PluginA\*.*
del /Q src\PluginW\*.*
del /Q src\PluginA64\*.*
del /Q src\PluginW64\*.*