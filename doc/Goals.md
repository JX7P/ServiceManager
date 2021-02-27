Project Goals for eComInit
==========================

Detailed herein are the project goals for eComInit.

The Four Motives
----------------

These are the guidelines for all eComCloud software:

1. **Adaptable**. We design software for flexibility. As far as practically
  possible, it is workable for almost all use cases. Our software is portable.

2. **Modular**. We decompose our software into multiple components in line with
  software engineering best-practice. These parts communicate with each other
  over clearly-defined interfaces, so they can be replaced readily.

3. **Reliable**. Before declaring a product stable, we ensure it is resilient
  against crashes to the greatest possible degree. In particular, we are careful
  to validate user input.

4. **Informative**. The user does not have to guess at why something is
  happening; as far as possible, they should be able to predict what will
  happen, and where external factors make that unachievable, clear explanations
  are readily provided.

Project Goals
-------------

These are the project-specific goals of eComInit.

1. **Portable**. Specifically to *FreeBSD*, *NetBSD*, *OpenBSD*, *DragonFlyBSD*,
  *HP-UX*, and *GNU/Linux*.

2. **Declarative**. As much as possible, eComInit components should carry out
  tasks based on a description of the end result sought, and let the details be
  left for it to work out, rather than compelling the user to concern themselves
  with trifles.

3. **Similar power to systemd**. Anything that is possible with systemd as one's
 init system should, within reason, be possible with eComInit as one's init
 system. 

    It does not follow that eComInit copies all features from systemd - it simply
 means that we must be flexible enough to permit most of them to be implemented
 at some level of the stack.

    The majority of features found in systemd-the-pid1 will be implemented across
 the core daemons, while other things will be reserved to the eComInit Subsystem
 for Systemd Applications.
