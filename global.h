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

#ifndef ZONEALARM_GLOBAL_H
#define ZONEALARM_GLOBAL_H

#include "SFPatch.h"
#include <proto/alib.h>

#if defined(__NOLIBBASE__)
GLOBAL struct Library * SysBase;
#endif

#include "debug.h"
#include "palloc.h"
#include "utils.h"
#include "thread.h"

#ifndef I64_MATH_INLINE
typedef struct {
    unsigned hi;  /* Most significant 32 bits */
    unsigned lo;  /* Least significant 32 bits */
} bigint;
#endif

typedef struct __GlobalProgramData
{
	ULONG MagicID;
	#define GLOBAL_MAGIC	0xff789cbc
	
	struct Task * mTask;			// ZoneAlarm (main) task
	struct MsgPort * mPort;			// Port to IPC Messages
	struct MsgPort * mWbPort;		// Port to Workbench Messages..
	struct SignalSemaphore * Semaphore;	// just a semaphore (do not use)
	
						// main patches..
	SetFunc * OpenLibrary_pch;
	SetFunc * CloseLibrary_pch;
	SetFunc * ColdReboot_pch;
	SetFunc * RemTask_pch;
	SetFunc * CreateNewProc_pch;
	SetFunc * CreateProc_pch;
						// further expansion reserved
	SetFunc * sf_reserved1;
	SetFunc * sf_reserved2;
	SetFunc * sf_reserved3;
	
						// linked lists records
	struct MinList * SocketPatches;
	struct MinList * CreateNewProcPatches;
	struct MinList * TaskRegList;
	struct MinList * ZoneAlarmTasks;
	struct MinList * AllZoneAlarmTasks;
						// further expansion reserved
	struct MinList * ml_reserved1;
	struct MinList * ml_reserved2;
	struct MinList * ml_reserved3;
	
	struct Task * Workbench_task;		// The Workbench task (crucial)
	APTR WBenchMsg;				// Startup WB Message
	STRPTR ProgFilename;			// full path filename to ZA
	
	ThreadManager * tm;			// Threading system
	
	bigint bytes_sent;
	bigint bytes_recv;
	
	ULONG reserved[32];			// reserved space..
} Global;

GLOBAL Global *G;

#if !defined(IsMinListEmpty)
# define IsMinListEmpty(x)     (((x)->mlh_TailPred) == (struct MinNode *)(x))
#endif

#define ITERATE_LIST(list, type, node)				\
	for (node = (type)((struct List *)(list))->lh_Head;	\
		((struct MinNode *)node)->mln_Succ;		\
			node = (type)((struct MinNode *)node)->mln_Succ)

GLOBAL UBYTE __dosname[];


GLOBAL STRPTR ProgramName( VOID );
GLOBAL STRPTR ProgramVersion( VOID );
GLOBAL STRPTR ProgramVersionTag( VOID );
GLOBAL STRPTR ProgramDate( VOID );

GLOBAL STRPTR DataBaseFile( VOID );
GLOBAL STRPTR DataBaseFileCached( VOID );

#define MAGIC_CHECK(RC)				\
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);	\
	if(G->MagicID != GLOBAL_MAGIC) return RC

/* this macro lets us long-align structures on the stack */
#define D_S(type,name)	\
	char a_##name[ sizeof( type ) + 3 ];	\
	type *name = ( type * ) ( ( LONG ) ( a_##name + 3 ) & ~3 )

#ifndef MAKE_ID
# define MAKE_ID(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#endif /* ZONEALARM_GLOBAL_H */
