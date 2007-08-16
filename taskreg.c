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
#include "taskreg.h"

//#define DATABASE_FILE		"ENVARC:ZoneAlarm.dat"
#define DATABASE_FILE		DataBaseFileCached()
#define DATABASE_ID		MAKE_ID('Z','A','L','R')
#define DATABASE_EOFID		MAKE_ID('Z','E','O','F')
#define DATABASE_VERSION	MAKE_ID('0','0','0','1')

// for further additions without the need to jump version
#define DATABASE_RESERVED	0 //(sizeof(ULONG) * 32)

GLOBAL struct Library * DOSBase;

struct DupsLog
{
	struct MinNode node;
	STRPTR TaskName;
	UWORD AlertFlags;
	UWORD ServerPort;
};
STATIC struct MinList tr_dups;

//------------------------------------------------------------------------------

/****  database format  *******************************************************
[ UWORD | STRPTR | BYTE | BYTE | [ULONG|ULONG|ULONG] | [ULONG|ULONG|ULONG] ...]
  |         |        |      |         \   |   /             \   |   /
  \      TaskName  allow  remember     \  |  /               \  |  /
   \ TaskNameLength                    RegTime - DateStamp -  ModTime
*******************************************************************************/

#define GetV( dst, SiZe ) \
({	if( size >= (LONG)sizeof(SiZe)) { \
		dst = *((SiZe *) ptr ); ptr += sizeof(SiZe); size -= sizeof(SiZe); \
		if( size < 1 ) error = ERROR_SEEK_ERROR; \
	} else { error = ERROR_SEEK_ERROR; dst = 0;} dst; \
})
#define GetX( dst, SiZe ) \
({	if( size >= (LONG)SiZe ) { \
		CopyMem( ptr, dst, SiZe ); ptr += SiZe; size -= SiZe; \
		if( size < 1 ) error = ERROR_SEEK_ERROR; \
	} dst; \
})

#define PutV( ptr, type, value ) \
({ \
	*((type *) ptr ) = value; \
	ptr += sizeof(type); \
})
#define PutX( ptr, size, data ) \
({ \
	CopyMem( data, ptr, size ); \
	ptr += size; \
})

//------------------------------------------------------------------------------

STATIC BOOL CorruptDatabase( VOID )
{
	BOOL rc = FALSE;
	UBYTE message[] = 
		"\033bZoneAlarm database is corrupt!\033n\n\n"
		"Do you want to delete it right now,\n"
		"continuing with and empty database?";
	
	if( ask ( message ))
	{
		DeleteFile(DATABASE_FILE);
		rc = TRUE;
	}
	
	return(rc);
}

//------------------------------------------------------------------------------

#define ENC_BIT	149
#define transcode __i_Transcode

INLINE VOID transcode( STRPTR data, LONG len )
{
	while( len-- > 0 )
	{
		switch( len % 7 )
		{
			case 2:
			case 3:
				*data ^= ENC_BIT;
				break;
			
			case 1:
			case 4:
				if(*data & 1)
					*data &= ~1;
				else
					*data |= 1;
				/* no break here */
			default:
				*data ^= len&0xff;
				break;
		}
		
		data++;
	}
}

//------------------------------------------------------------------------------

