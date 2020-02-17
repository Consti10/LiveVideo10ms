//
// Created by geier on 24/01/2020.
//

#ifndef LIVEVIDEO10MS_FRAMELIMITER_HPP
#define LIVEVIDEO10MS_FRAMELIMITER_HPP

#include <chrono>


class FrameLimiter{
public:
    //passing 0 or -1 as maxFPS means this call returns immediately
    //Else,it blocks until at least (1000/maxFPS)ms are elapsed since the last call
    //to limitFps when receiving from a video file.
    void limitFps(const int maxFPS){
        if(maxFPS<=0){
            return;
        }
        const float minTimeBetweenFramesIfEnabled=1000.0f/((float)maxFPS);
        //LOGD("Min frame time of %f",minTimeBetweenFramesIfEnabled);
        const auto minimumTimeBetweenMS=minTimeBetweenFramesIfEnabled;
        while(true){
            const auto now=std::chrono::steady_clock::now();
            const auto deltaSinceLastFrame=now-lastTimeCalled;
            const int64_t deltaSinceLastFrameMS=std::chrono::duration_cast<std::chrono::milliseconds>(deltaSinceLastFrame).count();
            if(deltaSinceLastFrameMS>=minimumTimeBetweenMS){
                break;
            }
        }
        lastTimeCalled=std::chrono::steady_clock::now();
        /*const auto now=steady_clock::now();
        const auto deltaSinceLastFrame=now-lastFrameLimitFPS;
        const int64_t waitTimeMS=32-duration_cast<milliseconds>(deltaSinceLastFrame).count();
        lastFrameLimitFPS=now;
        if(waitTimeMS>0){
            try{
                LOGD("Sleeping for %d",waitTimeMS);
                std::this_thread::sleep_for(milliseconds(waitTimeMS));
            }catch (...){
            }
        }*/
    }
private:
    std::chrono::steady_clock::time_point lastTimeCalled=std::chrono::steady_clock::now();
};

#endif //LIVEVIDEO10MS_FRAMELIMITER_HPP
