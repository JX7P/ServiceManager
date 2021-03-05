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

#include "Manager.hh"

bool Manager::subscribe_v1(WSRPCReq *req, std::string *rval, int hello)
{
    printf("Subscribe request: %d\n", hello);
    *rval = "Hello";
    return true;
}
