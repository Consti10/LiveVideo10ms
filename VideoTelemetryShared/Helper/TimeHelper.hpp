//
// Created by geier on 18/01/2020.
//

#ifndef LIVEVIDEO10MS_TIMEHELPER_HPP
#define LIVEVIDEO10MS_TIMEHELPER_HPP

#include <AndroidLogger.hpp>
#include <chrono>

class AvgCalculator{
private:
    // provide us (microseconds) resolution
    unsigned long sumUs=0;
    long sumCount=0;
public:
    AvgCalculator() = default;
    void add(const std::chrono::steady_clock::duration& duration){
        const auto us=std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        if(us>=0){
            sumUs+=us;
            sumCount++;
        }else{
            MLOGE<<"Negative duration "<<us<<" us";
        }
    }
    //void addNs(long duration){
    //    add(std::chrono::nanoseconds(duration));
    //}
    long getAvg_us(){
        if(sumCount==0)return 0;
        return sumUs / sumCount;
    }
    // milliseconds & float is the most readable format
    float getAvg_ms(){
        return (float)getAvg_us()/1000.0f;
    }
    void reset(){
        sumUs=0;
        sumCount=0;
    }
};

class RelativeCalculator{
private:
    long sum=0;
    long sumAtLastCall=0;
public:
    RelativeCalculator() = default;
    void add(unsigned long x){
        sum+=x;
    }
    long getDeltaSinceLastCall() {
        long ret = sum - sumAtLastCall;
        sumAtLastCall = sum;
        return ret;
    }
    long getAbsolute(){
        return sum;
    }
};

class MyTimeHelper{
public:
    // R stands for readable. Convert a std::chrono::duration into a readable format
    static std::string R(const std::chrono::steady_clock::duration& dur){
        if(dur>=std::chrono::seconds(1)){
            // More than one second, print as decimal with ms resolution.
            const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            return std::to_string(ms/1000.0f)+" s ";
        }
        if(dur>=std::chrono::milliseconds(1)){
            // More than one millisecond, print as decimal with us resolution
            const auto us=std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
            return std::to_string(us/1000.0f)+" ms ";
        }
        if(dur>=std::chrono::microseconds(1)){
            // More than one microsecond, print as decimal with ns resolution
            const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
            return std::to_string(ns/1000.0f)+" us ";
        }
        const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
        return std::to_string(ns)+" ns ";
    }
};

class MeasureExecutionTime{
private:
    const std::chrono::steady_clock::time_point begin;
    const std::string functionName;
    const std::string tag;
public:
    MeasureExecutionTime(const std::string& tag,const std::string& functionName):functionName(functionName),tag(tag),begin(std::chrono::steady_clock::now()){}
    ~MeasureExecutionTime(){
        const auto duration=std::chrono::steady_clock::now()-begin;
        MLOGD2(tag)<<"Execution time for "<<functionName<<" is "<<std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()<<"ms";
    }
};

// Macro to measure execution time of a specific function.
// See https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c/61886741#61886741
// Example output: ExecutionTime: For DecodeMJPEGtoANativeWindowBuffer is 54ms
// __CLASS_NAME__ comes from AndroidLogger
#define MEASURE_FUNCTION_EXECUTION_TIME const MeasureExecutionTime measureExecutionTime(__CLASS_NAME__,__FUNCTION__);

#include <chrono>
namespace TestSleep{
    //template <class _Rep, class _Period>
    static void sleep(const std::chrono::steady_clock::duration &duration,const bool print=false){
        const auto before=std::chrono::steady_clock::now();
        std::this_thread::sleep_for(duration);
        const auto actualSleepTime=std::chrono::steady_clock::now()-before;
        if(print){
            MLOGD<<"Slept for "<<MyTimeHelper::R(actualSleepTime)<<" instead of "<<MyTimeHelper::R(duration);
        }
    }
}
#endif //LIVEVIDEO10MS_TIMEHELPER_HPP