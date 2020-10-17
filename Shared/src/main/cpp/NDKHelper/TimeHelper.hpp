//
// Created by geier on 18/01/2020.
//

#ifndef LIVEVIDEO10MS_TIMEHELPER_HPP
#define LIVEVIDEO10MS_TIMEHELPER_HPP

#include "AndroidLogger.hpp"
#include <chrono>
#include <deque>

namespace MyTimeHelper{
    // R stands for readable. Convert a std::chrono::duration into a readable format
    // Readable format is somewhat arbitrary, in this case readable means that for example
    // 1second has 'ms' resolution since for values that big ns resolution probably isn't needed
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
// The type of the samples is for example std::chrono::nanoseconds when measuring time intervalls
template<typename T>
class BaseAvgCalculator{
private:
    // do not forget the braces to initialize with 0
    T sum{};
    long nSamples=0;
    T min=std::numeric_limits<T>::max();
    T max{};
public:
    BaseAvgCalculator() = default;
    void add(const T& value){
        if(value<T(0)){
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
    // Returns the average of all samples.
    // If 0 samples were recorded, return 0
    T getAvg()const{
        if(nSamples == 0)return T(0);
        return sum / nSamples;
    }
    // Returns the minimum value of all samples
    T getMin()const{
        return min;
    }
    // Returns the maximum value of all samples
    T getMax()const{
        return max;
    }
    // Returns the n of samples that were processed
    long getNSamples()const{
        return nSamples;
    }
    // Reset everything (as if zero samples were processed)
    void reset(){
        sum={};
        nSamples=0;
        min=std::numeric_limits<T>::max();
        max={};
    }
    // Merges two AvgCalculator(s) that hold the same types of samples together
    BaseAvgCalculator<T> operator+(const BaseAvgCalculator<T>& other){
        BaseAvgCalculator<T> ret;
        ret.add(this->getAvg());
        ret.add(other.getAvg());
        const auto min1=std::min(this->getMin(),other.getMin());
        const auto max1=std::max(this->getMax(),other.getMax());
        ret.min=min1;
        ret.max=max1;
        return ret;
    }
    // max delta between average and min / max
    std::chrono::nanoseconds getMaxDifferenceMinMaxAvg()const{
        const auto deltaMin=std::chrono::abs(getAvg()-getMin());
        const auto deltaMax=std::chrono::abs(getAvg()-getMax());
        if(deltaMin>deltaMax)return deltaMin;
        return deltaMax;
    }
    std::string getAvgReadable(const bool averageOnly=false)const{
        std::stringstream ss;
        if constexpr (std::is_same_v<T,std::chrono::nanoseconds>){
            // Class stores time samples
            if(averageOnly){
                ss<<"avg="<<MyTimeHelper::R(getAvg());
                return ss.str();
            }
            ss<<"min="<<MyTimeHelper::R(getMin())<<" max="<<MyTimeHelper::R(getMax())<<" avg="<<MyTimeHelper::R(getAvg());
        }else{
            // Class stores other type of samples
            if(averageOnly){
                ss<<"avg="<<getAvg();
                return ss.str();
            }
            ss<<"min="<<getMin()<<" max="<<getMax()<<" avg="<<getAvg();
        }
        return ss.str();
    }
    float getAvg_ms(){
        return (float)(std::chrono::duration_cast<std::chrono::microseconds>(getAvg()).count())/1000.0f;
    }
};
using AvgCalculator=BaseAvgCalculator<std::chrono::nanoseconds>;



// Instead of storing only the min, max and average this stores
// The last n samples in a queue. However, this makes calculating the min/max/avg values much more expensive
// And therefore should only be used with a small sample size.
class AvgCalculator2{
private:
    const size_t sampleSize;
    std::deque<std::chrono::nanoseconds> samples;
public:
    AvgCalculator2(size_t sampleSize=60):sampleSize(sampleSize){};
    //
    void add(const std::chrono::nanoseconds& value){
        if(value<std::chrono::nanoseconds(0)){
            MLOGE<<"Cannot add negative value";
            return;
        }
        samples.push_back(value);
        // Remove the oldest sample if needed
        if(samples.size()>sampleSize){
            samples.pop_front();
        }
    }
    std::chrono::nanoseconds getAvg()const{
        if(samples.empty()){
            return std::chrono::nanoseconds(0);
        }
        std::chrono::nanoseconds sum{0};
        for(const auto sample:samples){
            sum+=sample;
        }
        return sum / samples.size();
    }
    std::chrono::nanoseconds getMin()const{
        return *std::min_element(samples.begin(),samples.end());
    }
    std::chrono::nanoseconds getMax()const{
        return *std::max_element(samples.begin(),samples.end());
    }
    void reset(){
        samples.resize(0);
    }
    std::string getAvgReadable(const bool averageOnly=false)const{
        std::stringstream ss;
        if(averageOnly){
            ss<<"avg="<<MyTimeHelper::R(getAvg());
            return ss.str();
        }
        ss<<"min="<<MyTimeHelper::R(getMin())<<" max="<<MyTimeHelper::R(getMax())<<" avg="<<MyTimeHelper::R(getAvg())<<" N samples="<<samples.size();
        return ss.str();
    }
    size_t getNSamples()const{
        return samples.size();
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
    void reset(){
        sum=0;
        sumAtLastCall=0;
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