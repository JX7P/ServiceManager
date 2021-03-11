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
/**
 * Additions for SQLite
 */

#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "eci/SQLite.h"

static int eciVASPrintF(char **out, const char *fmt, va_list args)
{
    va_list args2;
    ssize_t sizOut;

    va_copy(args2, args);
    sizOut = vsnprintf(NULL, 0, fmt, args2);
    va_end(args2);

    if (!sizOut || sizOut == -1)
        return -ENOMEM;

    *out = (char *)malloc(sizOut + 1);
    if (!*out)
        return -ENOMEM;

    return vsprintf(*out, fmt, args);
}

#define FmtWrapper(fun, ...)                                                   \
    char *query;                                                               \
    int res;                                                                   \
    va_list args;                                                              \
                                                                               \
    va_start(args, fmt);                                                       \
                                                                               \
    if (eciVASPrintF(&query, fmt, args) < 0)                                   \
        return SQLITE_NOMEM;                                                   \
    res = fun(__VA_ARGS__);                                                    \
    free(query);                                                               \
                                                                               \
    va_end(args);                                                              \
    return res

int sqlite3_execf(sqlite3 *conn, int (*callback)(void *, int, char **, char **),
                  void *arg, char **errmsg, const char *fmt, ...)
{
    FmtWrapper(sqlite3_exec, conn, query, callback, arg, errmsg);
}

int sqlite3_get_single_int(sqlite3 *conn, int *result, const char *query)
{
    sqlite3_stmt *stmt;
    int res = sqlite3_prepare_v2(conn, query, -1, &stmt, 0);

    if (res != SQLITE_OK)
        return res;

    res = sqlite3_step(stmt);

    if (res == SQLITE_ROW)
        *result = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return res;
}

int sqlite3_get_single_intf(sqlite3 *conn, int *result, const char *fmt, ...)
{
    char *query;
    int res;
    va_list args;

    va_start(args, fmt);

    if (eciVASPrintF(&query, fmt, args) < 0)
        return SQLITE_NOMEM;
    res = sqlite3_get_single_int(conn, result, query);
    free(query);

    va_end(args);
    return res;
}