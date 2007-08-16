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

/**
 * $Id: startup.c,v 0.1 2006/07/06 23:39:43 diegocr Exp $
 */
asm(".globl __start\njbsr __start\nrts");

#define __NOLIBBASE__
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "global.h"

struct Library * SysBase = NULL;
struct Library * DOSBase = NULL;

GLOBAL LONG Main(APTR _WBenchMsg);
GLOBAL BOOL AllocGlobal ( VOID );
GLOBAL VOID FreeGlobal ( VOID );

//------------------------------------------------------------------------------

int _start ( void )
{
	struct Process * this_process;
	APTR old_window_pointer;
	struct Task *this_task;
	int rc = 1;
	struct WBStartup * _WBenchMsg = NULL;
	
	SysBase = *(struct Library **) 4L;
	
	this_process = (struct Process *)(this_task = FindTask(NULL));
	
	if(!this_process->pr_CLI)
	{
		struct MsgPort * mp = &this_process->pr_MsgPort;
		
		WaitPort(mp);
		
		_WBenchMsg = (struct WBStartup *)GetMsg(mp);
	}
	
	old_window_pointer = this_process->pr_WindowPtr;
	
	if(InitMemoryPool())
	{
		if(AllocGlobal ( ))
		{
			if((DOSBase = OpenLibrary(__dosname,0)))
			{
				rc = Main(G->WBenchMsg = (APTR)_WBenchMsg);
				
				CloseLibrary(DOSBase);
			}
			
			FreeGlobal();
		}
		
		ClearMemoryPool();
	}
	
	this_process->pr_WindowPtr = old_window_pointer;
	
	if(_WBenchMsg != NULL)
	{
		Forbid();
		
		ReplyMsg((struct Message *)_WBenchMsg);
	}
	
	return(rc);
}
void __main(){}