INLINE BOOL LoadTaskRegDatabase( VOID )
{
	D_S(struct FileInfoBlock,fib);
	BOOL rc = TRUE;
	STRPTR file;
	
	ENTER();
	
	if((file = FileToMem( DATABASE_FILE, fib, DOSBase )))
		transcode( file, fib->fib_Size );
	
	if(file == NULL)
	{
		LONG ioerr;
		
		if((ioerr = IoErr()) != ERROR_OBJECT_NOT_FOUND)
		{
			ShowIoErrMessage("cannot load database");
			rc = FALSE;
		}
	}
	else if(fib->fib_Size < (LONG)sizeof(ULONG) || *((ULONG *)file) != DATABASE_ID)
	{
		if((rc=CorruptDatabase()))
		{
			FreeVec( file );
			file = NULL;
		}
	}
	
	if( rc == TRUE && file != NULL )
	{
		STRPTR ptr=&file[sizeof(ULONG)];
		LONG size=(fib->fib_Size)-sizeof(ULONG), error = 0;
		
		// this is version 1, hence no more code here atm
		DBG_ASSERT(*((ULONG *)ptr)==DATABASE_VERSION);
		ptr += sizeof(ULONG);
		size -= sizeof(ULONG);
		
		/**
		 * NOTE that since memory pools are being used, I free nothing
		 * when a error happens, hence there is no problem on this code
		 */
		while(!error)
		{
			TaskReg * entry;
			
			if(size < (LONG)sizeof(ULONG)) {
				error = TRUE; break;
			}
			
			if(*((ULONG *) ptr ) == DATABASE_EOFID) break;
			
			if((entry = Malloc(sizeof(TaskReg))))
			{
				GetV( entry->TaskNameLength, UWORD );
				if(error) break;
				
				if((entry->TaskName = Malloc(entry->TaskNameLength+1)))
				{
					GetX( entry->TaskName, entry->TaskNameLength);
					if(error) break;
					
					GetV( entry->allow,    BYTE );
					GetV( entry->remember, BYTE );
					if(error) break;
					
					GetV( entry->RegTime.ds_Days,   ULONG );
					GetV( entry->RegTime.ds_Minute, ULONG );
					GetV( entry->RegTime.ds_Tick,   ULONG );
					GetV( entry->ModTime.ds_Days,   ULONG );
					GetV( entry->ModTime.ds_Minute, ULONG );
					GetV( entry->ModTime.ds_Tick,   ULONG );
					
					GetV( entry->accesses,   ULONG );
					GetV( entry->FileCRC,    ULONG );
					GetV( entry->CRCMods,    UWORD );
					GetV( entry->AlertFlags, UWORD );
					GetV( entry->ServerPort, UWORD );
					if(error) break;
					
					AddTail((struct List *) G->TaskRegList, (struct Node *) entry );
					
					#if DATABASE_RESERVED
					ptr += DATABASE_RESERVED;
					size -= DATABASE_RESERVED;
					#endif
				}
				else error = IoErr();
			}
			else error = IoErr();
		}
		
		if( error != 0 )
		{
			if(error == ERROR_NO_FREE_STORE)
			{
				OutOfMemory("loading database");
				rc = FALSE;
			}
			else if((rc = CorruptDatabase()))
			{
				TaskReg * entry;
				
				entry = (TaskReg *)((struct List *)G->TaskRegList)->lh_Head;
				while(TRUE)
				{
					struct MinNode * succ;
					
					if(entry == NULL) break;
					if(!(succ = ((struct MinNode *)entry)->mln_Succ))
						break;
					
					Remove((struct Node *)entry);
					Free(entry->TaskName);
					Free(entry);
					
					entry = (TaskReg *) succ;
				}
				
				NewList((struct List *) G->TaskRegList );
			}
		}
	}
	
	if( file != NULL )
		FreeVec( file );
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

BOOL InitTaskReg( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	DBG_ASSERT(G && G->MagicID == GLOBAL_MAGIC);
	if(G && G->MagicID == GLOBAL_MAGIC)
	{
		if((G->TaskRegList = Malloc(sizeof(struct MinList))))
		{
			NewList((struct List *) G->TaskRegList );
			
			rc = LoadTaskRegDatabase ( ) ;
		}
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

INLINE BOOL IsDup(STRPTR TaskName,UWORD AlertFlags,UWORD ServerPort)
{
	BOOL rc = FALSE;
	struct DupsLog *d;
	
	ITERATE_LIST( &tr_dups, struct DupsLog *, d )
	{
		if((TaskName!=d->TaskName)
			&& (AlertFlags==d->AlertFlags)
			&& (ServerPort==d->ServerPort)
			&& !StrCmp( TaskName, d->TaskName ))
		{
			rc = TRUE;
			break;
		}
	}
	return rc;
}

BOOL TaskRegSave( VOID )
{
	STRPTR data;
	ULONG data_len = 0;
	int items = 0;
	BOOL rc = FALSE;
	TaskReg *entry;
	struct DupsLog *d;
	
	ENTER();
	
	// this is executed uSemaLock()'ed or Forbid()'ed
	
	DBG_ASSERT(G && G->MagicID == GLOBAL_MAGIC);
	if(!G || G->MagicID != GLOBAL_MAGIC)
	{
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	
	if(IsMinListEmpty(G->TaskRegList))
	{
		DBG("The list is empty!\n");
		return TRUE;
	}
	
	NewList((struct List *) &tr_dups );
	
	ITERATE_LIST( G->TaskRegList, TaskReg *, entry)
	{
		if(IsDup(entry->TaskName,entry->AlertFlags,entry->ServerPort))
			continue;
		
		if((d = Malloc(sizeof(struct DupsLog))))
		{
			d->TaskName   = entry->TaskName;
			d->AlertFlags = entry->AlertFlags;
			d->ServerPort = entry->ServerPort;
			AddTail((struct List *)&tr_dups, (struct Node *)d);
		}
		
		items++;
		
		data_len += entry->TaskNameLength;
		#if DATABASE_RESERVED
		data_len += DATABASE_RESERVED;
		#endif
	}
	
	data_len += (sizeof(TaskReg) * items) + (sizeof(ULONG)*3);
	DBG_VALUE(data_len);
	
	if((data = Malloc(data_len)))
	{
		STRPTR ptr=data;
		BPTR fd;
		
		PutV( ptr, ULONG, DATABASE_ID);
		PutV( ptr, ULONG, DATABASE_VERSION);
		
		ITERATE_LIST( G->TaskRegList, TaskReg *, entry)
		{
			DBG_STRING(entry->TaskName);
			
			if(IsDup(entry->TaskName,entry->AlertFlags,entry->ServerPort))
				continue;
			
			PutV( ptr, UWORD, entry->TaskNameLength );
			PutX( ptr, entry->TaskNameLength, entry->TaskName );
			
			PutV( ptr, BYTE, entry->allow );
			PutV( ptr, BYTE, entry->remember );
			
			PutV( ptr, ULONG, entry->RegTime.ds_Days );
			PutV( ptr, ULONG, entry->RegTime.ds_Minute );
			PutV( ptr, ULONG, entry->RegTime.ds_Tick );
			
			PutV( ptr, ULONG, entry->ModTime.ds_Days );
			PutV( ptr, ULONG, entry->ModTime.ds_Minute );
			PutV( ptr, ULONG, entry->ModTime.ds_Tick );
			
			PutV( ptr, ULONG, entry->accesses   );
			PutV( ptr, ULONG, entry->FileCRC    );
			PutV( ptr, UWORD, entry->CRCMods    );
			PutV( ptr, UWORD, entry->AlertFlags );
			PutV( ptr, UWORD, entry->ServerPort );
			
			#if DATABASE_RESERVED
			ptr += DATABASE_RESERVED;
			#endif
		}
		
		PutV( ptr, ULONG, DATABASE_EOFID );
		
		if((fd = Open( DATABASE_FILE, MODE_NEWFILE )))
		{
			LONG len = (LONG)(ptr-data);
			DBG_VALUE(len);
			
			transcode( data, len );
			
			rc = (Write( fd, data, len ) == len);
			Close(fd);
		}
		
		Free(data);
	}
	
	while((d = (struct DupsLog *)RemHead((struct List *)(&tr_dups))))
	{
		Free(d);
	}
	
	RETURN(rc);
	return(rc);
}

VOID RemTaskReg( VOID )
{
	ENTER();
	DBG_ASSERT(G && G->MagicID == GLOBAL_MAGIC);
	if(G && G->MagicID == GLOBAL_MAGIC && G->TaskRegList != NULL)
	{
		if(!IsMinListEmpty(G->TaskRegList))
		{
			if(!TaskRegSave())
			{
				LONG ierr = IoErr();
				
				tell("\033bError writing database to disk!\033n\n\n"
					"The system returned: \"%s\" (%ld)",
					ioerrstr(NULL,DOSBase), ierr );
			}
			#ifdef DEBUG
			else
			{
				TaskReg * e;
				
				ITERATE_LIST( G->TaskRegList, TaskReg *, e)
				{
					DBG("Task=\"%s\" A=%ld R=%ld T=%ld C=%ld F=%ld P=%ld\n",
						e->TaskName, e->allow, e->remember, e->accesses, e->CRCMods, e->AlertFlags, e->ServerPort );
				}
			}
			#endif /* DEBUG */
		}
		
		Free(G->TaskRegList);
		G->TaskRegList = NULL;
	}
	
	LEAVE();
}
