//
// Created by Constantin on 27.10.2017.
//

#ifndef FPV_VR_CPUPRIORITIES_H
#define FPV_VR_CPUPRIORITIES_H

#include <sys/resource.h>
#include <unistd.h>
#include <android/log.h>

// This namespace contains helper to set specific CPU thread priorities to deal
// With scheduling in a multi-threaded application
namespace CPUPriority{
    constexpr int ANDROID_PRIORITY_LOWEST         =  19;

    //use for background tasks
    constexpr int ANDROID_PRIORITY_BACKGROUND     =  10;

    //most threads run at normal priority
    constexpr int ANDROID_PRIORITY_NORMAL         =   0;

    //threads currently running a UI that the user is interacting with
    constexpr int ANDROID_PRIORITY_FOREGROUND     =  -2;

    //the main UI thread has a slightly more favorable priority
    constexpr int ANDROID_PRIORITY_DISPLAY        =  -4;
    //ui service treads might want to run at a urgent display (uncommon)
    constexpr int ANDROID_PRIORITY_URGENT_DISPLAY =  -8;

    //all normal audio threads
    constexpr int ANDROID_PRIORITY_AUDIO          = -16;

    //service audio threads (uncommon)
    constexpr int ANDROID_PRIORITY_URGENT_AUDIO   = -19;

    //should never be used in practice. regular process might not
    //be allowed to use this level
    constexpr int ANDROID_PRIORITY_HIGHEST        = -20;

    constexpr int ANDROID_PRIORITY_DEFAULT        = ANDROID_PRIORITY_NORMAL;
    constexpr int ANDROID_PRIORITY_MORE_FAVORABLE = -1;
    constexpr int ANDROID_PRIORITY_LESS_FAVORABLE = +1;

    static const int getCurrentProcessPriority(){
        int which = PRIO_PROCESS;
        id_t pid = (id_t)getpid();
        return getpriority(which, pid);
    }
    static const void printCPUPriority(const char* caller){
        const int priority=getCurrentProcessPriority();
        __android_log_print(ANDROID_LOG_DEBUG, "CPUPrio", "Priority is %d in %s",priority,caller);
    }
    // If the current priority == wanted priority do nothing
    // Else, set the wanted priority and log Error / Success
    static const void setCPUPriority(const int wantedPriority, const char* caller){
        const int currentPriority=getCurrentProcessPriority();
        if(currentPriority == wantedPriority)return;
        int which = PRIO_PROCESS;
        auto pid = getpid();
        int ret = setpriority(which, (id_t)pid, wantedPriority);
        const int currentPriorityAfterSet=getCurrentProcessPriority();
        if(ret!=0 || currentPriorityAfterSet != wantedPriority){
            __android_log_print(ANDROID_LOG_DEBUG, "CPUPrio1", "ERROR set thread priority to:%d from %d in %s", wantedPriority, currentPriority, caller);
        }else{
            __android_log_print(ANDROID_LOG_DEBUG, "CPUPrio1", "SUCCESS Set thread priority to:%d from %d in %s", wantedPriority, currentPriority, caller);
        }
    }
}

// All these values are for FPV_VR - remember that this file is duplicated so be carefully when changing values
namespace FPV_VR_PRIORITY{
    constexpr int CPU_PRIORITY_GLRENDERER_STEREO_FB=-19; //This one needs a whole CPU core all the time anyways
    constexpr int CPU_PRIORITY_GLRENDERER_STEREO=-16; //The GL thread also should get 1 whole cpu core
    constexpr int CPU_PRIORITY_UDPRECEIVER_VIDEO=-16;  //needs low latency and does not use the cpu that much
    constexpr int CPU_PRIORITY_DECODER_OUTPUT=-16;     //needs low latency and does not use the cpu that much
    constexpr int CPU_PRIORITY_UVC_FRAME_CALLBACK=-16; //needs low latency but uses CPU a lot (decoding)
    // These are much lower
    constexpr int CPU_PRIORITY_GLRENDERER_MONO=-4; //only shows the OSD not video
    constexpr int CPU_PRIORITY_UDPRECEIVER_TELEMETRY=-4; //not as important as video but also needs almost no CPU processing time
    constexpr int CPU_PRIORITY_UDPSENDER_HEADTRACKING=-4;
}

#endif //FPV_VR_CPUPRIORITIES_H
