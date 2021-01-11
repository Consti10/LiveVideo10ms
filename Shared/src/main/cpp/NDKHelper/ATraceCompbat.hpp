//
// Created by geier on 20/08/2020.
//

#ifndef RENDERINGX_ATRACECOMPBAT_H
#define RENDERINGX_ATRACECOMPBAT_H

// If the selected android api is smaller than the minimum for ATrace, I declare
// dummy methods for ATrace such that the code compiles even tough the functionality is not available
// To enable tracing, re-compile code with minapi 23 (minimum for proper tracing with the android ndk)
// This can be usefull if you want to use tracing in your project when debugging but use a minapi lower than 23 for releases
#if __ANDROID_API__ >= 23
#include <android/trace.h>
static constexpr const bool isATraceAvailable=true;
#else

static void ATrace_beginSection(const char* sectionName){}
static void ATrace_endSection(){}
static constexpr const bool isATraceAvailable=false;
#endif

#endif //RENDERINGX_ATRACECOMPBAT_H
