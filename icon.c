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

/* :ts=4 */
#include "global.h"
#include <proto/icon.h>
#include <workbench/startup.h>
#include "icon.h"

GLOBAL struct Library * DOSBase;
STATIC struct Library * IconBase = NULL;

/**
 * data allocated here get no freed at exit, remember we're using pools!
 */
//------------------------------------------------------------------------------

INLINE STRPTR SetProgFilename( VOID )
{
	struct WBStartup * _WBenchMsg = (struct WBStartup *) G->WBenchMsg;
	LONG size = 1024;
	BOOL done = FALSE;
	
	MAGIC_CHECK(NULL);
	
	if((G->ProgFilename = Malloc(size+1)))
	{
		if(_WBenchMsg)
		{
			if (NameFromLock(GetProgramDir(), G->ProgFilename, size ))
			{
				if (AddPart(G->ProgFilename, _WBenchMsg->sm_ArgList[0].wa_Name, size))
				{
					done = TRUE;
				}
			}
		}
		else
		{
			if (NameFromLock(GetProgramDir(), G->ProgFilename, size ))
			{
				UBYTE buf[128];
				
				if (GetProgramName( buf, sizeof(buf)-1))
				{
					if (AddPart(G->ProgFilename, buf, size ))
					{
						done = TRUE;
					}
				}
			}
		}
	}
	
	DBG_STRING(G->ProgFilename);
	
	if( done == FALSE )
	{
		Free(G->ProgFilename);
		G->ProgFilename = NULL;
	}
	
	return G->ProgFilename;
}

//------------------------------------------------------------------------------

STATIC STRPTR GetValue( STRPTR string, STRPTR outbuf, LONG outlen )
{
	UBYTE ech;
	
#if 0
	while(*string && *string++ != '=');
	
	// icon.library >= v44 allow spaces
	while(*string && *string == 0x20)
		string++;
#endif
	
	ech = ((*string == '\"') ? '\"' : 0x20);
	if(ech != 0x20)
		string++;
	
	while(*string && *string != ech && --outlen > 0)
	{
		*outbuf++ = *string++;
	}
	*outbuf = 0;
	
	return((outlen > 0) ? string : NULL);
}

//------------------------------------------------------------------------------

#define RANDOM_NUMBER \
({	ULONG num = ~0;   \
	num ^= CRC32( dobj, sizeof(struct DiskObject), num); \
	num &= ~(((ULONG)FindTask(NULL))>>15); num; \
})

STRPTR DataBaseFile( VOID )
{
	STRPTR rc = NULL;
	
	if((IconBase = OpenLibrary("icon.library", 0L)))
	{
		struct DiskObject * dobj;
		STRPTR icon;
		
		if((icon = SetProgFilename ( )) != NULL )
		{
			BPTR lock;
			BOOL missing = TRUE;
			UBYTE info[4096];
			
			SNPrintf( info, sizeof(info)-1, "%s.info", icon );
			
			// if my prog has no icon, write out a default one
			if(!(lock = Lock( info, SHARED_LOCK )))
			{
				DBG("Writing default icon...\n");
				
				lock = Open( info, MODE_NEWFILE);
				DBG_ASSERT(lock);
				if( lock )
				{
					Write( lock, (STRPTR)my_default_icon, sizeof(my_default_icon));
					Close( lock );
				}
			}
			else UnLock( lock );
			
			if((dobj = GetDiskObject( icon )))
			{
				STRPTR *arry = dobj->do_ToolTypes;
				
				while(arry && *arry)
				{
					STRPTR item = *arry++;
					
					if(item[0]=='*' && item[1]=='*' && item[2]=='*') break;
					
					// This MUST be the db file!
					{
						missing = FALSE;
						
						if((rc = Malloc( 512 )))
						{
							AddPart( rc, "ENVARC:", 500 );
							GetValue( item, &rc[sizeof("ENVARC:")-1], 500 );
						}
						DBG_STRING(rc);
						break;
					}
				}
				
				FreeDiskObject( dobj );
			}
			
			//if( rc == NULL  &&  IoErr() != ERROR_NO_FREE_STORE )
			if( missing == TRUE )
			{
				dobj = GetDiskObject( icon );
				DBG_ASSERT(dobj != NULL);
				
				if( dobj )
				{
					UWORD items = 1;
					STRPTR old_def_tool;
					STRPTR *ot = dobj->do_ToolTypes;
					STRPTR *old_tooltypes, *new_tooltypes;
					
					while(ot && *ot++) items++;
					
					new_tooltypes = Malloc(++items<<2);
					DBG_ASSERT(new_tooltypes);
					
					if( new_tooltypes )
					{
						D_S(struct FileInfoBlock,fib);
						STRPTR * nt;
						UBYTE def_dbtt[32], *dbfile;
						int len;
						
						old_def_tool = dobj->do_DefaultTool;
						old_tooltypes = dobj->do_ToolTypes;
						
						dobj->do_DefaultTool = NULL;
						
						nt = new_tooltypes;
						ot = old_tooltypes;
						
						dbfile = RandomString( def_dbtt, sizeof(def_dbtt)-1);
						len = StrLen(dbfile); DBG_VALUE(len);
						
						*nt++ = dbfile;
						DBG_STRING(dbfile);
						
						while(ot && *ot) {
							*nt++ = *ot++;
						}
						*nt = NULL;
						
						dobj->do_ToolTypes = new_tooltypes;
						dobj->do_StackSize = 16384;
						
						bZero( fib, sizeof(struct FileInfoBlock));
						
						// try to preserve icon attrs
						if((lock = Lock( info, SHARED_LOCK )))
						{
							if(Examine( lock, fib )!=DOSFALSE) missing = FALSE;
							UnLock( lock );
						}
						
						// finally, save icon
						PutDiskObject( icon, dobj );
						
						if( missing == FALSE )
						{
							SetProtection( info, fib->fib_Protection);
							SetFileDate( info, (struct DateStamp *)&fib->fib_Date);
							if(*fib->fib_Comment)
								SetComment( info, fib->fib_Comment );
						}
						
						Free( new_tooltypes );
						
						dobj->do_DefaultTool = old_def_tool;
						dobj->do_ToolTypes = old_tooltypes;
						
						len += sizeof("ENVARC:") + 2;
						if((rc = Malloc( len )))
						{
							if(!AddPart( rc, "ENVARC:", len )||!AddPart( rc, dbfile, len ))
							{
								Free(rc);
								rc = NULL;
							}
						}
					}
					
					FreeDiskObject( dobj );
				}
			}
		}
		
		CloseLibrary( IconBase );
	}
	
	if( rc == NULL )
	{
		ShowIoErrMessage("cannot obtain database file from icon");
	}
	DBG_STRING(rc);
	
	return rc;
}

//------------------------------------------------------------------------------

STRPTR DataBaseFileCached( VOID )
{
	STATIC STRPTR db = NULL;
	
	ENTER();
	
	if( db == NULL ) db = DataBaseFile ( ) ;
	
	RETURN(db);
	
	if(db == NULL)
		return("");
	return(db);
}

//------------------------------------------------------------------------------

