/*******************************************************************

    PROPRIETARY NOTICE

These coded instructions, statements, and computer programs contain
proprietary information of eComCloud Object Solutions, and they are
protected under copyright law. They may not be distributed, copied,
or used except under the provisions of the terms of the Source Code
Licence Agreement, in the file "LICENCE.md", which should have been
included with this software

    Copyright Notice

    (c) 2021 eComCloud Object Solutions.
        All rights reserved.
********************************************************************/

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <unistd.h>

#include "Backend.hh"
#include "Manager.hh"
#include "eci/Core.h"
#include "repositorySchema.sql.h"
#include "sqlite3.h"
#include "volatileRepositorySchema.sql.h"

#define Str(s) #s

struct ExecGetSingleIntContext
{
    int value;
    int result;
};

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

int sqlite3_execf(sqlite3 *conn, /* An open database */
                  int (*callBack)(void *, int, char **,
                                  char **), /* Callback function */
                  void *arg,                /* 1st argument to callback */
                  char **errMsg,            /* Error msg written here */
                  const char *fmtSql, /*  SQL format string to be evaluated */
                  ...                 /* Variadic arguments */
)
{
    char *buf = NULL;
    int res;
    va_list args;

    va_start(args, fmtSql);

    if (eciVASPrintF(&buf, fmtSql, args) == -1)
        return SQLITE_NOMEM;
    res = sqlite3_exec(conn, buf, callBack, arg, errMsg);

    free(buf);
    va_end(args);
    return res;
}

static int cbExecGetSingleInt(void *udp, int nCols, char **colVals,
                              char **colNames)
{
    ExecGetSingleIntContext *ctx = (ExecGetSingleIntContext *)udp;
    uint32_t val;
    char *endPtr = colVals[0];

    assert(nCols == 1);

    if (colVals[0] == NULL)
        return 0;

    errno = 0;
    ctx->value = strtol(colVals[0], &endPtr, 10);
    if (colVals[0] == endPtr)
        goto bad;
    else if (errno != 0)
        goto bad;

    ctx->result = SQLITE_OK;
    return 0;

bad:
    ctx->result = EBADF;
    return -1;
}

Backend::Backend(Manager *mgr) : mgr(mgr), Logger("db-backend", mgr)
{
    log(kInfo, "repository server backed by SQLite version %s\n",
        sqlite3_version);
}

int Backend::metadataInit(sqlite3 *conn)
{
    return sqlite3_exec(conn,
                        "INSERT INTO \"METADATA\""
                        "VALUES (" MStr(ECI_BACKEND_SCHEMA_VERSION) ");",
                        NULL, NULL, NULL);
}

int Backend::metadataValidate(sqlite3 *conn)
{
    ExecGetSingleIntContext ctx;
    int res;

    res = sqlite3_exec(conn, "SELECT Version FROM Metadata;",
                       cbExecGetSingleInt, &ctx, NULL);
    if (res != SQLITE_OK)
    {
        log(kErr, "Failed to get version from the metadata table: %s\n",
            sqlite3_errmsg(conn));
        return -1;
    }

    if (ctx.result != SQLITE_OK)
    {
        loge(kErr, ctx.result, "Failed to get version from the metadata table");
        return -1;
    }

    return ctx.value;
}

int Backend::metadataSetVersion(sqlite3 *conn)
{
}

int Backend::repositoryInit(sqlite3 *conn, const char *schema)
{
    int res = sqlite3_exec(conn, schema, NULL, NULL, NULL);
    if (res != SQLITE_OK)
        return res;

    res = metadataInit(conn);
    return res;
}

void Backend::init(const char *aPathPersistentDb, const char *aPathVolatileDb,
                   bool startReadOnly, bool recreatePersistentDb,
                   bool reattachVolatileRepository)
{
    pathPersistentDb = aPathPersistentDb;
    pathVolatileDb = aPathVolatileDb;
    readOnly = startReadOnly;

    /* setup persistent repository */
    if (recreatePersistentDb)
    {
        int res;

        log(kInfo, "(re)creating persistent reposistory %s\n",
            pathPersistentDb);

        if (unlink(pathPersistentDb) == -1 && errno != ENOENT)
            edie(errno, "Failed to delete old persistent repository");

        res = sqlite3_open_v2(pathPersistentDb, &connPersistent,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);
        if (res != SQLITE_OK)
            die("Failed to create persistent repository: %s",
                sqlite3_errmsg(connPersistent));

        if (repositoryInit(connPersistent, krepositorySchema_sql) != SQLITE_OK)
            die("Failed to initialise persistent repository: %s\n",
                sqlite3_errmsg(connPersistent));
    }
    else
    {
        int res;

        log(kDebug, "attaching to existing persistent repository %s as %s\n",
            pathPersistentDb, readOnly ? "read-only" : "read-write");

        res = sqlite3_open_v2(pathPersistentDb, &connPersistent,
                              (readOnly ? SQLITE_OPEN_READONLY
                                        : SQLITE_OPEN_READWRITE) |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to attach persistent repository: %s\n",
                sqlite3_errmsg(connPersistent));

        res = metadataValidate(connPersistent);
        if (res == -1)
            die("Persistent repository has invalid metadata.\n");
        else if (res != ECI_BACKEND_SCHEMA_VERSION)
            die("Persistent repository has mismatched schema version.\n");
    }

    /* setup volatile repository */
    if (reattachVolatileRepository) /* must also be in read-write mode */
    {
        int res;

        log(kInfo, "reattaching to existing volatile repository %s\n",
            pathVolatileDb);

        res = sqlite3_open_v2(pathVolatileDb, &connVolatile,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to attach to volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));

        res = metadataValidate(connVolatile);
        if (res == -1)
            die("Volatile repository has invalid metadata.\n");
        else if (res != ECI_BACKEND_SCHEMA_VERSION)
            die("Volatile repository has mismatched schema version.\n");
    }
    else if (startReadOnly || !pathVolatileDb)
    {
        int res;
        /* checking if pathVolatileDb should've been set isn't our duty */
        log(kInfo, "creating in-memory volatile repository\n");

        res = sqlite3_open_v2(":memory:", &connVolatile,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to create in-memory volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));

        if (repositoryInit(connVolatile, kvolatileRepositorySchema_sql) !=
            SQLITE_OK)
            die("Failed to initialise in-memory volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));
    }
    else
    {
        int res;

        log(kInfo, "(re)creating volatile repository %s\n", pathVolatileDb);

        if (unlink(pathVolatileDb) == -1 && errno != ENOENT)
            edie(errno, "Failed to delete old volatile repository");

        res = sqlite3_open_v2(pathVolatileDb, &connVolatile,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to create volatile repository: %s",
                sqlite3_errmsg(connVolatile));

        if (repositoryInit(connVolatile, kvolatileRepositorySchema_sql) !=
            SQLITE_OK)
            die("Failed to initialise volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));
    }
}