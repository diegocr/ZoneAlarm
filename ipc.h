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


#ifndef __IPC_H
#define __IPC_H

typedef enum
{
	IPCA_NOACTION,
	IPCA_QUIT,
	IPCA_ZONEALARMTASK,
} IPCACT_T;

typedef enum { IPCR_OK, IPCR_FAIL, IPCR_ABORTED } IPCRES_T;

struct IPCMsg
{
	struct Message  ipc_msg;
	unsigned long   ipc_ID;
	IPCACT_T        ipc_action;
	APTR            ipc_data;
	union
	{
		LONG            ipc_res_result;
		struct MsgPort *ipc_res_port;
	} ipc_res;
};

#define ipc_result	ipc_res.ipc_res_result
#define ipc_port	ipc_res.ipc_res_port
#define IPC_MAGIC	0x9ffff444


GLOBAL BOOL IPC_PutMsg( struct MsgPort *destino, IPCACT_T action, APTR *udata );
GLOBAL VOID IPC_Dispatch( struct MsgPort * mPort );

#endif /* __IPC_H */
