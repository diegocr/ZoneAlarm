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


#ifndef HEAVY_DEBUG
# undef DEBUG
#endif

#include "global.h"
#include "ipc.h"
#include "za_task.h"

GLOBAL BOOL running;
GLOBAL struct Library * DOSBase;
//------------------------------------------------------------------------------

BOOL IPC_PutMsg( struct MsgPort *destino, IPCACT_T action, APTR *udata )
{
	struct MsgPort *replyport = NULL;
	BOOL error = TRUE;
	
	ENTER();
	DBG_POINTER(destino);
	
	if(destino && (replyport = CreateMsgPort()))
	{
		struct IPCMsg ipcmsg;
		APTR xMsg;
		
		ipcmsg.ipc_msg.mn_ReplyPort	= replyport;
		ipcmsg.ipc_msg.mn_Length	= sizeof(struct IPCMsg);
		ipcmsg.ipc_ID			= IPC_MAGIC;
		ipcmsg.ipc_action		= action;
		ipcmsg.ipc_result		= IPCR_ABORTED;
		ipcmsg.ipc_data			= udata ? (*udata) : NULL;
		
		DBG("Sending action '%ld' from %lx to %lx\n", action, replyport, destino);
		
		Forbid();
		PutMsg( destino, &ipcmsg.ipc_msg);
		WaitPort(replyport);
		while((xMsg = GetMsg( replyport )))
		{
			DBG("Got reply...\n");
			
			switch(((struct IPCMsg *)xMsg)->ipc_result)
			{ // TODO
				case IPCR_ABORTED:
					if(udata) (*udata) = NULL;
					DBG("IPCR_ABORTED\n");
					break;
				
				case IPCR_FAIL:
					if(udata) (*udata) = NULL;
					DBG("IPCR_FAIL\n");
					break;
				
				case IPCR_OK:
					if(udata) (*udata) = ipcmsg.ipc_data;
					DBG("IPCR_OK\n");
					break;
				default:
					break;
			}
		}
		Permit();
		
		DeleteMsgPort(replyport);
		
		error = FALSE;
	}
	
	LEAVE();
	
	return !error;
}

//------------------------------------------------------------------------------

VOID IPC_Dispatch( struct MsgPort * mPort )
{
	struct IPCMsg * ipcmsg;
	
	ENTER();
	DBG_POINTER(mPort);
	
	// get the next waiting message
	while((ipcmsg = (struct IPCMsg *)GetMsg( mPort )))
	{
		DBG_VALUE(ipcmsg->ipc_ID);
		
		if(ipcmsg->ipc_ID == IPC_MAGIC)
		{
			switch( ipcmsg->ipc_action )
			{
				case IPCA_QUIT:
					running = FALSE;
					ipcmsg->ipc_data = (APTR) IPCA_QUIT;
					ipcmsg->ipc_result = IPCR_OK;
					break;
				
				case IPCA_ZONEALARMTASK:
				{
					ZoneAlarmTask * zat;
					
					zat = zat_malloc_SAFE((struct Task *)ipcmsg->ipc_data);
					
					ipcmsg->ipc_data = (APTR) zat;
					ipcmsg->ipc_result = IPCR_OK;
				}	break;
				
				default:
					break;
			}
		}
		
		ReplyMsg((APTR) ipcmsg );
	}
	
	LEAVE();
}
