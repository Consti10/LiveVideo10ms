//
// Created by Constantin on 27.10.2017.
//

#ifndef OSDTESTER_CPUPRIORITY_H
#define OSDTESTER_CPUPRIORITY_H

#include <sys/resource.h>
#include <unistd.h>
#include <android/log.h>
#include <string>

static const void setCPUPriority(const int priority,const std::string caller){
    int which = PRIO_PROCESS;
    auto pid = getpid();
    int ret = setpriority(which, (id_t)pid, priority);
    if(ret!=0){
        __android_log_print(ANDROID_LOG_DEBUG, "CPUPrio1","Error set thread priority to:%d in %s",priority,caller.c_str());
    }
    pid = getpid();
    ret = getpriority(which, (id_t)pid);
}

#endif //OSDTESTER_CPUPRIORITY_H
