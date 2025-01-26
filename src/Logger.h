#pragma once

// 定义日志登记 INFO ERROR FATAL DEBUG

#include "noncopyable.h"
#include <cstdio>
#include <string>

//LOG_INFO("%s %d", arg1, arg2...)
#define LOG_INFO(LogmsgFormat, ...)\
    do \
    { \
        Logger *logger = Logger::instance(); \
        logger->setLogLevel(INFO);\
        char buf[1024] = {0};\
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__);\
        logger->log(buf);\
    }while(0)

#define LOG_ERROR(LogmsgFormat, ...)\
    do \
    { \
        Logger *logger = Logger::instance(); \
        logger->setLogLevel(ERROR);\
        char buf[1024] = {0};\
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__);\
        logger->log(buf);\
    } while(0)

#define LOG_FATAL(LogmsgFormat, ...)\
    do \
    { \
        Logger *logger = Logger::instance(); \
        logger->setLogLevel(FATAL);\
        char buf[1024] = {0};\
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__);\
        logger->log(buf);\
        exit(-1);\
    } while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)\
    do \
    { \
        Logger *logger = Logger::instance(); \
        logger->setLogLevel(DEBUG);\
        char buf[1024] = {0};\
        snprintf(buf, sizeof(buf), LogmsgFormat, ##__VA_ARGS__);\
        logger->log(buf);\
    } while(0)
#else
    #define LOG_DEBUG(LogmsgFormat, ...)
#endif

enum LogLevel {
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};
// 输出一个日志类
class Logger : noncopyable
{
    public:
    // 输出一个日志类
    static Logger *instance();
    // 设置日志等级
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

    private:
    int logLevel_;
    Logger();
};