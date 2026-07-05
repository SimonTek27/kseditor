#pragma once

#ifdef _WIN32
    #ifdef KSASSETTOCORSA_EXPORTS
        #define KS_ASSETTOCORSA_API __declspec(dllexport)
    #else
        #define KS_ASSETTOCORSA_API __declspec(dllimport)
    #endif
#else
    #define KS_ASSETTOCORSA_API __attribute__((visibility("default")))
#endif
