#define MY_VER_MAJOR 4
#define MY_VER_MINOR 65
#define MY_VER_BUILD 20
#define MY_VER_BUILDS "20"

#ifdef _UNICODE
#define MY_VER_TYPE " Unicode "
#else
#define MY_VER_TYPE " ANSI "
#endif

#ifdef _WIN64
#define MY_VER_PLF "x64"
#else
#define MY_VER_PLF "x32"
#endif

#define MY_VERSION "4.65 Alt b" MY_VER_BUILDS MY_VER_TYPE MY_VER_PLF

#define MY_7ZIP_VERSION "7-Zip 4.65"
#define MY_DATE "2009-02-03"
#define MY_COPYRIGHT "Copyright (c) 1999-2009 Igor Pavlov"
#define MY_VERSION_COPYRIGHT_DATE MY_VERSION "  " MY_COPYRIGHT "  " MY_DATE
