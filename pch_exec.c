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
#include "alert.h"
#include "usema.h"
#include "pchs.h"
#include "pch_dos.h"

STATIC struct Library * za_OpenLibrary
(	REG(a1, STRPTR libName),
	REG(d0, ULONG version),
	REG(a6, struct ExecBase *SysBase));
STATIC void za_CloseLibrary
(	REG(a1, struct Library * library),
	REG(a6, struct ExecBase *SysBase));
STATIC void za_ColdReboot
(	REG(a6,struct ExecBase *SysBase));
STATIC void za_remtask
(	REG(a1,struct Task * task),
	REG(a6, struct ExecBase *SysBase));

VOID RemoveExecPatches( VOID );

// @ pch_socket.c
GLOBAL BOOL InstallSocketPatches(struct Library *socketbase);
GLOBAL VOID RemoveSocketPatches(struct Library *socketbase);
GLOBAL VOID DisposeSocketPatches( VOID );

GLOBAL struct Library * DOSBase;
//-----------------------------------------------------------------------------

BOOL InstallExecPatches( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
	if(G->MagicID != GLOBAL_MAGIC)
		return FALSE;
	
	Forbid();
	
	/**
	 * G->SocketPatches are managed here just by the fact they are 
	 * installed from out OpenLibrary patch.
	 */
	if((G->SocketPatches = Malloc(sizeof(struct MinList))))
	{
		SefFuncWrapper Patches[] = {
			{ &(G->OpenLibrary_pch),	-0x228, za_OpenLibrary,  0},
			{ &(G->CloseLibrary_pch),	-0x19e, za_CloseLibrary, 0},
			{ &(G->ColdReboot_pch),		-0x2d6, za_ColdReboot,   0},
			{ &(G->RemTask_pch),		-0x120, za_remtask,      0},
			{ NULL, 0, NULL, 0 }
		};
		
		if(sf_install( Patches, SysBase ))
		{
			rc = TRUE;
			
			NewList((struct List *)G->SocketPatches);
		}
		else
		{
			Free(G->SocketPatches);
			G->SocketPatches = NULL;
		}
	}
	
	Permit();
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

VOID RemoveExecPatches( VOID )
{
	ENTER();
	DBG_ASSERT(G->MagicID == GLOBAL_MAGIC);
	
	if(G->MagicID == GLOBAL_MAGIC)
	{
		ULONG patches[] = {
			(ULONG) &(G->RemTask_pch),
			(ULONG) &(G->OpenLibrary_pch),
			(ULONG) &(G->CloseLibrary_pch),
			(ULONG) &(G->ColdReboot_pch),
			 0L
		};
		
	/**
	 * WARNING:  dunno why, but I've noticed the ColdReboot() function 
	 * being called (NOT ALWAYS, ofcourse) from this function !!
	 * The exact situation was by the main task owning the uSema, when 
	 * MiamiDx tryed to re-open socketbase it fails to lock the sema
	 * since we are exiting (dunno if something to do with MiamiDx, tho)
	 * also.. at the same time our RemTask() patch was being used, and may
	 * the responsible..thanks that I have another ColdReboot() patch from
	 * a external program i was able to avoid the reset, and just after
	 * the debugging message from RemTask() appears..    (?????)
	 * SOLVE: if I dunno why happened....well, G->RemTask_pch->sf_Count
	 * was missing (since the function do not return) but I've added it
	 * right now, plus RemTask patch will be the first to remove.. :-/
	 */
		
		sf_remove((ULONG **)patches);
		
		DBG_ASSERT(G->SocketPatches);
		if(G->SocketPatches)
		{
			DisposeSocketPatches();
			
			DBG_ASSERT(IsMinListEmpty(G->SocketPatches)==TRUE);
			
			Free(G->SocketPatches);
			G->SocketPatches = NULL;
		}
	}
	
	LEAVE();
}

//-----------------------------------------------------------------------------

typedef struct Library * ASM (*OpenLibraryCB)
(	REG(a1, STRPTR libName),
	REG(d0, ULONG version),
	REG(a6, struct ExecBase *SysBase));

#define toLower(src_ch) \
({ \
	unsigned char ch = src_ch; \
	if(((ch > 64) && (ch < 91)) || ((ch > 191) && (ch < 224))) ch += 32; \
	ch; \
})

INLINE BOOL Compare( unsigned char * pajar, unsigned char * aguja )
{
	register const unsigned char *src=(const unsigned char *) pajar;
	
	while(*src)
	{
		register const unsigned char * a=(const unsigned char *) aguja;
		
		while(*src && (toLower(*src++) == toLower(*a++)))
		{
			if(!(*a))
			{
				if(!(*src))
					return TRUE;
				
				break;
			}
		}
	}
	
	return FALSE;
}


STATIC struct Library * za_OpenLibrary
(	REG(a1, STRPTR libName),
	REG(d0, ULONG version),
	REG(a6, struct ExecBase *SysBase))
{
	OpenLibraryCB sys_OpenLibrary;
	struct Library * rc = NULL;
	
//	ENTER();
//	DBG("%s: %s v%ld\n", FindTask(NULL)->tc_Node.ln_Name, libName, version);
	
	Forbid();
	G->OpenLibrary_pch->sf_Count++;
	Permit();
	
	sys_OpenLibrary = G->OpenLibrary_pch->sf_OriginalFunc;
	rc = sys_OpenLibrary( libName, version, SysBase);
	
	if(rc && Compare( libName, "bsdsocket.library"))
	{
		if(uSemaLock ( ))
		{
			InstallSocketPatches(rc);
			uSemaUnLock();
		}
	}
	
	Forbid();
	G->OpenLibrary_pch->sf_Count--;
	Permit();
	
//	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

typedef void ASM (*CloseLibraryCB)
(	REG(a1, struct Library * library),
	REG(a6, struct ExecBase *SysBase));

STATIC void za_CloseLibrary
(	REG(a1, struct Library * library),
	REG(a6, struct ExecBase *SysBase))
{
	CloseLibraryCB sys_CloseLibrary;
	
//	ENTER();
//	DBG_STRING(FindTask(NULL)->tc_Node.ln_Name);
	
	Forbid();
	G->CloseLibrary_pch->sf_Count++;
	
	sys_CloseLibrary = G->CloseLibrary_pch->sf_OriginalFunc;
	
	// check if 'library' is a SocketBase and remove from our list if so
	RemoveSocketPatches(library);
	
	sys_CloseLibrary( library, SysBase );
	
	G->CloseLibrary_pch->sf_Count--;
	Permit();
	
//	LEAVE();
}

//-----------------------------------------------------------------------------

typedef VOID ASM (*ColdRebootCB)(REG(a6,struct ExecBase *SysBase));

STATIC VOID za_ColdReboot(REG(a6,struct ExecBase *SysBase))
{
	ENTER();
	TRACE_START;
	
	Forbid();
	G->ColdReboot_pch->sf_Count++;
	Permit();
	
	if(uSemaLock ( ))
	{
		struct xyz_magic_zat crp;
		
		bZero( &crp, sizeof(struct xyz_magic_zat));
		
		if((crp.zat = zat_malloc()))
		{
			UWORD Flags = (AF_EXEC|AF_COLDREBOOT);
			BOOL allow = FALSE;
			LONG a_rc;
			TaskReg * tr;
			
			a_rc = TaskRegRequest(tr,crp.zat,Flags,0xfffe);
			
			if(a_rc == -1)
			{
				ALERT_DATA( Flags, &crp, tr );
				
				a_rc = alert( &ad );
				
				DBG_ASSERT(a_rc != -1);
				if( a_rc != -1 )
				{
					allow = (a_rc == 1);
					
					TaskRegUpdate(tr,crp.zat,allow,&ad,0xfffe);
				}
			}
			else
			{
				DBG_ASSERT(a_rc==1||a_rc==0);
				
				allow = (a_rc ? TRUE : FALSE);
			}
			
			if(allow == TRUE)
			{
				ColdRebootCB sys_ColdReboot;
				
				sys_ColdReboot = G->ColdReboot_pch->sf_OriginalFunc;
					
				#ifdef DEBUG
				DBG("\n\n\n...REBOOTING SYSTEM as requested by \"%s\"...\n", crp.zat->task_name);
				Delay(150);
				#endif
				
				/**
				 * We are about the reset the system, save the most 
				 * important data...
				 */
				TaskRegSave ( ) ;
				
				
				// that is right!, REBOOTING SYSTEM...
				sys_ColdReboot( SysBase );
			}
			
			zat_free(crp.zat);
		}
		
		uSemaUnLock ( ) ;
	}
	
	Forbid();
	G->ColdReboot_pch->sf_Count--;
	Permit();
	
	TRACE_END;
	LEAVE();
}

//-----------------------------------------------------------------------------

typedef VOID ASM (*RemTaskCB)
(	REG(a1, struct Task * task),
	REG(a6, struct ExecBase *SysBase));

STATIC void za_remtask
(	REG(a1, struct Task * task),
	REG(a6, struct ExecBase *SysBase))
{
	RemTaskCB sys_RemTask;
	struct Task * proc;
	CreateNewProcPatch * npp;
	
	/**
	 * This function NEVER returns!
	 */
	
	#ifdef DEBUG
	int parents = 0, childs = 0, items = 0;
	#endif
	
	Forbid();
	G->RemTask_pch->sf_Count++;
	Permit();
	
	// even if could happens, it is not recomended..
	DBG_ASSERT(task == NULL);
	if(!(proc = task))
		proc = FindTask(NULL);
	
	npp = (CreateNewProcPatch *)((struct List *)(G->CreateNewProcPatches))->lh_Head;
	while(TRUE)
	{
		struct MinNode * succ;
		
		if(npp == NULL) break;
		if(!(succ = ((struct MinNode *)npp)->mln_Succ))
			break;
		
		if(npp->parent == proc || (struct Task *)npp->newproc == proc)
		{
			#ifdef DEBUG
			if(npp->parent == proc)
				childs++;
			else
				parents++;
			items--;
			#endif
			
			Remove((struct Node *)npp);
			Free(npp);
		}
		#ifdef DEBUG
		items++;
		#endif
		
		npp = (CreateNewProcPatch *) succ;
	}
	
	DBG("Task $%lx (%s) created %ld subprocesses (%ld/%ld)\n",
		proc, proc->tc_Node.ln_Name, childs, parents, items );
	
	
	sys_RemTask = G->RemTask_pch->sf_OriginalFunc;
	
	Forbid();
	G->RemTask_pch->sf_Count--;
	Permit();
	
	sys_RemTask( task, SysBase );
}

