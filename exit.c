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


#include "global.h"
#include "za_task.h"
#include "usema.h"

GLOBAL struct Library * DOSBase;

// SNPrintf is safe if maxlen-len < 1 !!
#define ADD(fmt...)	\
	len += SNPrintf( &buf[len], sizeof(buf)-len-1, fmt )

//------------------------------------------------------------------------------

BOOL SafeToExit( VOID )
{
	STATIC CONST UBYTE __reallyquitmsg[] = "Do you really want to quit?";
	BOOL rc = TRUE, imle;
	ZoneAlarmTask * zat;
	int len=0;
	UBYTE buf[4096];
	
	ENTER();
	
	Forbid();
	
	if(!uSemaLockAttempt ( ))
	{
		struct Task * t = uSemaOwner ( ) ;
		
		rc = FALSE;
		ADD("Cannot try to exit right now, Process $%lx (%s) is actively using me!", t, t->tc_Node.ln_Name );
		
		Permit();
		
		RA_Requester(tell_gadgets,buf);
	}
	else if(IsMinListEmpty(G->ZoneAlarmTasks))
	{
		/**
		 * There is no tasks using me, pop a single confirmation
		 */
		
		Permit();
		
		if(RA_Requester(ask_gadgets,(STRPTR)__reallyquitmsg)!=1)
		{
			rc = FALSE;
		}
	}
	else
	{
		/**
		 * There are some programs using me, ask for confirmation
		 * telling the user which tasks are running
		 */
		
		ADD("The following applications are\nstill using %s:\n\n",ProgramName());
		
		ITERATE_LIST( G->ZoneAlarmTasks, ZoneAlarmTask *, zat )
		{
			ADD(" %s ($%lx)\n", zat->task_name, zat->task );
		}
		
		ADD("\n\033b%s", __reallyquitmsg);
		DBG_STRING(buf);
		
		Permit();
		
		if(RA_Requester("_Quit|_Cancel", buf) == 1)
		{
			/**
			 * user really want to quit, break the tasks using me..
			 */
			
			Forbid();
			ITERATE_LIST( G->ZoneAlarmTasks, ZoneAlarmTask *, zat )
			{
				DBG(".........Breaking $%lx (%s)\n", zat->task, zat->task_name);
				
				Signal( zat->task, SIGBREAKF_CTRL_C );
			}
			Permit();
			
			/**
			 * do a little delay to let the tasks exit
			 */
			Delay( 4 * TICKS_PER_SECOND );
			
			Forbid();//ObtainSemaphore(G->Semaphore);
			imle = IsMinListEmpty(G->ZoneAlarmTasks) ? TRUE:FALSE;
			Permit();//ReleaseSemaphore(G->Semaphore);
			
			if(imle==FALSE)
			{
				/**
				 * uh-oh... some task refused to quit, let the
				 * user know which
				 */
				
				Forbid();
				
				len = 0;
				ADD("\033b%s is unable to exit, the following\napplications refuse to quit:\033n\n\n", ProgramName());
				
				ITERATE_LIST( G->ZoneAlarmTasks, ZoneAlarmTask *, zat )
				{
					ADD(" %s ($%lx)\n", zat->task_name, zat->task );
				}
				
				//ADD("\n\033b...try again right now");
				ADD("\n\033b...do you want to quit anyway? your\nsystem MAY becomes unstable though\n");
				DBG_STRING(buf);
				
				Permit();
				
				if(RA_Requester(ask_gadgets,buf)==0)
					rc = FALSE;
			}
		}
		else rc = FALSE;
	}
	
	/**
	 * if we're about to exit, do not unlock our uSema!, so we can safely
	 * exit being waiting tasks breaked and making the at exit step faster..
	 */
	if( rc == FALSE )
		uSemaUnLock ( ) ;
	
	RETURN(rc);
	return(rc);
}
