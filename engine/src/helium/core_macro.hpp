#pragma once


#if defined(__GNUC__) && !defined(__clang__)
#    define GCC_COMPILER 1
#elif defined(__clang__)
#    define CLANG_COMPILER 1
#elif defined(_MSC_VER)
#    define MSVC_COMPILER 1
#endif
