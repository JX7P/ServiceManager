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

#include <cstdarg>
#include <list>
#include <string>

class Logger
{
    static std::list<Logger> loggers;

    std::string subsys;

  public:
    enum LogLevel
    {
        kDebug,
        kInfo,
        kWarn,
        kErr
    };

  protected:
    /* Print date, log level, subsystem name, and optional extra string. */
    void printPrefix(LogLevel level, const char *optExtra = NULL);

  public:
    /**
     * Create a new logger for the given subsystem name. If \param parent is
     * specified, append subsystem name to parent's subsystem name.
     */
    Logger(const char *subsysName, Logger *parent = NULL);

    /* Log a line with an optional extra string. */
    void logi(LogLevel level, const char *extra, const char *fmt, va_list args);
    /* Log a line. */
    void log(LogLevel level, const char *fmt, ...);
    /* Log a line, appending a colon followed by (": %s\n", strerror(eNo)). */
    void loge(LogLevel level, int eNo, const char *fmt, ...);
    /* Log a line with an optional extra string. */
    void ilog(LogLevel level, const char *extra, const char *fmt, ...);
};