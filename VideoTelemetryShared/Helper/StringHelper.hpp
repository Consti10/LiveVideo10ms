//
// Created by Constantin on 09.10.2017.
//

#ifndef OSDTESTER_STRINGHELPER_H
#define OSDTESTER_STRINGHELPER_H

#include <string>
#include <sstream>
#include "AndroidLogger.hpp"

//Really messy spaghetti code

class StringHelper{
private:
    static constexpr auto TAG="StringHelper";
    static const int countDigitsFast(long n){
        if(n==std::numeric_limits<long>::min()){
            return 11;
        }
        if(n<0){
            return countDigitsFast(-n)+1;
        }
        if(n==0){
            return 1;
        }
        return (int)std::log10(n)+1;
    }
    static const int countDigitsSlow(long n){
        return (int)(std::stringstream()<<n).str().length();
    }
public:
    static const std::string normalS(std::wstring& input){
        return std::string(input.begin(),input.end());
    }
    static const std::wstring intToString(const int value,const int maxStringL){
        const auto asString=(std::wstringstream()<<value).str();
        if(asString.length()>maxStringL){
            return L"E";
        }
        return asString;
    }

//writes the fractional and non-fractional parts of value into input wstring(s)
//maxLength: Maximum length of the string,including '-' and '.'
//resAfterCome: The maximum resolution after the come
    static const std::wstring doubleToString(double value,int maxLength,int maxResAfterCome){
        const int maxPrecision=maxLength-countDigitsFast((long)std::abs(value))-1-(value<0 ? 1 : 0);
        const int precision=maxPrecision>maxResAfterCome ? maxResAfterCome : maxPrecision;
        std::wstring result;
        if(precision>=0){
            result=(std::wstringstream()<<std::fixed<<std::setprecision(precision)<<value).str();
        }else{
            result=L"E";
        }
        //if(result.compare(L"-0")!=0){
        //    result=L"0";
        //}
        //LOGSH("val:%f maxLen:%d maxResAfterCome:%d maxPrecision:%d precision:%d result:%s",value,maxLength,maxResAfterCome,maxPrecision,precision,normalS(result).c_str());
        if(result.length()>maxLength){
            LOGE(TAG)<<"ERROR max length";
            result=L"EM";
        }
        return result;
    }


    static const void doubleToString(std::wstring& sBeforeCome,std::wstring& sAfterCome,double value,int maxLength,int maxResAfterCome){
        const auto valueAsString=doubleToString(value,maxLength,maxResAfterCome);
        const auto idx=valueAsString.find(L".");
        const auto nonFractional=idx==std::wstring::npos ? valueAsString : valueAsString.substr(0,idx);
        const auto fractional=idx==std::wstring::npos ? L"" : valueAsString.substr(idx,valueAsString.length());
        sBeforeCome=nonFractional;
        sAfterCome=fractional;
    }

    static const void test1(){
        doubleToString(10.9234,10,4);
        doubleToString(10.9234,10,2);
        doubleToString(10.9234,10,0);
        doubleToString(10.9234,4,4);
        doubleToString(-10.9234,4,4);
    }

    static const void testCountDigits(){
        //
        std::srand(std::time(nullptr));
        int logc=0;
        LOGD(TAG)<<"testCountDigits() start";
        for(long i=-std::numeric_limits<int>::min();i<std::numeric_limits<int>::max();i+=10000){
        //for(long i=-10000;i<10000;i+=1){
        //for(long i=-std::numeric_limits<long>::max();i>=std::numeric_limits<long>::min();i-=100000){
            int len1=countDigitsFast(i);
            int len2=countDigitsSlow(i);
            if(len1!=len2){
                LOGE(TAG)<<"ERROR "<<i<<" "<<len1;
            }
            //i+=std::rand()%100000;
            logc++;
            if(logc>100){
                LOGD(TAG)<<"%ld"<<i;
            }
        }
        LOGD(TAG)<<"testCountDigits() end";
    }



};



