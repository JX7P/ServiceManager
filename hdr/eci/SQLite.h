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

#ifndef SQLITE_H_
#define SQLITE_H_

#include "sqlite3.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int sqlite3_execf(sqlite3 *, /* An open database */
                      int (*callback)(void *, int, char **,
                                      char **), /* Callback function */
                      void *,                   /* 1st argument to callback */
                      char **errmsg,            /* Error msg written here */
                      const char *fmt,          /* SQL to be evaluated */
                      ...);

    int sqlite3_prepare_v2f(
        sqlite3 *db,           /* Database handle */
        sqlite3_stmt **ppStmt, /* OUT: Statement handle */
        const char **pzTail,   /* OUT: Pointer to unused portion of zSql */
        const char *fmt,       /* SQL statement, UTF-8 encoded */
        ...);

    /** Try to get a single int. Returns non-0 on failure. */
    int sqlite3_get_single_int(sqlite3 *conn, int *result, const char *query);
    int sqlite3_get_single_intf(sqlite3 *conn, int *result, const char *fmt,
                                ...);

    /** Try to get two ints. Returns non-0 on failure. */
    int sqlite3_get_two_int(sqlite3 *conn, int *result1, int *result2,
                            const char *query);
    int sqlite3_get_two_intf(sqlite3 *conn, int *result1, int *result2,
                             const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif