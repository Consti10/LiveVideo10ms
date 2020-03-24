//
// Created by geier on 18/01/2020.
//

#ifndef LIVEVIDEO10MS_TIMEHELPER_HPP
#define LIVEVIDEO10MS_TIMEHELPER_HPP

class AvgCalculator{
private:
    long sum=0;
    long sumCount=0;
public:
    AvgCalculator() = default;

    void add(long x){
        sum+=x;
        sumCount++;
    }
    long getAvg(){
        if(sumCount==0)return 0;
        return sum/sumCount;
    }
    void reset(){
        sum=0;
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


#endif //LIVEVIDEO10MS_TIMEHELPER_HPP