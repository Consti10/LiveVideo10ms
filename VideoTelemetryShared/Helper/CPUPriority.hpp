//
// Created by Constantin on 27.10.2017.
//

#ifndef FPV_VR_CPUPRIORITIES_H
#define FPV_VR_CPUPRIORITIES_H

#include <sys/resource.h>
#include <unistd.h>
#include <android/log.h>
#include <sched.h>
#include <pthread.h>
//#include <MDebug.hpp>

// This namespace contains helper to set specific CPU thread priorities to deal
// With scheduling in a multi-threaded application
namespace CPUPriority{

    static constexpr auto TAG="CPUPriority";
    static void CPULOGD(const char* fmt,...) {
        va_list argptr;
        va_start(argptr, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG, TAG,fmt,argptr);
        va_end(argptr);
    }
    /*template<class... Args>
    static void CPULOGD(const Args&... args) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG,(const char*)args...);
    }*/

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
    static const void setCPUPriorityXXX(const int wantedPriority, const char* caller){
        const int currentPriority=getCurrentProcessPriority();
        if(currentPriority == wantedPriority)return;
        int which = PRIO_PROCESS;
        auto pid = getpid();
        int ret = setpriority(which, (id_t)pid, wantedPriority);
        const int currentPriorityAfterSet=getCurrentProcessPriority();
        if(ret!=0 || currentPriorityAfterSet != wantedPriority){
            CPULOGD("ERROR set thread priority to:%d from %d in %s pid %d", wantedPriority, currentPriority, caller,(int)pid);
        }else{
            CPULOGD("SUCCESS Set thread priority to:%d from %d in %s pid %d", wantedPriority, currentPriority, caller,(int)pid);
        }
    }

    static const void printX(){
        int ret;
        // We'll operate on the currently running thread.
        pthread_t this_thread = pthread_self();

        // struct sched_param is used to store the scheduling priority
        struct sched_param params;
        auto errror=sched_getparam(this_thread,&params);
        CPULOGD("sched_getparam returns %d",errror);

        // We'll set the priority to the maximum.
        /*CPULOGD("Scheduler is %d",sched_getscheduler(this_thread));

        params.sched_priority = sched_get_priority_max(SCHED_FIFO);

        CPULOGD("Trying to set thread realtime prio = %d ",params.sched_priority);

        // Attempt to set thread real-time priority to the SCHED_FIFO policy
        ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
        if (ret != 0) {
            // Print the error
            CPULOGD("Unsuccessful in setting thread realtime prio");
            return;
        }
        sched_getparam()*/
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

#include <thread>
namespace TEST_CPU_PRIO{
    static void setThreadPriorityContiniously(const int prio,const char* name) {
        while (true){
            CPUPriority::setCPUPriorityXXX(prio,name);
        }
    }
    static void testXXX(){
        std::thread* thread1=new std::thread(setThreadPriorityContiniously,1,"Thread 1");
        std::thread* thread2=new std::thread(setThreadPriorityContiniously,2,"Thread 2");
    }
}


#endif //FPV_VR_CPUPRIORITIES_H
