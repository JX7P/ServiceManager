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
#include "eci/SQLite.h"
#include "eci/queryGetInstancePropertiesComposed.sql.h"
#include "repositorySchema.sql.h"
#include "sqlite3.h"
#include "volatileRepositorySchema.sql.h"

#define Str(s) #s

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
    int res;
    int ver;

    res = sqlite3_get_single_int(conn, &ver, "SELECT Version FROM Metadata;");
    if (res != SQLITE_ROW)
    {
        log(kErr, "Failed to get version from the metadata table: %s\n",
            sqlite3_errmsg(conn));
        return -1;
    }
    return ver;
}

int Backend::metadataSetVersion(sqlite3 *conn)
{
}

int Backend::persistentInstanceLookup(InstanceName &name)
{
    int svcId;
    int instId;
    int res;

    res = sqlite3_get_single_intf(connPersistent, &svcId,
                                  "SELECT ServiceID FROM Services"
                                  "WHERE Type = '%s' AND Name = '%s';",
                                  name.type.c_str(), name.svc.c_str());
    if (res != SQLITE_ROW)
    {
        log(kDebug, "No such service %s$%s\n", name.type.c_str(),
            name.svc.c_str());
        return -ENOENT;
    }

    res = sqlite3_get_single_intf(connPersistent, &instId,
                                  "SELECT InstanceID FROM Instances"
                                  "WHERE FK_Parent_ServiceID = %d"
                                  "AND Name = '%s';",
                                  svcId, name.nst.c_str());
    if (res != SQLITE_ROW)
    {
        log(kDebug, "No such instance %s of service %s$%s\n", name.nst.c_str(),
            name.type.c_str(), name.svc.c_str());
        return -ENOENT;
    }

    return instId;
}

int Backend::persistentInstanceSnapshotCreate(int instanceID, const char *name)
{
    int res = sqlite3_bind_int(permNstCurProps, 1, instanceID);

    if (res != SQLITE_OK)
        die("Failed to bind composed property lookup statement: %s\n",
            sqlite3_errmsg(connPersistent));

    while ((res = sqlite3_step(permNstCurProps)) != SQLITE_DONE)
    {
        if (res == SQLITE_ROW)
        {
            int rowID = sqlite3_column_int(permNstCurProps, 0);
            printf("Row: %d\n", rowID);
        }
    }

    sqlite3_reset(permNstCurProps);

    return 0;
}

int Backend::repositoryInit(sqlite3 *conn, const char *schema)
{
    int res = sqlite3_exec(conn, schema, NULL, NULL, NULL);
    if (res != SQLITE_OK)
        return res;

    res = metadataInit(conn);
    return res;
}

void Backend::sqliteLog(void *userData, int errCode, const char *errMsg)
{
    ((Backend *)userData)->log(kWarn, "SQLite: %s\n", errMsg);
}

void Backend::init(const char *aPathPersistentDb, const char *aPathVolatileDb,
                   bool startReadOnly, bool recreatePersistentDb,
                   bool reattachVolatileRepository)
{
    int res;

    pathPersistentDb = aPathPersistentDb;
    pathVolatileDb = aPathVolatileDb;
    readOnly = startReadOnly;

    sqlite3_config(SQLITE_CONFIG_LOG, sqliteLog, this);

    /* setup persistent repository */
    if (recreatePersistentDb)
    {
        log(kInfo, "(re)creating persistent reposistory %s\n",
            pathPersistentDb);

        if (unlink(pathPersistentDb) == -1 && errno != ENOENT)
            edie(errno, "Failed to delete old persistent repository");

        res = sqlite3_open_v2(pathPersistentDb, &connPersistent,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);
        if (res != SQLITE_OK)
            die("Failed to create persistent repository: %s\n",
                sqlite3_errmsg(connPersistent));

        if (repositoryInit(connPersistent, krepositorySchema_sql) != SQLITE_OK)
            die("Failed to initialise persistent repository: %s\n",
                sqlite3_errmsg(connPersistent));
    }
    else
    {
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

    res = sqlite3_prepare_v2(connPersistent,
                             kqueryGetInstancePropertiesComposed_sql, -1,
                             &permNstCurProps, NULL);
    if (res != SQLITE_OK)
        die("Failed to ready prepared statements: %s",
            sqlite3_errmsg(connPersistent));

    printf("Stmt: %p.\n", permNstCurProps);
}

void Backend::shutdown()
{
    sqlite3_close(connVolatile);
    sqlite3_close(connPersistent);
}