//static const void doubleToString(std::wstring& sBeforeCome,std::wstring& sAfterCome,double value,int maxLength,int resAfterCome){
//    lol(sBeforeCome,sAfterCome,value,5,2);
//
//
//    LOGSH("Number:%f | %s %s",value,(std::string(sBeforeCome.begin(),sBeforeCome.end()).c_str()),(std::string(sAfterCome.begin(),sAfterCome.end()).c_str()));
//
//
//    //number before come,as long
//    const bool positive=value>0;
//    const long nonFractional=(long)value;
//    const double fractional=std::abs(std::fmod(value,1));
//
//    LOGSH("Number:%f, positive:%d nonFractional:%ld fractional:%f",value,positive,nonFractional,fractional);
//
//    std::wstringstream ss;
//    ss<<nonFractional;
//
//    //If we already have to many chars, we can only return an error
//    const auto stringLengthBeforeCome=ss.str().length();
//    if(stringLengthBeforeCome>maxLength){
//        sBeforeCome=L"E";
//        sAfterCome=L"E";
//        return;
//    }
//    sBeforeCome=ss.str();
//    //e.g. return 10 instead of 10.
//    if(stringLengthBeforeCome+1>=maxLength || resAfterCome==0){
//        sAfterCome=L"";
//        return;
//    }
//    //calculate how many chars we can append after the come
//    const auto charsLeft=(int)(maxLength-(stringLengthBeforeCome+1));
//    if(resAfterCome>charsLeft){
//        resAfterCome=charsLeft;
//    }
//    std::wstringstream ss2;
//    ss2<<".";
//    long t=(long)(fractional*(std::pow(10,resAfterCome)));
//    ss2<<t;
//    //LOGV3("Number:%f, nonFractional:%d fractional:%f str: %s",value,nonFractional,fractional,ss.str().c_str());
//    if(ss2.str().length()>maxLength){
//        sBeforeCome=L"E2";
//        sAfterCome=L"E2";
//        return;
//    }
//    sAfterCome=ss2.str();
//}
//LOGSH("B %f",value);
//    const std::string nonFractionalS=(std::stringstream()<<nonFractional).str();
//    //first, calculate how much space is left for the fractional part after
//    //we wrote the non-fractional part
//    const int nonFractionalPartLen=(int)nonFractionalPart.length();
//    //check if we don't already exceed the maximum length. If so, we return 'E' for error
//    if(nonFractionalPartLen>maxLength){
//        sBeforeCome=L"E";
//        sAfterCome=L"E";
//        return;
//    }
//    sBeforeCome=std::wstring(nonFractionalPart.begin(),nonFractionalPart.end());
//    //don't forget to add one length for the come
//    const int lengthLeft=maxLength-(nonFractionalPartLen+1);
//    //return with an empty afterCome string if we cannot write any more characters
//    if(lengthLeft<=0){
//        sAfterCome=L"";
//        return;
//    }
//
//
//    LOGSH("B %s",nonFractionalPart.c_str());
//
//    //write value into a stringstream
//    std::stringstream ss;
//    ss<<std::fixed<<std::setprecision(1)<<value;
//
//    //If the value has a fractional part
//    //if we exceed the maximum length, we remove chars from the end until we don't have too much
//    //anymore
//    const int excessChars=maxLength-((int)ss.str().length());
//    if(excessChars>0){
//
//    }



/////////////
//const bool positive=value>0;
//    const long nonFractional=(long)value;
//    const double fractional=std::abs(std::fmod(value,1));
//
//    const int digitsNonFractional=countDigits(nonFractional);
//    if(digitsNonFractional>maxLength){
//        sBeforeCome=L"E";
//        sAfterCome=L"E";
//        return;
//    }
//    //-1 because we have to add the . between no-fractional/fractional parts
//    sBeforeCome=(std::wstringstream()<<(positive ? L"":L"-")<<nonFractional).str();
//    const int charsLeft=maxLength-digitsNonFractional-1;
//    if(charsLeft<=0){
//        sAfterCome=L"";
//        return;
//    }
//    const auto tmp=(std::wstringstream()<<std::fixed<<std::setprecision(resAfterCome)<<fractional).str();
//    //remove the '0.' part
//    const auto tmp2=tmp.substr(2,tmp.length());
//    sAfterCome=tmp2;
//
//    LOGSH("Number:%f nonFractional:%ld Fractional:%f || %s %s",value,nonFractional,fractional,sBeforeCome.c_str(),sAfterCome.c_str());


//static const double shiftMy(double value){
//        const double fractional=std::abs(std::fmod(value,1));
//        if(fractional>0){
//            return shiftMy(value*10);
//        }
//        return value;
//    }

//static const void doubleToString(std::wstring& sBeforeCome,std::wstring& sAfterCome,const double value,const int maxLength,const int resAfterCome){
//        const auto valueAsString=(std::wstringstream()<<std::fixed<<std::setprecision(resAfterCome)<<value).str();
//        //find the '.' if available to seperate fractional and non-fractional parts
//        const auto idx=valueAsString.find(L".");
//        const auto nonFractional=idx==std::wstring::npos ? valueAsString : valueAsString.substr(0,idx);
//        const auto fractional=idx==std::wstring::npos ? L"" : valueAsString.substr(idx,valueAsString.length());
//        sBeforeCome=nonFractional;
//        sAfterCome=fractional;
//        LOGSH("%s %s",normalS(sBeforeCome).c_str(),normalS(sAfterCome).c_str());
//        if(nonFractional.length()>maxLength){
//            sBeforeCome=L"E";
//            sAfterCome=L"E";
//            return;
//        }
//        if(nonFractional.length()+1>=maxLength){
//            //We have to return the nonFractional part only
//            sBeforeCome=nonFractional;
//            sAfterCome=L"";
//            return;
//        }
//        const auto valuesToRemove=valueAsString.length()-maxLength;
//        const auto tmp=fractional.substr(0,fractional.length()-valuesToRemove);
//        sBeforeCome=nonFractional;
//        sAfterCome=tmp;
//    }
//
//    static const void test1(){
//        //std::numeric_limits<double>::min()
//        LOGSH("HXX");
//        for(double val=-1000.0;val<=1000.0;val+=0.0001){
//            std::wstring w1,w2;
//            std::srand(std::time(nullptr));
//            const int maxLength=std::rand()%10;
//            const int resAfterCome=std::rand()%10;
//            doubleToString(w1,w2,val,maxLength,resAfterCome);
//            std::wstring tmp1=(std::wstringstream()<<std::fixed<<std::setprecision(resAfterCome)<<val).str();
//            std::wstring tmp2=w1.append(w2);
//            if(tmp1.compare(tmp2)!=0 && tmp2.length()<maxLength){
//                LOGSH("NOT OK %s %s",normalS(tmp1).c_str(),normalS(tmp2).c_str());
//                LOGSH("%d %d",maxLength,resAfterCome);
//            }else{
//                LOGSH("OK");
//            }
//        }
//    }
#endif //OSDTESTER_STRINGHELPER_H
