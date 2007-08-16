/* ***** BEGIN LICENSE BLOCK *****
 * Version: GPL License
 * 
 * Copyright (C) 2007 Diego Casorran <dcasorran@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef ZONEALARM_PCH_SOCKET_H
#define ZONEALARM_PCH_SOCKET_H

#include "global.h"
#include "za_task.h"

typedef struct SocketBasePatch
{
	struct MinNode node;
	ZoneAlarmTask * zat; // <- do not change the order
	
	SetFunc * connect_pch;
	SetFunc * bind_pch;
	SetFunc * sendto_pch;
	
	SetFunc * reserved1;
	SetFunc * reserved2;
	SetFunc * reserved3;
	SetFunc * reserved4;
	SetFunc * reserved5;
	SetFunc * reserved6;
	SetFunc * reserved7;
	SetFunc * reserved8;
	
	struct Library * base;		/* This is the task's SocketBase */
	
	APTR udata;			/* pointer to patched function data,
					 * ie for connect() patch this is
					 * 'struct sockaddr_in *' */
	
} SocketBasePatch;

GLOBAL BOOL InstallSocketPatches(struct Library *socketbase);
GLOBAL VOID RemoveSocketPatches(struct Library *socketbase);
GLOBAL VOID DisposeSocketPatches( VOID );

#define SOCKADDR(sbp) \
	((struct sockaddr_in *)((sbp)->udata))

#endif /* ZONEALARM_PCH_SOCKET_H */
