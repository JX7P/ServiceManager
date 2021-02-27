Ports
=====


eComInit is developed on FreeBSD, an extensively supported, advanced,
open systems compliant operating system, which is available as freeware.
We believe that the sector-leading quality of engineering offered in FreeBSD
makes it the perfect host for eComInit.

A number of other platforms are also supported. Platforms may differ in
feature-set. Notes on each port are listed below:

FreeBSD
-------

Fully functional.

NetBSD
------

Fully functional. Subreaping not available - exit events can be relayed from
init(8) instead. <!-- is that necessary? -->

DragonFly BSD
-------------

Believed to be fully functional. Not recently tested.

GNU/Linux
---------

The *ECI-Proc-NetLink* process events connector plugin is used by default on
GNU/Linux for System Manager instances. For User Manager instances, the plain
*ECI-Proc-POSIX* plugin is used, which **cannot track processes that double-fork
to reparent to init**.

A new *ECI-Proc-SystemForwarder* plugin is planned to mitigate this limitation;
this plugin will establish a connection between a User Manager and the System
Manager, with the System Manager then forwarding only those process events which
match the PID of the User Manager instance. **This may lead to undesirable
leakage of events across container boundaries.**


OpenBSD
-------

OpenBSD fails to provide a mechanism to obtain the credentials of the sender of
a message through a datagram socket in the UNIX domain. Only with stream sockets
is authentication possible. Therefore the `sd-notify` interface on OpenBSD is
implemented with a stream socket instead of a datagram socket, which will break
any implementations of the `sd-notify` interface that do not call `sd_notify(3)`
but rather reimplement the logic.

HP-UX
-----

HP-UX fails to provide any means of authenticating peers of a socket in the Unix
domain, therefore the `sd-notify` interface is implemented insecurely, with the
sender allowed to specify their own PID.