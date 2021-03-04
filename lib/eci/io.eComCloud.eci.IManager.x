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
/**
 * The eComInit daemon manager interface.
 */

%#include <string>

%#include "eci/ECI.hh"

program io.eComCloud.eci.IManager
{
	version manager1
	{
		/**
		 * Subscribe to state-change notifications in line with the ISubscriber
		 * protocol.
		 */
		void subscribe() = 0;
	} = 1;
} = 0x40DD1001;
