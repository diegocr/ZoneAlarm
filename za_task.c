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
#include "pch_dos.h"
#include "ipc.h"
#include "xvs.h"

//------------------------------------------------------------------------------

INLINE ULONG strlen(const char *string)
{ const char *s;
#if 1
  if(!(string && *string))
  	return 0;
#endif
  s=string;
  do;while(*s++); return ~(string-s);
}

//------------------------------------------------------------------------------

INLINE STRPTR StrNDup( STRPTR string, ULONG length )
{
	STRPTR dst;
	
	if((dst = Malloc( length+1 )))
	{
		CopyMem( string, dst, length );
		dst[length] = 0;
	}
	
	return dst;
}

//------------------------------------------------------------------------------

INLINE STRPTR StrDup( STRPTR string )
{
	return( StrNDup ( string, (ULONG) strlen( string ) + 1 ));
}

//------------------------------------------------------------------------------

INLINE STRPTR GetTaskName( struct Task *task, UWORD *namelen )
{
	struct Process *pr = (struct Process *) task;
	STRPTR name = NULL;
	
//	ENTER();
//	DBG_POINTER(task);
	
	#define b2c(x) ( x << 2 )  /*  BPTR to C string */
	
	if(namelen) (*namelen) = 0;
	
	if (task->tc_Node.ln_Type == NT_PROCESS && pr->pr_CLI != (BPTR)NULL)
	{
		struct CommandLineInterface *cli =
			(struct CommandLineInterface *)BADDR(pr->pr_CLI);
		
		if(cli && cli->cli_Module && cli->cli_CommandName)
		{
			STRPTR bn = (STRPTR) b2c(cli->cli_CommandName);
			
			if(*bn > 0)
			{
				if((name = StrNDup( &bn[1], *bn )))
				{
					if(namelen) (*namelen) = *bn;
				}
			}
		}
	}
	
	if( name == NULL )
	{
		UWORD len = strlen(task->tc_Node.ln_Name);
		
		if((name = StrNDup(task->tc_Node.ln_Name, len )))
		{
			if(namelen) (*namelen) = len;
		}
	}
	
//	DBG_STRING(name);
	
//	RETURN(name);
	return(name);
}

//------------------------------------------------------------------------------

INLINE STRPTR Executable(struct Task *task)
{
	STRPTR ename = NULL;
	GLOBAL struct DosLibrary * DOSBase;
	
//	ENTER();
	DBG_ASSERT(G->mTask == FindTask(NULL));
	DBG_ASSERT(IsTaskRunning(task)==TRUE);
	
	if((ename = Malloc(1025)))
	{
		struct Process * pr = ((struct Process *)task);
		STRPTR tn = GetTaskName(task, NULL);
		
		if(task->tc_Node.ln_Type != NT_TASK && pr->pr_HomeDir)
		{
			NameFromLock( pr->pr_HomeDir, ename, 1024);
		}
		
		if(tn != NULL)
		{
			AddPart( ename, FilePart(tn), 1024);
			Free(tn);
		}
	}
	
//	DBG_STRING(ename);
	
//	RETURN(ename);
	return(ename);
}

//------------------------------------------------------------------------------

