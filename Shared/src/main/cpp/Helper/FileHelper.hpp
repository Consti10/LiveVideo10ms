//
// Created by geier on 25/03/2020.
//

#ifndef LIVEVIDEO10MS_FILEHELPER_HPP
#define LIVEVIDEO10MS_FILEHELPER_HPP

#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
//#include <filesystem>
#include <cassert>

namespace FileHelper{
    static std::string findUnusedFilename(std::string directory,std::string filetype) {
        //std::filesystem::create_directory(directory);
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream filenameShort;
        //first,try with date,hours and minutes only
        filenameShort<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M")<<"."<<filetype;
        std::ifstream infile(filenameShort.str());
        if(!infile.good()){
            return filenameShort.str();
        }
        //else, also use seconds and assume this one is valid
        std::stringstream filenameLong;
        filenameLong<<directory<<std::put_time(&tm, "%d-%m-%Y %H-%M-%S")<<"."<<filetype;
        return filenameLong.str();
    }
    static bool endsWith(const std::string& str, const std::string& suffix){
            return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
    }
    static std::string changeFileContainerFPVtoMP4(std::string inputFPV){
        assert(endsWith(inputFPV,".fpv"));
        const std::string originalContainerName=".fpv";
        const std::string wantedContainerName=".mp4";
        inputFPV.resize(inputFPV.length()-originalContainerName.length());
        return inputFPV+wantedContainerName;
    }
}
#endif //LIVEVIDEO10MS_FILEHELPER_HPP
