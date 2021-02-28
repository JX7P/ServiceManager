/*******************************************************************

    PROPRIETARY NOTICE

These coded instructions, statements, and computer programs contain
proprietary information of eComCloud Object Solutions, and they are
protected under copyright law. They may not be distributed, copied,
or used except under the provisions of the terms of the Source Code
Licence Agreement, in the file "LICENCE.md", which should have been
included with this software

    Copyright Notice

    (c) 2015-2021 eComCloud Object Solutions.
        All rights reserved.
********************************************************************/

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "eci/Logger.hh"

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

Logger::Logger(const char *subsysName, Logger *parent)
{
    if (parent)
        subsys = parent->subsys + subsysName;
    else
        subsys = subsysName;
}

void Logger::printPrefix(LogLevel level, const char *optExtra)
{
    time_t rawtime;
    struct tm timeinfo;
    char time_str[26];
    const char *pfx = "";

    if (level == kErr)
        pfx = KRED "ERROR: " KNRM;
    else if (level == kWarn)
        pfx = KYEL "WARNING: " KNRM;

    time(&rawtime);
    localtime_r(&rawtime, &timeinfo);
    snprintf(time_str, 26, "%.2d:%.2d:%.2d", timeinfo.tm_hour, timeinfo.tm_min,
             timeinfo.tm_sec);

    // asctime_r(&timeinfo, time_str);
    // time_str[strlen(time_str) - 1] = '\0';

    if (optExtra)
    {
        printf(KWHT "[%s] "
                    "%s%s%s" KNRM ": %s",
               time_str, subsys.c_str(), "/" KBLU, optExtra, pfx);
    }
    else
        printf(KWHT "[%s] " KNRM "%s: " KNRM "%s", time_str, subsys.c_str(),
               pfx);
}

void Logger::logi(LogLevel level, const char *extra, const char *fmt,
                  va_list args)
{
    printPrefix(level, extra);
    vprintf(fmt, args);
}

void Logger::log(LogLevel level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printPrefix(level);
    vprintf(fmt, args);
    va_end(args);
}

/* Log a line, appending a colon followed by strerror(eNo). */
void Logger::loge(LogLevel level, int eNo, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printPrefix(level);
    vprintf(fmt, args);
    printf(": %s\n", strerror(eNo));
    va_end(args);
}

void Logger::ilog(LogLevel level, const char *extra, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logi(level, extra, fmt, args);
    va_end(args);
}