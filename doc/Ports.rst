Ports
=====


eComInit is developed on FreeBSD, an extensively supported, advanced,
open systems compliant operating system, which is available as freeware.
We believe that the sector-leading quality of engineering offered in FreeBSD
makes it the perfect host for eComInit.

A number of other platforms are also supported. Platforms may differ in
feature-set. The Support Matrix is below: it details the minimum supported
version of each platform and the features supported for it. An explanation of
the features follows thereafter.

.. list-table:: Support Matrix
   :header-rows: 1

   * - Platform
     - Min. ver.
     - SD-Notify
     - Fork-tracking
   * - FreeBSD
     - 13.0
     - Datagram
     - Yes
   * - GNU/Linux
     - 4.0
     - Datagram
     - Yes
   * - NetBSD
     - 8.0
     - Datagram
     - Yes
   * - DragonFly BSD
     - 5.6
     - Datagram
     - Yes
   * - OpenBSD
     - 6.6
     - Stream
     - Yes
   * - Mac OS X
     - 10.4
     - Stream?
     - Yes
   * - HP-UX
     - 11iv1 w/ Gold Quality Pack
     - Insecure
     - No

SD-Notify
---------

This is the kind of SD-Notify interface supported. The three kinds are:
 - Datagram: Uses a datagram socket with credential passing. Binary compatible
   with systemd.
 - Stream: Uses a stream socket with credential passing.
 - Insecure: Uses a stream socket without credential passing. Insecure as the
   sender reports their own PID, and this could be falsified.

*Datagram* is binary-compatible with systemd's interface, while *Stream* and
*Insecure* are source compatible with systemd's interface systemd *only if the
provided sd_notify(3) function is used*.

Fork-tracking
-------------

Whether the restarters using *libeci-delegate*'s process tracking functionality
can track a process that tries to escape supervision by double-forking.