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

#ifndef ECI_MANAGER_HH
#define ECI_MANAGER_HH

#include <list>

#include "Backend.hh"
#include "eci/Event.hh"

class Object
{
    /**
     * If B sets a propagation flag for its dependency on A, then the
     * following happens: If B
     */
    enum PropagationFlags
    {
        /* If something bad happens (error in running a method, for
         * example) then
         */
        kPropagateBad = 0x1,
        kPropagateAny
    };

    /**
     * These are the enqueuers. They simply cause an appropriate job to be
     * enqueued.
     */
    std::list<Object *> start;
    std::list<Object *> verifyOnline;
    std::list<Object *> stop;

    /* These are the ordering directives. */
    std::list<Object *> after;
    std::list<Object *> before;

    /* These are the event subscriptions. When the cited object emits the
     * given event, then this object will carry out the given action. */
};

struct Task
{
    enum State
    {
        /** Action not yet dispatched, likely waiting for a dependent
           action. */
        kWaiting,
        /** Action is currently running. */
        kRunning,
        /** Action successfully completed. */
        kComplete,
        /** Action failed. */
        kFailed
    };

    enum Kind
    {
        kStart,
        /* Halt this */
        kStop,
    };

    /** The object of this transaction */
    Object *obj;
    /** State of the transaction */
    State state;
};

struct Transaction
{
};

class Manager : public Handler, public Logger
{
    EventLoop loop;
    Backend bend;

    /**
     * Whether we are trying to reattach to an extant session. This would
     * involve trying to attach to an existing volatile repository on-disk,
     * re-establishing connections with delegates, etc. Otherwise we simply
     * delete the volatile repository and create a new one.
     */
    bool reattaching = false;

    /**
     * Whether we are in System mode. System mode is a two-step affair:
     *
     * - we first boot (with backend started in read-only mode) to
     * runlevel$early-init, which will typically bring the root filesystem into
     * read/write mode and run a manifest import. Thus newly-added services may
     * be added to the repository and only very basic system initialisation
     * services will not be updated.
     *
     * - only after runlevel$early-init has come up do we then target
     * runlevel$default.
     */
    bool systemMode = false;

    /** Initialise the backend. */
    void backendInit();

  public:
    Manager() : Logger("mgr"), bend(this){};

    void init(int argc, char *argv[]);
    void run();
};

extern Manager gMgr;

#endif