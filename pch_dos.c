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
#include "pch_dos.h"
#include "pchs.h"
#include <exec/execbase.h>

STATIC struct Process * za_CreateNewProc
(	REG(d1, struct TagItem *tags),
	REG(a6, struct DosLibrary *DOSBase));
STATIC struct MsgPort * za_CreateProc
(	REG(d1, STRPTR name),
	REG(d2, LONG pri),
	REG(d3, BPTR seglist),
	REG(d4, LONG stackSize),
	REG(a6, struct DosLibrary *DOSBase));

GLOBAL struct Library * DOSBase;

//-----------------------------------------------------------------------------

BOOL InstallDosPatches( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
	if(G->MagicID != GLOBAL_MAGIC)
		return FALSE;
	
	Forbid();
	
	/**
	 * Install a CreateNewProc() patch to watch 'child' processes and hence
	 * be able to obtain the real task (parent) name, executable, ...
	 */
	if((G->CreateNewProcPatches = Malloc(sizeof(struct MinList))))
	{
		SefFuncWrapper Patches[] = {
			{ &(G->CreateNewProc_pch), -0x1f2, za_CreateNewProc, 0},
			{ &(G->CreateProc_pch),    -0x8a,  za_CreateProc,    0},
			{ NULL, 0, NULL, 0 }
		};
		
		if(sf_install( Patches, DOSBase ))
		{
			NewList((struct List *)G->CreateNewProcPatches);
			
			rc = TRUE;
		}
	}
	
	if( rc == FALSE )
	{
		Free(G->CreateNewProcPatches);
		G->CreateNewProcPatches = NULL;
	}
	
	Permit();
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

VOID RemoveDosPatches( VOID )
{
	ENTER();
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
	
	if(G->MagicID == GLOBAL_MAGIC)
	{
		ULONG patches[] = {
			(ULONG) &(G->CreateNewProc_pch),
			(ULONG) &(G->CreateProc_pch),
			0L
		};
		
		sf_remove((ULONG **)patches);
		
		// could happens anyway..
		DBG_ASSERT(IsMinListEmpty(G->CreateNewProcPatches)==TRUE);
	}
	
	LEAVE();
}

//-----------------------------------------------------------------------------
/**
 * This function is used to remove from our list any log for tasks no 
 * longer running.
 * NOTE: instead MAY should I install a new PATCH (maybe on RemTask()) to
 * "auto detect" when they exit, but Im not so guru on how the AmigaOS
 * removes processes...
 */
/**
 * UPDATE: yep, finally I've created the RemTask() patch, under alpha 
 * state tough..
 */
#if 0
INLINE BOOL IsTaskRunning(struct Task * qTask)
{
	BOOL rc = FALSE;
	struct Task * task;
	struct ExecBase * SysBase = *(struct ExecBase **) 4L;
	
	// This is called Forbid()'d
	
	ENTER();
	DBG_POINTER(qTask);
	
	ITERATE_LIST(&SysBase->TaskReady, struct Task *, task)
	{
		if(qTask == task) {
			rc = TRUE; break;
		}
	}
	
	if( rc == FALSE )
	{
		ITERATE_LIST(&SysBase->TaskWait, struct Task *, task)
		{
			if(qTask == task) {
				rc = TRUE; break;
			}
		}
	}
	
	RETURN(rc);
	return(rc);
}
INLINE BOOL NPSD_Iterator( VOID )
{
	BOOL rc = FALSE;
	CreateNewProcPatch * npp;
	
	ITERATE_LIST( G->CreateNewProcPatches, CreateNewProcPatch *, npp )
	{
		if(!IsTaskRunning(npp->parent))
		{
			NewProcRemove(npp->parent);
			rc = TRUE;
			break;
		}
	}
	
	return rc;
}
INLINE VOID NewProcSearchAndDestroy( VOID )
{
	while(NPSD_Iterator());
}
#endif

//-----------------------------------------------------------------------------
/**
 * Check if the task we are about to patch (the one which opened 
 * bsdsocket.library), is a child of another task, returning the parent
 * task if so
 */
struct Task * NewProcSearch(struct Task * task)
{
	struct Task * rc = NULL;
	CreateNewProcPatch * npp;
	
	// this is executed uSemaLock'ed
	
	ENTER();
	DBG_POINTER(task);
	
	Forbid();
	//NewProcSearchAndDestroy();
	
	ITERATE_LIST( G->CreateNewProcPatches, CreateNewProcPatch *, npp )
	{
		if(((ULONG)npp->newproc) == ((ULONG)task))
		{
			DBG("Found task $%lx (%s) as a child of $%lx (%s)\n", task, task->tc_Node.ln_Name, npp->parent, npp->parent->tc_Node.ln_Name);
			
			rc = npp->parent;
			break;
		}
	}
	
	Permit();
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

VOID NewProcRemove(struct Task *task)
{
	// this is executed uSemaLock()'ed OR Forbid()'ed
	
	CreateNewProcPatch * npp;
	
	#ifdef DEBUG
	int childs = 0, items = 0;
	
	ENTER();
	DBG_POINTER(task);
	#endif
	
	Forbid();
	
	npp = (CreateNewProcPatch *)((struct List *)(G->CreateNewProcPatches))->lh_Head;
	while(TRUE)
	{
		struct MinNode * succ;
		
		if(npp == NULL) break;
		if(!(succ = ((struct MinNode *)npp)->mln_Succ))
			break;
		
		if(npp->parent == task)
		{
			Remove((struct Node *)npp);
			Free(npp);
			
			#ifdef DEBUG
			childs++;
			#endif
		}
		#ifdef DEBUG
		items++;
		#endif
		
		npp = (CreateNewProcPatch *) succ;
	}
	
	DBG("Task $%lx created %ld subprocesses (%ld)\n", task, /*task->tc_Node.ln_Name,*/ childs, items);
	Permit();
	
	LEAVE();
}

//-----------------------------------------------------------------------------

INLINE VOID NewProcLog( struct Process * proc )
{
	struct Task * ThisTask = FindTask(NULL);
	
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
	DBG("Process $%lx (%s) Created From Task $%lx (%s)\n", proc, 
		((struct Task *)proc)->tc_Node.ln_Name, ThisTask, ThisTask->tc_Node.ln_Name);
	
	if( ThisTask == G->Workbench_task || ThisTask == G->mTask )
	{
		DBG("Avoiding CreateNewProc() Patch from %s task!\n",
			ThisTask == G->Workbench_task ? "The Workbench":"MY");
	}
	else if(G->MagicID == GLOBAL_MAGIC)
	{
		CreateNewProcPatch * npp;
		
		if((npp = Malloc(sizeof(CreateNewProcPatch))))
		{
			npp->parent = ThisTask;
			npp->newproc = proc;
			
			Forbid();
			AddTail((struct List *)G->CreateNewProcPatches, (struct Node *)npp);
			Permit();
		}
	}
}

//-----------------------------------------------------------------------------

typedef struct Process * ASM (*CreateNewProcCB)
(	REG(d1, struct TagItem *tags),
	REG(a6, struct DosLibrary *DOSBase));

STATIC struct Process * za_CreateNewProc
(	REG(d1, struct TagItem *tags),
	REG(a6, struct DosLibrary *DOSBase))
{
	CreateNewProcCB sys_CreateNewProc;
	struct Process * rc;
	
	ENTER();
	
	Forbid();
	G->CreateNewProc_pch->sf_Count++;
	Permit();
	
	sys_CreateNewProc = G->CreateNewProc_pch->sf_OriginalFunc;
	rc = sys_CreateNewProc( tags, DOSBase );
	
	if( rc != NULL )
	{
		NewProcLog( rc );
	}
	
	Forbid();
	G->CreateNewProc_pch->sf_Count--;
	Permit();
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

typedef struct MsgPort * ASM (*CreateProcCB)
(	REG(d1, STRPTR name),
	REG(d2, LONG pri),
	REG(d3, BPTR seglist),
	REG(d4, LONG stackSize),
	REG(a6, struct DosLibrary *DOSBase));

STATIC struct MsgPort * za_CreateProc
(	REG(d1, STRPTR name),
	REG(d2, LONG pri),
	REG(d3, BPTR seglist),
	REG(d4, LONG stackSize),
	REG(a6, struct DosLibrary *DOSBase))
{
	CreateProcCB sys_CreateProc;
	struct MsgPort * rc = NULL;
	
	ENTER();
	
	Forbid();
	G->CreateProc_pch->sf_Count++;
	Permit();
	
	sys_CreateProc = G->CreateProc_pch->sf_OriginalFunc;
	rc = sys_CreateProc( name, pri, seglist, stackSize, DOSBase );
	
	if( rc != NULL )
	{
		DBG("...Hey, this task seems to be about 15 years old to be using me\n");
		NewProcLog((struct Process *)((struct Task *)rc - 1));
	}
	
	Forbid();
	G->CreateProc_pch->sf_Count--;
	Permit();
	
	RETURN(rc);
	return(rc);
}
