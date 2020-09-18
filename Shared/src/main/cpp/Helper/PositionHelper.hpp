//
// Created by Constantin on 17.02.2018.
//
#ifndef POSITION_HELPER
#define POSITION_HELPER


#include <cmath>

//taken from tinygps: https://github.com/mikalhart/TinyGPS/blob/master/TinyGPS.cpp#L296
//changed slightly to use double precision
static const double distance_between(double lat1, double long1, double lat2, double long2) {
    // returns distance in meters between two positions, both specified
    // as signed decimal-degrees latitude and longitude. Uses great-circle
    // distance computation for hypothetical sphere of radius 6372795 meters.
    // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
    // Courtesy of Maarten Lamers
    double delta = (long1-long2)*0.017453292519;
    double sdlong = sin(delta);
    double cdlong = cos(delta);
    lat1 = (lat1)*0.017453292519;
    lat2 = (lat2)*0.017453292519;
    double slat1 = sin(lat1);
    double clat1 = cos(lat1);
    double slat2 = sin(lat2);
    double clat2 = cos(lat2);
    delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
    delta = delta*delta;
    delta += (clat2 * sdlong)*(clat2 * sdlong);
    delta = sqrt(delta);
    double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
    delta = atan2(delta, denom);
    return delta * 6372795;
}
//taken from tinygps: https://github.com/mikalhart/TinyGPS/blob/master/TinyGPS.cpp#L321
//changed slightly to use double precision
static const double course_to (double lat1, double long1, double lat2, double long2)
{
    // returns course in degrees (North=0, West=270) from position 1 to position 2,
    // both specified as signed decimal-degrees latitude and longitude.
    // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
    // Courtesy of Maarten Lamers
    double dlon = (long2-long1)*0.017453292519;
    lat1 = (lat1)*0.017453292519;
    lat2 = (lat2)*0.017453292519;
    double a1 = sin(dlon) * cos(lat2);
    double a2 = sin(lat1) * cos(lat2) * cos(dlon);
    a2 = cos(lat1) * sin(lat2) - a2;
    a2 = atan2(a1, a2);
    if (a2 < 0.0)
    {
        a2 += M_PI*2;
    }
    return (180.0/M_PI)*(a2);
}


#endif