//
// Created by Ryan Skelton on 01/10/2020.
//

#ifndef LOG_LOG_H
#define LOG_LOG_H

#include <iostream>
#include <sstream>
#include <ctime>
#include <fstream>
#include <unordered_map>

#define LOG(type, msg) (Log::getInstance()->logger(type, __LINE__, msg, ""))

class Log
{
public:
    enum Type
    {
        WARNING,
        INFO,
        MESSAGE,
        DEBUG,
        ERROR
    };

public:
    ~Log();
    static Log *getInstance();
    Log(const Log &) = delete;
    void operator=(const Log &) = delete;
    void logger(Type, int, std::string, std::string);

private:
    Log() = default;
    static Log *logInstance;

    std::unordered_map<Log::Type, std::string> typeMap = {
        {std::make_pair(Log::WARNING, "WARNING")},
        {std::make_pair(Log::INFO, "INFO")},
        {std::make_pair(Log::MESSAGE, "MESSAGE")},
        {std::make_pair(Log::DEBUG, "DEBUG")},
        {std::make_pair(Log::ERROR, "ERROR")}};

    struct Colours
    {
        const std::string RED = "\033[1;31m";
        const std::string GREEN = "\033[1;92m";
        const std::string YELLOW = "\033[1;93m";
        const std::string BLUE = "\033[1;34m";
        const std::string MAGENTA = "\033[1;95m";
        const std::string CYAN = "\033[1;96m";
        const std::string WHITE = "\033[1;37m";
        const std::string RESET = "\033[1;0m";
    } colour;
};

#endif //LOG_LOG_H
