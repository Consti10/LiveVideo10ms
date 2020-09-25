//
// Created by geier on 20/08/2020.
//

#ifndef RENDERINGX_ATRACECOMPBAT_H
#define RENDERINGX_ATRACECOMPBAT_H

// If the selected android api is smaller than the minimum for ATrace, I declare
// dummy methods for ATrace such that the code compiles even tough the functionality is not available

#include <android/trace.h>

#if __ANDROID_API__ >= 23
// Do nothing
#else

static void ATrace_beginSection(const char* sectionName){}
static void ATrace_endSection(){}
#endif

#endif //RENDERINGX_ATRACECOMPBAT_H
