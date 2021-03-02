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

#include <cstring>
#include <unistd.h>

#include "Backend.hh"
#include "Manager.hh"
#include "sqlite3.h"

Backend::Backend(Manager *mgr) : mgr(mgr), Logger("db-backend", mgr)
{
    log(kInfo, "repository server backed by SQLite version %s\n",
        sqlite3_version);
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

        if (unlink(pathPersistentDb) == -1 && errno != ENOENT)
            edie(errno, "Failed to delete old persistent repository");

        res = sqlite3_open_v2(pathPersistentDb, &connPersistent,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to create persistent repository: %s",
                sqlite3_errmsg(connPersistent));
    }
    else
    {
        int res = sqlite3_open_v2(
            pathPersistentDb, &connPersistent,
            (readOnly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE) |
                SQLITE_OPEN_NOMUTEX,
            NULL);

        if (res != SQLITE_OK)
            die("Failed to attach persistent repository: %s\n",
                sqlite3_errmsg(connPersistent));
    }

    /* setup volatile repository */
    if (reattachVolatileRepository) /* must also be in read-write mode */
    {
        int res =
            sqlite3_open_v2(pathVolatileDb, &connVolatile,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL);

        if (res != SQLITE_OK)
            die("Failed to reattach to volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));
    }
    else if (startReadOnly)
    {
        int res =
            sqlite3_open_v2(":memory:", &connVolatile,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL);

        if (res != SQLITE_OK)
            die("Failed to create in-memory volatile repository: %s\n",
                sqlite3_errmsg(connVolatile));
    }
    else
    {
        int res;

        if (unlink(pathVolatileDb) == -1 && errno != ENOENT)
            edie(errno, "Failed to delete old volatile repository");

        res = sqlite3_open_v2(pathVolatileDb, &connVolatile,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                  SQLITE_OPEN_NOMUTEX,
                              NULL);

        if (res != SQLITE_OK)
            die("Failed to create volatile repository: %s",
                sqlite3_errmsg(connVolatile));
    }
}