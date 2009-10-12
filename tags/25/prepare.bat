@set build=25
@set bdata=2009-10-12

@rem ------------------------------
@rem prepare FILE_ID.DIZ
@rem ------------------------------
@echo Generating FILE_ID.DIZ
@echo 7-ZipFar 4.65 Alternative build %build% %bdata% > SRC\COMMON\FILE_ID.DIZ
@echo All features of original 7-ZipFar  >> SRC\COMMON\FILE_ID.DIZ
@echo with some additions (see readme and help)  >> SRC\COMMON\FILE_ID.DIZ

@rem ------------------------------
@rem prepare MyVersion.h
@rem ------------------------------
@echo Generating MyVersion.h
@echo #define MY_VER_MAJOR 4 > SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_MINOR 65 >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_BUILD %build% >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_BUILDS "%build%" >> SRC\CPP\7zip\MyVersion.h
@echo #ifdef _UNICODE >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_TYPE " Unicode " >> SRC\CPP\7zip\MyVersion.h
@echo #else >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_TYPE " ANSI " >> SRC\CPP\7zip\MyVersion.h
@echo #endif >> SRC\CPP\7zip\MyVersion.h
@echo #ifdef _WIN64 >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_PLF "x64" >> SRC\CPP\7zip\MyVersion.h
@echo #else >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VER_PLF "x32" >> SRC\CPP\7zip\MyVersion.h
@echo #endif >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VERSION "4.65 Alt b" MY_VER_BUILDS MY_VER_TYPE MY_VER_PLF >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_7ZIP_VERSION "7-Zip 4.65" >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_DATE "%bdata%" >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_COPYRIGHT "Copyright (c) 1999-2009 Igor Pavlov" >> SRC\CPP\7zip\MyVersion.h
@echo #define MY_VERSION_COPYRIGHT_DATE MY_VERSION "  " MY_COPYRIGHT "  " MY_DATE >> SRC\CPP\7zip\MyVersion.h
