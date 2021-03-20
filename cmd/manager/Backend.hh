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
class InstanceName;
struct sqlite3;
struct sqlite3_stmt;

class Backend : public Logger
{
    friend class Manager;

    Manager *mgr;

    sqlite3 *connPersistent;
    sqlite3 *connVolatile;

    /**
     * Prepared statement to select the latest composed set of properties from
     * an instance ID in the persistent repository.
     *
     * Use sqlite_bind_int(permNstCurProps, 1, $nstID) to
     * set the instance ID to retrieve properties therefrom.
     */
    sqlite3_stmt *permNstCurProps;

    /**
     * Path to the persistent repository - we need it so that, should we
     * transition from or to read-only mode, we can reopen the repository
     * connection.
     */
    const char *pathPersistentDb;
    /**
     * Path to the volatile repository. If this is NULL, we simply keep the
     * volatile database in-memory only. Otherwise, we either open the volatile
     * database from the get-go, if we aren't starting in read-only mode, and as
     * soon as we're in read-write mode, we write it out to disk.
     */
    const char *pathVolatileDb = NULL;

    bool readOnly = false;

    /** Initialises the metadata table. The table must be empty. -1 on fail. */
    int metadataInit(sqlite3 *conn);
    /** Validates the Metadata table, returning the version. -1 on fail. */
    int metadataValidate(sqlite3 *conn);
    /** Set the schema version in the Metadata table. -1 on fail. */
    int metadataSetVersion(sqlite3 *conn);

    /**
     * Find the given instance in the persistent database.
     *
     * @param instName (absolute) instance name to look up.
     *
     * @returns instance ID (>0) if found
     * @returns 0 if not found
     */
    int persistentInstanceLookup(InstanceName &name);

    /**
     * Make a snapshot of the given instance's current composed view of
     * properties.
     *
     * The properties are recursively copied; to avoid duplication of data, the
     * PropertyValues to which they point are not copied; changes in
     * configuration trigger new PropertyValues to be created anyway.
     *
     * If a snapshot of the given name already exists for the instance, then it
     * is deleted outright and replaced.
     *
     * @returns snapshot ID (>0) if successful
     * @returns -ENOENT if instance does not exist
     */
    int persistentInstanceSnapshotCreate(int instanceID, const char *name);

    /** Initialise a new repository with the given schema. -1 on fail. */
    int repositoryInit(sqlite3 *conn, const char *schema);

    /* Print an SQLite error. */
    static void sqliteLog(void *userData, int errCode, const char *errMsg);

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

    /** Shut down the database backend. */
    void shutdown();
};

#endif