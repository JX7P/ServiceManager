#ifndef ECI_MANAGER_HH
#define ECI_MANAGER_HH

#include <list>

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

class Manager
{
  public:
    void init();
    void run();
};

extern Manager gMgr;

#endif