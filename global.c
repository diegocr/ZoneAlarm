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

#ifdef DEBUG
# include "za_task.h"
#endif

Global * G = NULL;

//------------------------------------------------------------------------------

/**
 * This allocates just the minimun required.
 */

BOOL AllocGlobal ( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	
	Forbid();
	
	if((G = Malloc(sizeof(Global))))
	{
		G->MagicID = GLOBAL_MAGIC;
		G->mTask = FindTask(NULL);
		
		if((G->ZoneAlarmTasks = Malloc(sizeof(struct MinList))))
		{
			NewList((struct List *) G->ZoneAlarmTasks );
			
			G->AllZoneAlarmTasks = Malloc(sizeof(struct MinList));
			
			if( G->AllZoneAlarmTasks )
			{
				NewList((struct List *) G->AllZoneAlarmTasks );
				
				/**
				 * NOTE: do NOT use G->Semaphore unless REALLY
				 * required!, uSemaLock is a GOOD alternative.
				 */
				G->Semaphore = Malloc(sizeof(struct SignalSemaphore));
				
				if(G->Semaphore)
				{
					InitSemaphore(G->Semaphore);
					
					G->Workbench_task = FindTask("Workbench");
					DBG_ASSERT(G->Workbench_task != NULL);
					
					if(G->Workbench_task != NULL)
						rc = TRUE;
				}
			}
		}
	}
	
	if( rc == FALSE )
	{
		if( G != NULL )
		{
			Free(G->AllZoneAlarmTasks);
			Free(G->ZoneAlarmTasks);
			Free(G->Semaphore);
			Free(G);
			G = NULL;
		}
	}
	
	Permit();
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

VOID FreeGlobal ( VOID )
{
	ENTER();
	
	Forbid();
	
	DBG_ASSERT(G != NULL);
	if( G != NULL )
	{
		DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
		
		if(G->MagicID == GLOBAL_MAGIC)
		{
			if(G->Semaphore != NULL)
			{
				DBG_ASSERT(G->Semaphore->ss_Owner == NULL);
				
				bZero(G->Semaphore,sizeof(struct SignalSemaphore));
				Free(G->Semaphore);
				G->Semaphore = NULL;
			}
			
			if(G->ZoneAlarmTasks != NULL)
			{
				DBG_ASSERT(IsMinListEmpty(G->ZoneAlarmTasks)==TRUE);
				Free(G->ZoneAlarmTasks);
				G->ZoneAlarmTasks = NULL;
			}
			
			if(G->AllZoneAlarmTasks != NULL)
			{
				#ifdef DEBUG
				ZoneAlarmTask * zat;
				
				DBG("---- Tasks running from this session:\n");
				
				ITERATE_LIST( G->AllZoneAlarmTasks, ZoneAlarmTask *, zat )
				{
					DBG("task=%lx name=\"%s\" parent=%lx VIR=%s\n",
						zat->task, zat->task_name, zat->parent, zat->virus );
					
					DBG_STRING(zat->executable);
					DBG_STRING(zat->exever);
					DBG_VALUE(zat->execrc);
					DBG(" + \n");
				}
				DBG("---- no more tasks --\n");
				#endif /* DEBUG */
				
				Free(G->AllZoneAlarmTasks);
				G->AllZoneAlarmTasks = NULL;
			}
			
			
			
			G->MagicID = 0xDEAD;
			Free(G);
		}
		
		G = NULL;
	}
	
	Permit();
	
	LEAVE();
}


