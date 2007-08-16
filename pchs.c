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

GLOBAL BOOL InstallExecPatches( VOID );
GLOBAL VOID RemoveExecPatches( VOID );

//-----------------------------------------------------------------------------

VOID RemovePatches( VOID )
{
	RemoveExecPatches();
	RemoveDosPatches();
}

//-----------------------------------------------------------------------------

BOOL InstallPatches( VOID )
{
	BOOL rc = FALSE;
	
	ENTER();
	
	if(InstallExecPatches())
	{
		if(InstallDosPatches())
		{
			rc = TRUE;
		}
	}
	
	if( rc == FALSE )
	{
		RemovePatches();
	}
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

BOOL sf_install(SefFuncWrapper *pchs, struct Library * base)
{
	BOOL error = FALSE, rc = FALSE;
	SefFuncWrapper * p;
	
	ENTER();
	Forbid();
	
	p = pchs;
	while( p->pch != NULL )
	{
		if((*(p->pch) = Malloc(sizeof(SetFunc))))
		{
			(*(p->pch))->sf_Func       = p->func;
			(*(p->pch))->sf_Library    = base;
			(*(p->pch))->sf_Offset     = p->offset;
			(*(p->pch))->sf_QuitMethod = SFQ_COUNT;
			
			if(!SFReplace(*(p->pch)))
			{
				error = TRUE;
				break;
			}
			else p->patched = TRUE;
		}
		p++;
	}
	
	if(!(rc = !error))
	{
		p=pchs;
		
		while( p->pch != NULL )
		{
			if(*(p->pch))
			{
				if(p->patched)
					SFRestore(*(p->pch));
				
				Free(*(p->pch));
			}
			p++;
		}
	}
	
	Permit();
	
	RETURN(rc);
	return(rc);
}

//-----------------------------------------------------------------------------

VOID sf_remove(ULONG ** pchs)
{
	ENTER();
	
	while(*pchs != 0L)
	{
		SetFunc * sf = (SetFunc *) (**pchs);
		
		if( sf != NULL )
		{
			SFRestore( sf );
			Free( sf );
		}
		pchs++;
	}
	
	LEAVE();
}

