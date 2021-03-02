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

#ifndef BACKEND_HH__
#define BACKEND_HH__

#include "eci/Logger.hh"

class Manager;
struct sqlite3;

class Backend : public Logger
{
    friend class Manager;

    sqlite3 *connPersistent;
    sqlite3 *connVolatile;

    Manager *mgr;

    const char *pathPersistentDb;
    /**
     * Path to the volatile repository. If this is NULL, we simply keep the
     * volatile database in-memory only. Otherwise, we either open the volatile
     * database from the get-go, if we aren't starting in read-only mode, and as
     * soon as we're in read-write mode, we write it out to disk.
     */
    const char *pathVolatileDb = NULL;

    bool readOnly = false;

  public:
    Backend(Manager *mgr);

    /**
     * Initialise the database backend.
     *
     * @param aPathPersistentDb Path to the persistent repository. Must be
     * specified.
     * @param aPathVolatileDb Nullab.e Path to the volatile repository. If NUL,
     * the volatile database is kept only in-memory. If \p startReadOnly is true
     * then the volatile database is kept in-memory regardless of this
     * parameter, unless and until we exit read-only mode.
     * @param recreatePersistentDb Whether to delete any existing database at /p
     * aPathPersistentDb and create a new one. Useful to prepare a seed
     * repository. Of dubious import when combined with \p startReadOnly.
     * @param startReadOnly Whether to start in read-only mode.
     * @param reattachVolatileRepository Whether to reattach to an existing
     * volatile repository.
     */
    void init(const char *aPathPersistentDb, const char *aPathVolatileDb,
              bool startReadOnly, bool recreatePersistentDb,
              bool reattachVolatileRepository);
};

#endif