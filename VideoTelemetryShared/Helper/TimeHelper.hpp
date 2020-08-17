//
// Created by geier on 18/01/2020.
//

#ifndef LIVEVIDEO10MS_TIMEHELPER_HPP
#define LIVEVIDEO10MS_TIMEHELPER_HPP

#include <AndroidLogger.hpp>
#include <chrono>

namespace MyTimeHelper{
    // R stands for readable. Convert a std::chrono::duration into a readable format
    static std::string R(const std::chrono::steady_clock::duration& dur){
        const auto durAbsolute=std::chrono::abs(dur);
        if(durAbsolute>=std::chrono::seconds(1)){
            // More than one second, print as decimal with ms resolution.
            const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            return std::to_string(ms/1000.0f)+"s";
        }
        if(durAbsolute>=std::chrono::milliseconds(1)){
            // More than one millisecond, print as decimal with us resolution
            const auto us=std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
            return std::to_string(us/1000.0f)+"ms";
        }
        if(durAbsolute>=std::chrono::microseconds(1)){
            // More than one microsecond, print as decimal with ns resolution
            const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
            return std::to_string(ns/1000.0f)+"us";
        }
        const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count();
        return std::to_string(ns)+"ns";
    }
    static std::string ReadableNS(uint64_t nanoseconds){
        return R(std::chrono::nanoseconds(nanoseconds));
    }
};

// Use this class to compare many samples of the same kind
// Saves the minimum,maximum and average of all the samples
class AvgCalculator{
private:
    // do not forget the braces to initalize with 0
    std::chrono::nanoseconds sum{};
    long nSamples=0;
    std::chrono::nanoseconds min=std::chrono::nanoseconds::max();
    std::chrono::nanoseconds max{};
public:
    AvgCalculator() = default;
    // typedef duration<long long,         nano> nanoseconds;
    // I think std::chrono::nanoseconds is a duration
    void add(const std::chrono::nanoseconds& value){
        if(value<std::chrono::nanoseconds(0)){
            MLOGE<<"Cannot add negative value";
            return;
        }
        sum+=value;
        nSamples++;
        if(value<min){
            min=value;
        }
        if(value>max){
            max=value;
        }
    }
    std::chrono::nanoseconds getAvg()const{
        if(nSamples == 0)return std::chrono::nanoseconds(0);
        return sum / nSamples;
    }
    std::chrono::nanoseconds getMin()const{
        return min;
    }
    std::chrono::nanoseconds getMax()const{
        return max;
    }
    // max delta between average and min / max
    std::chrono::nanoseconds getMaxDifferenceMinMaxAvg()const{
        const auto deltaMin=std::chrono::abs(getAvg()-getMin());
        const auto deltaMax=std::chrono::abs(getAvg()-getMax());
        if(deltaMin>deltaMax)return deltaMin;
        return deltaMax;
    }
    float getAvg_ms(){
        return (float)(std::chrono::duration_cast<std::chrono::microseconds>(getAvg()).count())/1000.0f;
    }
    long getNSamples()const{
        return nSamples;
    }
    void reset(){
        sum={};
        nSamples=0;
        min=std::chrono::nanoseconds::max();
        max={};
    }
    std::string getAvgReadable(const bool averageOnly=false)const{
        std::stringstream ss;
        if(averageOnly){
            ss<<"avg="<<MyTimeHelper::R(getAvg());
            return ss.str();
        }
        ss<<"min="<<MyTimeHelper::R(getMin())<<" max="<<MyTimeHelper::R(getMax())<<" avg="<<MyTimeHelper::R(getAvg());
        return ss.str();
    }
    static AvgCalculator median(const AvgCalculator& c1,const AvgCalculator& c2){
        AvgCalculator ret;
        ret.add(c1.getAvg());
        ret.add(c2.getAvg());
        const auto min=std::min(c1.getMin(),c2.getMin());
        const auto max=std::max(c1.getMax(),c2.getMax());
        ret.min=min;
        ret.max=max;
        return ret;
    }
};


class Chronometer:public AvgCalculator {
public:
    explicit Chronometer(std::string name="Unknown"):mName(std::move(name)){}
    void start(){
        startTS=std::chrono::steady_clock::now();
    }
    void stop(){
        const auto now=std::chrono::steady_clock::now();
        const auto delta=(now-startTS);
        AvgCalculator::add(delta);
    }
    void printInIntervalls(const std::chrono::steady_clock::duration& interval,const bool avgOnly=true) {
        const auto now=std::chrono::steady_clock::now();
        if(now-lastLog>interval){
            lastLog=now;
            MLOGD2(mName)<<"Avg: "<<AvgCalculator::getAvgReadable(avgOnly);
            reset();
        }
    }
private:
    const std::string mName;
    std::chrono::steady_clock::time_point startTS;
    std::chrono::steady_clock::time_point lastLog;
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
#include <thread>
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