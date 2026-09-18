#pragma once

#ifdef _WIN32
    #ifdef KSASSETTOCORSAEVO_EXPORTS
        #define KS_ASSETTOCORSAEVO_API __declspec(dllexport)
    #else
        #define KS_ASSETTOCORSAEVO_API __declspec(dllimport)
    #endif
#else
    #define KS_ASSETTOCORSAEVO_API __attribute__((visibility("default")))
#endif
