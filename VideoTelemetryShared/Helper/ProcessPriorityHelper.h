//
// Created by geier on 17/05/2020.
//

#ifndef FPV_VR_OS_PROCESSPRIORITYHELPER_H
#define FPV_VR_OS_PROCESSPRIORITYHELPER_H


#include <AndroidLogger.hpp>
#include <sys/resource.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

namespace ProcessPriorityHelper{
    constexpr auto TAG="ProcessPriorityHelper";
    static const int getCurrentProcessPriority(){
        int which = PRIO_PROCESS;
        id_t pid = (id_t)getpid();
        return getpriority(which, pid);
    }
    static const void printCPUPriority(const char* caller){
        const int priority=getCurrentProcessPriority();
        LOGD(TAG)<<"Priority is "<<priority<<"in"<<caller;
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
            LOGE(TAG)<<"ERROR set thread priority to:"<<wantedPriority<<" from "<<currentPriority<<" in "<<caller<<" pid "<<pid);
        }else{
            LOGD(TAG)<<"SUCCESS Set thread priority to:"<<wantedPriority<<" from "<<currentPriority<<" in "<<caller<<" pid "<<pid);
        }
    }

    static void setAffinity(int core){
        cpu_set_t  cpuset;
        CPU_ZERO(&cpuset);       //clears the cpuset
        CPU_SET( core, &cpuset); //set CPU x on cpuset*/
        long err,syscallres;
        pid_t pid=gettid();
        syscallres=syscall(__NR_sched_setaffinity,pid, sizeof(cpuset),&cpuset);
        if(syscallres) {
            //PrivLOG("Error sched_setaffinity");
        }
    }

    static const void printX(){
        int ret;
        // We'll operate on the currently running thread.
        pthread_t this_thread = pthread_self();

        // struct sched_param is used to store the scheduling priority
        struct sched_param params;
        auto errror=sched_getparam(this_thread,&params);
        LOGD(TAG)<<"sched_getparam returns "<<errror;

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
#include <thread>
namespace TEST_PROCESS_PRIO{
    static void setThreadPriorityContiniously(const int prio,const char* name) {
        while (true){
            ProcessPriorityHelper::setCPUPriorityXXX(prio,name);
        }
    }
    static void testXXX(){
        std::thread* thread1=new std::thread(setThreadPriorityContiniously,1,"Thread 1");
        std::thread* thread2=new std::thread(setThreadPriorityContiniously,2,"Thread 2");
    }
}

#endif //FPV_VR_OS_PROCESSPRIORITYHELPER_H
