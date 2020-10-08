//
// Created by geier on 14/09/2020.
//

#ifndef LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H
#define LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H

#include <string>

// Helper that contains a general telemetry value in a readable format
class MTelemetryValue{
public:
    std::wstring prefix=std::wstring();
    std::wstring prefixIcon=std::wstring();
    float prefixScale=0.83f;
    std::wstring value=L"";
    double valueNotAsString;
    std::wstring metric=L"";
    int warning=0; //0==okay 1==orange 2==red and -1==green
    size_t getLength()const{
        return prefix.length()+value.length()+metric.length();
    }
    // Not every telemetry value has an Icon. If no icon was generated yet, a simple string
    // is used instead (example 'Batt' instead of Battery icon)
    bool hasIcon()const{
        return (!prefixIcon.empty());
    }
    std::wstring getPrefix()const{
        return hasIcon() ? prefixIcon : prefix;
    }
};

#endif //LIVEVIDEO10MS_BASETELEMETRYRECEIVER_H
