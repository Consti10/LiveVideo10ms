//
// Created by Constantin on 1/13/2019.
//

#ifndef FPV_VR_SPSHELPER_H
#define FPV_VR_SPSHELPER_H


#include <cstdint>


class SPSHelper{
private:
    static const uint8_t * m_pStart;
    static int m_nLength;
    static int m_nCurrentBit;
    static unsigned int ReadBit();
    static unsigned int ReadBits(int n);
    static unsigned int ReadExponentialGolombCode();
    static unsigned int ReadSE();
public:
    static void ParseSPS(const uint8_t * pStart, int nLen,int *width,int *height);
};


#endif //FPV_VR_SPSHELPER_H
