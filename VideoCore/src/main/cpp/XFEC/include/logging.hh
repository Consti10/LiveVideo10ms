
#pragma once

#ifdef __ANDROID__

// Using AndroidLogger instead is straight forward
#include <AndroidLogger.hpp>
#define LOG_DEBUG MLOGD
#define LOG_ERROR MLOGE
#define LOG_INFO MLOGD

#endif

#ifndef __ANDROID__

#include <cctype>
#include <algorithm>

#include <log4cpp/Category.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/SyslogAppender.hh>

#ifdef LOG_CRITICAL
#undef LOG_CRITICAL
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif
#ifdef LOG_WARNING
#undef LOG_WARNING
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#define LOG_CRITICAL log4cpp::Category::getRoot() << log4cpp::Priority::CRIT
#define LOG_ERROR log4cpp::Category::getRoot() << log4cpp::Priority::ERROR
#define LOG_WARNING log4cpp::Category::getRoot() << log4cpp::Priority::WARN
#define LOG_INFO log4cpp::Category::getRoot() << log4cpp::Priority::INFO
#define LOG_DEBUG log4cpp::Category::getRoot() << log4cpp::Priority::DEBUG

static log4cpp::Priority::PriorityLevel get_log_level(std::string level) {
  std::transform(level.begin(), level.end(), level.begin(),
                 [](unsigned char c){ return std::tolower(c); });  
  if (level == "emergency") {
    return log4cpp::Priority::EMERG;
  } else if (level == "fatal") {
    return log4cpp::Priority::FATAL;
  } else if (level == "alert") {
    return log4cpp::Priority::ALERT;
  } else if (level == "critical") {
    return log4cpp::Priority::CRIT;
  } else if (level == "error") {
    return log4cpp::Priority::ERROR;
  } else if (level == "warn") {
    return log4cpp::Priority::WARN;
  } else if (level == "notice") {
    return log4cpp::Priority::NOTICE;
  } else if (level == "info") {
    return log4cpp::Priority::INFO;
  } else if (level == "debug") {
    return log4cpp::Priority::DEBUG;
  } else {
    return log4cpp::Priority::NOTSET;
  }
}

#endif