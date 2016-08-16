/***************************************************************************
                          hello.h  -  description
                             -------------------
    begin                : Wed Aug 13 2003
    copyright            : (C) 2003 by Luke Klein-Berndt
    email                : kleinb@nist.gov
 ***************************************************************************/
#ifndef HELLO_H
#define HELLO_H

#include "aodv.h"
#include "aodv_neigh.h"
#include "aodv_route.h"
#include "socket.h"
#include "signal.h"

int send_hello();
int recv_hello(rrep * tmp_rrep, task * tmp_packet);


#endif
