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

class Manager : Handler
{
  public:
    void init();
    void run();
};

extern Manager gMgr;

#endif