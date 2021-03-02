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

#include <csignal>
#include <cstdio>
#include <unistd.h>

#include "Backend.hh"
#include "Manager.hh"
#include "eci/Event.hh"

Manager gMgr;

void Manager::init(int argc, char *argv[])
{
    int r = 0;
    char c;
    bool systemMode = false;
    bool readOnly = false;
    /* backend settings */
    const char *pathPersistentDb = NULL;
    const char *pathVolatileDb = NULL;
    bool recreatePersistentDb = false;

#define SetIf(condition)                                                       \
    if (condition == -1)                                                       \
        r = -1;

    SetIf(loop.init());

    SetIf(loop.addSignal(this, SIGHUP));
    SetIf(loop.addSignal(this, SIGTERM));

    if (r == -1)
        die("Failed to initialise runloop.\n");

    /**
     * -c: delete any existing database at the given persistent DB path, and
     * create a new one instead. Do not try to start any targets. Used to create
     * a seed repository.
     * -o: start ready, only try to go into read-write mode if later requested
     * -p <path>: permanent db path
     * -q <path>: volatile db path
     * -r: try reattachment (if not, any volatile DB at given path is deleted
     * and recreated)
     * -s: system mode (start readonly - try to open read-write later. UNLESS
     * also reattaching - then we assume read-write unless directed otherwise)
     */

    while ((c = getopt(argc, argv, "cp:q:rs")) != -1)
        switch (c)
        {
        case 'c':
            recreatePersistentDb = true;
            break;
        case 'p':
            pathPersistentDb = optarg;
            break;
        case 'q':
            pathVolatileDb = optarg;
            break;
        case 'r':
            reattaching = true;
            break;
        case 's':
            systemMode = true;
            readOnly = true;
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            abort();
        }

    if (!pathPersistentDb)
        die("No path given for persistent repository.\n");
    if (!pathVolatileDb && reattaching)
        die("Asked to reattach, but no path given for volatile repository.\n");
    if (!pathVolatileDb && !readOnly)
        die("No path given for volatile repository.\n");

    if (systemMode)
        readOnly = true;

    bend.init(pathPersistentDb, pathVolatileDb, readOnly, recreatePersistentDb,
              reattaching);
}

void Manager::run()
{
}

int main(int argc, char *argv[])
{
    gMgr.init(argc, argv);
    gMgr.run();
}