INLINE STRPTR GetProgVersion(REG(a0, STRPTR p), REG(a1, LONG size ))
{
	STRPTR rc = NULL;
	
	ENTER();
	
	if((rc = Malloc( 257 )))
	{
		STRPTR v=rc;
		rc[0] = 0;
		
		while( size-- > 0 )
		{
			if(*p++ == '$' && *p++ == 'V' && *p++ == 'E' && *p++ == 'R' && *p++ == ':')
			{
				int max = 256;
				
				while(*p && *p == 0x20) p++;
				
				while(*p && max-- > 0)
				{
					if(*p == '\r' || *p == '\n') break;
					*v++ = *p++;
				}
				
				*v = 0;
				break;
			}
		}
		
		if(rc[0] == 0) // no version found?
		{
			Free(rc);
			rc = NULL;
		}
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

INLINE ZoneAlarmTask * ZoneAlarmRecord( ZoneAlarmTask * zat )
{
	ZoneAlarmTask * ptr, * rc = NULL;
	
	ITERATE_LIST( G->AllZoneAlarmTasks, ZoneAlarmTask *, ptr )
	{
		if(!StrCmp( ptr->executable, zat->executable ))
		{
			rc = ptr;
			break;
		}
		
		DBG_ASSERT(ptr->xvs != XVS_VIRUSDETECTED);
	}
	
	return rc;
}

//------------------------------------------------------------------------------

INLINE VOID zat_properties( ZoneAlarmTask * zat )
{
	D_S(struct FileInfoBlock,fib);
	GLOBAL struct Library * DOSBase;
	STRPTR data;
	ZoneAlarmTask * x;
	
	zat->execrc = (~0);
	zat->exever = NULL;
	zat->virus  = NULL;
	
	ENTER();
	DBG_ASSERT(G->mTask == FindTask(NULL));
	
	if((x = ZoneAlarmRecord( zat )))
	{
		DBG("Found record for \"%s\"\n", zat->executable );
		
		zat->execrc = x->execrc;
		
		if( x->exever && *x->exever )
			zat->exever = StrDup(x->exever);
		
		zat->xvs = x->xvs;
		if( x->virus && *x->virus )
			zat->virus = StrDup(x->virus);
		
		DBG_ASSERT(x->xvs != XVS_VIRUSDETECTED);
	}
	else if((data = FileToMem( zat->executable, fib, DOSBase )))
	{
		// Get the CRC32 checksum of the file
		zat->execrc = CRC32( data, fib->fib_Size, zat->execrc );
		
		// Get the $VER: string
		zat->exever = GetProgVersion( data, fib->fib_Size );
		
		// Check if it has some virus..
		zat->xvs = xvs_analyzer( data, fib->fib_Size, &zat->virus );
		
		FreeVec(data);
	}
	else if(IoErr() == ERROR_NO_FREE_STORE)
	{
		BPTR fd;
		
		DBG(" -+-+-+-+-+-+-+-+-+-+- have to load the file by chunks!\n");
		
		if((fd = Open( zat->executable, MODE_OLDFILE)))
		{
			LONG size = 262144;
			
			while(!(data = AllocVec( size+1, MEMF_FAST )))
			{
				size >>= 1;
				
				if(size < 1024) break;
			}
			
			if( data != NULL )
			{
				LONG r;
				
				while((r=Read( fd, data, size )) > 0)
				{
					zat->execrc = CRC32( data, r, zat->execrc );
				}
				
				FreeVec(data);
			}
			
			Close(fd);
		}
	}
	
	if( x == NULL )
	{
		// if not already, record this entry
		
		if((x = zat_copy( zat )))
		{
			AddTail((struct List *)G->AllZoneAlarmTasks,(struct Node *)x);
		}
	}
	
	LEAVE();
}

//------------------------------------------------------------------------------

ZoneAlarmTask * zat_malloc_SAFE(struct Task *task)
{
	ZoneAlarmTask * zat = NULL;
	GLOBAL struct Library * DOSBase;
	
	ENTER();
	DBG_ASSERT(task != NULL);
	
	if((task != NULL) && (zat = Malloc(sizeof(ZoneAlarmTask))))
	{
		struct Task * last = NULL, * next;
		
		/**
		 * TODO: before trying to obtain the executable from the 
		 * parent task may I should check if it is running, fact
		 * which is always unless a CLI program is detached ...
		 */
		
		next = zat->task = task;
		while((zat->parent = NewProcSearch(next)))
			next = last = zat->parent;
		
		zat->parent	= last;
		zat->task_name	= GetTaskName(zat->task,&(zat->TaskNameLen));
		zat->executable	= Executable(zat->parent?zat->parent:zat->task);
		zat_properties( zat );
		
		DBG("task=%lx parent=%lx name=\"%s\" exe=\"%s\" execrc=%lx exever=%lx\n",
			zat->task,
			zat->parent,
			zat->task_name,
			zat->executable,
			zat->execrc, zat->exever );
		
		AddTail((struct List *)G->ZoneAlarmTasks,(struct Node *)zat);
	}
	
	RETURN(zat);
	return zat ;
}

ZoneAlarmTask * zat_malloc( VOID )
{
	/**
	 * Sometimes calling NameFromLock() (well, I've noticed it only from 
	 * ixemul.library's 'vfork()'d process') causes a GURU of type
	 * 0x87000004 (no wb startup) ... using IPC fixes the problem..even 
	 * if slowy :-/
	 * 
	 * UPDATE: same prob happens with CRC32OfFile() (also only from vfork)
	 * hence zat_malloc has been renamed to zat_malloc_SAFE (which called
	 * from the main task) and the IPC_PutMsg has been moved from 
	 * Executable() to this function, from now the whole job of obtain 
	 * that info is done from the main task.
	 */
	
	APTR data = NULL;
	
	ENTER();
	
	DBG_ASSERT(G && G->MagicID == GLOBAL_MAGIC);
	if(G && G->MagicID == GLOBAL_MAGIC)
	{
		data = (APTR) FindTask(NULL);
		
		if(!IPC_PutMsg( G->mPort, IPCA_ZONEALARMTASK, &data ))
			data = NULL;
	}
	
	RETURN(data);
	return((ZoneAlarmTask *) data );
}

//------------------------------------------------------------------------------

ZoneAlarmTask * zat_copy(ZoneAlarmTask *zat)
{
	ZoneAlarmTask * new;
	
	if((new = Malloc(sizeof(ZoneAlarmTask))))
	{
		BOOL ok = FALSE;
		
		CopyMem( zat, new, sizeof(ZoneAlarmTask));
		
		new->task_name	= NULL;
		new->executable = NULL;
		new->virus	= NULL;
		bZero( &(new->node), sizeof(struct MinNode));
		
		if((new->task_name = StrNDup( zat->task_name, zat->TaskNameLen)))
		{
			if((new->executable = StrDup( zat->executable )))
				ok = TRUE;
			
			if(zat->virus && *zat->virus)
				new->virus = StrDup( zat->virus );
		}
		
		if( ok == FALSE )
		{
			Free(new->virus);
			Free(new->executable);
			Free(new->task_name);
			Free(new);
			new = NULL;
		}
	}
	
	return new;
}

//------------------------------------------------------------------------------

VOID zat_free(ZoneAlarmTask * zat)
{
	ENTER();
	
	Forbid();
	
	if(zat != NULL)
	{
		ZoneAlarmTask *ptr;
		
		DBG("task=%lx parent=%lx name=\"%s\" exe=\"%s\" execrc=%lx\n",
			zat->task,
			zat->parent,
			zat->task_name,
			zat->executable,
			zat->execrc );
		
	/**
	 * WARNING: I've noticed the follow case:
	 * MiamiDx creates two subprocesses (at least), "Miami Beach" and
	 * "Miami IPNatD" and at some step while they ARE still running, 
	 * MiamiDx closes its bsdsocket.library which makes zat_free() being 
	 * called to free any data associated with the task, well, when this 
	 * happens and IPNatD tryes to do something ZoneAlarm can watch, the 
	 * reported parent exe of it is "Miami Beach" (who created IPNatD on 
	 * this case) and what makes properly identification impossible.
	 * 
	 * SOLVE: for now I'll try to avoid using NewProcRemove here, and 
	 * instead a RemTask() patch will be now installed to safely remove
	 * them, there is still the problem about a CLI program could be 
	 * detached from its parent task ... may a new linked list should be
	 * added to copy the zat data which is about to be freed here, to later
	 * obtain the task data from that copy if the task isn't running.
	 */
		
		// remove any CreateNewProc() log by this task (if any)
	//	NewProcRemove(zat->task);
		
		ITERATE_LIST( G->ZoneAlarmTasks, ZoneAlarmTask *, ptr )
		{
			if(((ULONG)ptr) == ((ULONG)zat))
			{
				DBG_ASSERT(ptr->execrc == zat->execrc);
				DBG("The task have been unlinked\n");
				Remove((struct Node *) ptr );
				break;
			}
		}
		
		Free(zat->exever);
		Free(zat->task_name);
		Free(zat->executable);
		Free(zat);
	}
	
	Permit();
	
	LEAVE();
}

//------------------------------------------------------------------------------


