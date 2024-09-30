#pragma once
#define  PLATFORM_WINDOWS

typedef signed char mbyte;
typedef long long mlong;       //long 8
typedef unsigned long long mulong;  //unsig long 8
typedef unsigned long long Addr;  //unsig long 8
typedef long long offset;  // long 8
typedef double mdouble;       //double 8
typedef float mfloat;         //float 4
typedef unsigned int uint;
typedef unsigned char boolean;
typedef unsigned char uchar;
typedef unsigned int CharType;
typedef unsigned char UTF8;
typedef unsigned short UTF16;
typedef unsigned short ushort;
typedef unsigned int UTF32;
typedef signed short int int16;

typedef void* Handle;

struct RADDR {
    Addr addr;                    // 起始地址
    Addr taddr;                   // 结束地址
};
