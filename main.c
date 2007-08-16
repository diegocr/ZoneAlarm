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
#include "ipc.h"
#include "taskreg.h"
#include "usema.h"
#include <proto/wb.h>

//------------------------------------------------------------------------------

#define PROGNAME	"ZoneAlarm"
#define PROGVERZ	"1.0"
#define PROGDATE	"25.03.2007"

//------------------------------------------------------------------------------

UBYTE __dosname[] = "dos.library";

BOOL running = TRUE; /* used at ipc.c as well */

GLOBAL BOOL InstallPatches( VOID );
GLOBAL VOID RemovePatches( VOID );
GLOBAL BOOL SafeToExit( VOID );

GLOBAL struct Library * DOSBase;

//------------------------------------------------------------------------------
/**
 * <Loop>	Looping function where task events are received/dispatched
 */

INLINE VOID Loop ( VOID )
{
	ULONG sigs,
		mPortSig = (1L << G->mPort->mp_SigBit),
		AppSigMask = (1L << G->mWbPort->mp_SigBit),
	TaskSignals=(G->tm->sigmask|SIGBREAKF_CTRL_C|AppSigMask|mPortSig);
	
	while(TRUE)
	{
		while(running)
		{
			sigs = Wait( TaskSignals );
			
			if( sigs & mPortSig )
			{
				IPC_Dispatch(G->mPort);
			}
			
			if( sigs & G->tm->sigmask )
			{
				ThreadManagerDispatcher(G->tm);
			}
			
			if( sigs & AppSigMask )
			{
				APTR msg;
				
				while((msg = GetMsg(G->mWbPort)))
				{
					DBG(" ++++++++++++++++++++ GOT APPITEM MESSAGE\n");
					
					ReplyMsg( msg );
				}
			}
			
			if(sigs & SIGBREAKF_CTRL_C)
				running = FALSE;
		}
		
		if(SafeToExit()) break;
		
		running = TRUE;
	}
}

//------------------------------------------------------------------------------
/**
 * <Main_4>	Final main function to set-up the rest needed..
 */

INLINE LONG Main_4 ( VOID )
{
	LONG rc = RETURN_ERROR;
	
	if( InitTaskReg ( ))
	{
		if(uSemaInit ( ))
		{
			if(InstallPatches())
			{
				Loop ( ) ;
				
			/**
			 * There could be the case where some patched
			 * function owns the uSema or are waiting for
			 * it, and RemovePatches() being called could
			 * cause to wait forever,to try to solve this
			 * situation uSemaKill have been created.
			 */
				uSemaKill ( ) ;
				
				RemovePatches();
				rc = 0;
			}
			else
			{
				ShowIoErrMessage("Cannot install supervisor code");
			}
			
			uSemaClear ( ) ;
		}
		else OutOfMemory(NULL);
		
		RemTaskReg ( ) ;
	}
	
	return(rc);
}

//------------------------------------------------------------------------------
/**
 * <Main_3>	Setup Workbench's tools menu item
 */

INLINE LONG Main_3 ( VOID )
{
	LONG rc = -1;
	struct Library * WorkbenchBase;
	
	if((WorkbenchBase = OpenLibrary("workbench.library",37)))
	{
		if((G->mWbPort = CreateMsgPort()))
		{
			struct AppMenuItem *appitem;
			struct Message * msg;
			
			appitem = AddAppMenuItemA(0L,NULL,PROGNAME,G->mWbPort,NULL);
			
			if( appitem )
			{
				rc = Main_4 ( ) ;
				
				RemoveAppMenuItem(appitem);
			}
			
			Forbid();
			while((msg = GetMsg(G->mWbPort)))
				ReplyMsg( msg );
			DeleteMsgPort( G->mWbPort );
			Permit();
		}
		
		CloseLibrary(WorkbenchBase);
	}
	
	if( rc == -1 )
	{
		ShowIoErrMessage("Failed to add AppMenuItem");
		rc = RETURN_ERROR;
	}
	
	return(rc);
}

//------------------------------------------------------------------------------
/**
 * <Main_2>	Function to set-up our public message port where other 
 * 		tasks could communicate with us
 */

INLINE LONG Main_2 ( VOID )
{
	LONG rc = RETURN_ERROR;
	
	if(!(G->mPort = CreateMsgPort()))
	{
		ShowIoErrMessage("Failed to create message port");
	}
	else
	{
		G->mPort->mp_Node.ln_Name = PROGNAME;
		G->mPort->mp_Node.ln_Pri = 0;
		
		AddPort(G->mPort);
		
		rc = Main_3 ( ) ;
		
		RemPort(G->mPort);
		
		Forbid();
		IPC_Dispatch(G->mPort);
		DeleteMsgPort(G->mPort);
		G->mPort = NULL;
		Permit();
	}
	
	return(rc);
}

//------------------------------------------------------------------------------
/**
 * <Main_1>	Function to set-up our threads system
 */

INLINE LONG Main_1 ( VOID )
{
	LONG rc = RETURN_ERROR;
	
	if(!(G->tm = InitThreadManager(0)))
	{
		ShowIoErrMessage("Failed to create multi-threaded system");
	}
	else
	{
		rc = Main_2 ( ) ;
		
		CleanupThreadManager ( G->tm ) ;
	}
	
	return(rc);
}

//------------------------------------------------------------------------------
/**
 * <Main>  Main function called from startup code
 */

LONG Main(APTR _WBenchMsg UNUSED)
{
	LONG rc = RETURN_ERROR;
	
	ENTER();
	MAGIC_CHECK(rc);
	
	Forbid();
	G->mPort = FindPort(PROGNAME);
	Permit();
	
	if(G->mPort != NULL) // already running?
	{
		APTR data = NULL;
		
		// tell to quit
		
		if(!IPC_PutMsg( G->mPort, IPCA_QUIT, &data) || data != (APTR)IPCA_QUIT)
		{
			ShowIoErrMessage("cannot tell main task to quit");
		}
		else
		{
			Delay(20);
			rc = RETURN_OK;
		}
	}
	else
	{
		rc = Main_1 ( ) ;
	}
	
	RETURN(rc);
	return rc;
}

//------------------------------------------------------------------------------

STATIC CONST UBYTE __VerID[] = 
	"$VER:" PROGNAME " " PROGVERZ " " PROGDATE " (C)2007 Diego Casorran";

STRPTR ProgramName( VOID )
{
	return((STRPTR)(PROGNAME));
}

#if 0
STRPTR ProgramVersion( VOID )
{
	return((STRPTR)(PROGVERZ));
}

STRPTR ProgramVersionTag( VOID )
{
	return((STRPTR)(__VerID));
}

STRPTR ProgramDate( VOID )
{
	return((STRPTR)(PROGDATE));
}
#endif

