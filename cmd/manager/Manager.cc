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

#include "Manager.hh"
#include "eci/Event.hh"

Manager gMgr;

void Manager::die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logi(kErr, NULL, fmt, args);
    exit(EXIT_FAILURE);
    va_end(args);
}

void Manager::init(int argc, char *argv[])
{
    int r = 0;
    char c;

#define SetIf(condition)                                                       \
    if (condition == -1)                                                       \
        r = -1;

    SetIf(loop.init());

    SetIf(loop.addSignal(this, SIGHUP));
    SetIf(loop.addSignal(this, SIGTERM));

    if (r == -1)
        die("Failed to initialise runloop.\n");

    /**
     * -p <path>: permanent db path
     * -n <path>: volatile db path
     * -r: try reattachment (if not, any volatile DB at given path is deleted
     * and recreated)
     * -o: start in read-only mode (wait for sys:/runlevel/)
     */

    while ((c = getopt(argc, argv, "p:n:r")) != -1)
        switch (c)
        {
        case p:
            persistentDbPath = optarg;
            break;
        case n:
            volatileDbPath = optarg;
            break;
        case 'r':
            reattach = true;
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return 1;
        default:
            abort();
        }
}

void Manager::run()
{
}

int main(int argc, char *argv[])
{
    gMgr.init(argc, argv);
    gMgr.run();
}