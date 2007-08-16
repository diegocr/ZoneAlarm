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


#include "pch_socket.h"
#include "taskreg.h"
#include <proto/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <amitcp/socketbasetags.h>
#include <errno.h>
#include "ipc.h"
#include "usema.h"
#include "alert.h"
#include "pchs.h"

STATIC LONG ASM za_connect
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase));
STATIC LONG ASM za_bind
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase));
STATIC LONG ASM za_sendto
(	REG(d0, LONG s),
	REG(a0, const unsigned char *buf),
	REG(d1, LONG len),
	REG(d2, LONG flags),
	REG(a1, struct sockaddr *to),
	REG(d3, LONG tolen),
	REG(a6, struct Library *SocketBase));

STATIC LONG ASM za_recv
(	REG(d0, long s),
	REG(a0, unsigned char *buf),
	REG(d1, long len),
	REG(d2, long flags),
	REG(a6, struct Library *SocketBase));
STATIC LONG ASM za_send
(	REG(d0, long s),
	REG(a0, unsigned char *buf),
	REG(d1, long len),
	REG(d2, long flags),
	REG(a6, struct Library *SocketBase));


STATIC VOID DeleteSocketPatch(SocketBasePatch * sbp);

//------------------------------------------------------------------------------

STATIC VOID SetNoAccessError(struct Library * SocketBase)
{
	SocketBaseTags(SBTM_SETVAL(SBTC_ERRNO), EACCES );
}

//------------------------------------------------------------------------------

// called from OpenLibrary() patch
BOOL InstallSocketPatches(struct Library *socketbase)
{
	SocketBasePatch * sbp;
	BOOL rc = FALSE;
	
	// this is executed uSemaLock()'ed
	
	ENTER();
	DBG_POINTER(socketbase);
	MAGIC_CHECK(FALSE);
	
	if((sbp = Malloc(sizeof(SocketBasePatch))))
	{
		if(( sbp->zat = zat_malloc ( )))
		{
			SefFuncWrapper Patches[] = {
				{ &(sbp->connect_pch),	-0x36, za_connect, 0},
				{ &(sbp->bind_pch),	-0x24, za_bind,    0},
				{ &(sbp->sendto_pch),	-0x3c, za_sendto,  0},
				{ NULL, 0, NULL, 0 }
			};
			
			if(sf_install( Patches, socketbase ))
			{
				sbp->base = socketbase;
				
				AddTail((struct List *)G->SocketPatches,(struct Node *)sbp);
				rc = TRUE;
			}
		}
	}
	
	if(rc == FALSE)
	{
		if(sbp)
		{
			zat_free(sbp->zat);
			Free(sbp);
		}
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

// called from CloseLibrary() patch
VOID RemoveSocketPatches(struct Library *socketbase)
{
	SocketBasePatch * sbp;
	
	// this is executed Forbid()'ed
	
//	ENTER();
//	DBG_POINTER(socketbase);
	MAGIC_CHECK( );
	
	ITERATE_LIST( G->SocketPatches, SocketBasePatch *, sbp )
	{
		if(sbp->base == socketbase)
		{
			DeleteSocketPatch(sbp);
			TaskRegSave();
			break;
		}
	}
	
//	LEAVE();
}

//------------------------------------------------------------------------------

// called at program shutdown
VOID DisposeSocketPatches( VOID )
{
	SocketBasePatch * sbp;
	
	ENTER();
	MAGIC_CHECK(/**/);
	
	Forbid();
	
	ITERATE_LIST( G->SocketPatches, SocketBasePatch *, sbp )
	{
		struct MinNode * succ = sbp->node.mln_Succ;
		
		DeleteSocketPatch(sbp);
		if(!(sbp = (SocketBasePatch *) succ))
			break;
	}
	
	Permit();
	
	LEAVE();
}

//------------------------------------------------------------------------------

STATIC VOID DeleteSocketPatch(SocketBasePatch * sbp)
{
	ULONG patches[] = {
		(ULONG) &(sbp->connect_pch),
		(ULONG) &(sbp->bind_pch),
		(ULONG) &(sbp->sendto_pch),
		0L
	};
	UWORD Flags = AF_SOCKET, fls[] = {
		AF_CONNECT, AF_BIND, AF_SENDTO, 0
	}, *f_p=fls;
	
	sf_remove((ULONG **)patches);
	
	/**
	 * Set this task(s) as no longer running from our reg
	 */
	while(*f_p)
	{
		Flags |= (*f_p);
		
		ClearTaskRegTaskByFlags( sbp->zat, Flags );
		
		Flags &= ~(*f_p);
		f_p++;
	}
	
	zat_free(sbp->zat);
	Remove((struct Node *)sbp);
	Free(sbp);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

INLINE SocketBasePatch * LookupSBP(struct Library *socketbase)
{
	SocketBasePatch * sbp, * rc = NULL;
	
	ITERATE_LIST( G->SocketPatches, SocketBasePatch *, sbp )
	{
		if(sbp->base == socketbase)
		{
			rc = sbp;
			break;
		}
	}
	
	return rc;
}

//------------------------------------------------------------------------------

typedef LONG ASM (*ConnectCB)
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase));

STATIC LONG ASM za_connect
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase))
{
	LONG rc = -1;
	SocketBasePatch * sbp;
	struct Library * DOSBase;
	
	TRACE_START;
	ENTER();
	MAGIC_CHECK(-1);
	
	Forbid();
	
	sbp = LookupSBP(SocketBase);
	DBG_ASSERT(sbp != NULL);
	if((sbp != NULL) && (DOSBase = OpenLibrary(__dosname,0)))
	{
		ConnectCB sys_connect;
		
		sbp->connect_pch->sf_Count++;
		sys_connect = sbp->connect_pch->sf_OriginalFunc;
		Permit();
		
		if(uSemaLock ( ))
		{
			BOOL allow = FALSE;
			LONG a_rc;
			TaskReg * tr;
			UWORD Flags = (AF_SOCKET|AF_CONNECT);
			
			a_rc = TaskRegRequest(tr,sbp->zat,Flags,0);
			
			if(a_rc == -1)
			{
				ALERT_DATA( Flags, sbp, tr );
				sbp->udata = (APTR) name;
				
				/**
				 * alert() returns -1 if the ReAction window
				 * cannot be opened.
				 */
				a_rc = alert( &ad );
				
				DBG_ASSERT(a_rc != -1);
				if( a_rc != -1 )
				{
					allow = (a_rc == 1);
					
					TaskRegUpdate(tr,sbp->zat,allow,&ad,0);
				}
			}
			else
			{
				DBG_ASSERT(a_rc==1||a_rc==0);
				
				allow = (a_rc ? TRUE : FALSE);
			}
			
			if(allow == TRUE)
			{
				#ifdef DEBUG
				struct sockaddr_in * sa = (struct sockaddr_in *) name;
				char * ip = (char *) &(sa->sin_addr);
				LONG port = (LONG) sa->sin_port;
				#define IC(ch) (((int)ch)&0xFF)
				
				DBG("Allowing access from task $%lx (%s) to host %ld.%ld.%ld.%ld:%ld\n",
					sbp->zat->task, sbp->zat->executable, IC(ip[0]),IC(ip[1]),IC(ip[2]),IC(ip[3]), port );
				
				#endif /* DEBUG */
				
				/**
				 * unlock our uSema due sys_connect() could take
				 * some time and may other tasks are waiting it
				 */
				uSemaUnLock ( ) ;
				
				rc = sys_connect( s, name, namelen, SocketBase );
			}
			else
			{
				uSemaUnLock ( ) ;
				
				SetNoAccessError( SocketBase );
			}
		}
		
		CloseLibrary( DOSBase );
		
		Forbid();
		sbp->connect_pch->sf_Count--;
	}
	
	Permit();
	
	RETURN(rc);
	TRACE_END;
	return(rc);
}

//------------------------------------------------------------------------------

typedef LONG ASM (*BindCB)
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase));

STATIC LONG ASM za_bind
(	REG(d0, LONG s),
	REG(a0, const struct sockaddr *name),
	REG(d1, LONG namelen),
	REG(a6, struct Library *SocketBase))
{
	LONG rc = -1;
	SocketBasePatch * sbp;
	struct Library * DOSBase;
	
	TRACE_START;
	ENTER();
	MAGIC_CHECK(-1);
	
	Forbid();
	
	sbp = LookupSBP(SocketBase);
	DBG_ASSERT(sbp != NULL);
	if((sbp != NULL) && (DOSBase = OpenLibrary(__dosname,0)))
	{
		BindCB sys_bind;
		
		sbp->bind_pch->sf_Count++;
		sys_bind = sbp->bind_pch->sf_OriginalFunc;
		Permit();
		
		if(uSemaLock ( ))
		{
			BOOL allow = FALSE;
			LONG a_rc;
			TaskReg * tr;
			UWORD Flags = (AF_SOCKET|AF_BIND);
			UWORD po = ((struct sockaddr_in *)name)->sin_port;
			
			a_rc = TaskRegRequest(tr,sbp->zat,Flags,po);
			
			if(a_rc == -1)
			{
				ALERT_DATA( Flags, sbp, tr );
				sbp->udata = (APTR) name;
				
				/**
				 * alert() returns -1 if the ReAction window
				 * cannot be opened.
				 */
				a_rc = alert( &ad );
				
				DBG_ASSERT(a_rc != -1);
				if( a_rc != -1 )
				{
					allow = (a_rc == 1);
					
					TaskRegUpdate(tr,sbp->zat,allow,&ad,po);
				}
			}
			else
			{
				DBG_ASSERT(a_rc==1||a_rc==0);
				
				allow = (a_rc ? TRUE : FALSE);
			}
			
			if(allow == TRUE)
			{
				#ifdef DEBUG
				struct sockaddr_in * sa = (struct sockaddr_in *) name;
				char * ip = (char *) &(sa->sin_addr);
				LONG port = (LONG) sa->sin_port;
				#define IC(ch) (((int)ch)&0xFF)
				
				DBG("Allowing task $%lx (%s) to act as a server on host %ld.%ld.%ld.%ld:%ld\n",
					sbp->zat->task, sbp->zat->executable, IC(ip[0]),IC(ip[1]),IC(ip[2]),IC(ip[3]), port );
				
				#endif /* DEBUG */
				
				uSemaUnLock ( ) ;
				
				rc = sys_bind( s, name, namelen, SocketBase );
			}
			else
			{
				uSemaUnLock ( ) ;
				
				SetNoAccessError( SocketBase );
			}
		}
		
		CloseLibrary( DOSBase );
		
		Forbid();
		sbp->bind_pch->sf_Count--;
	}
	
	Permit();
	
	RETURN(rc);
	TRACE_END;
	return(rc);
}

//------------------------------------------------------------------------------

typedef LONG ASM (*SendtoCB)
(	REG(d0, LONG s),
	REG(a0, const unsigned char *buf),
	REG(d1, LONG len),
	REG(d2, LONG flags),
	REG(a1, struct sockaddr *to),
	REG(d3, LONG tolen),
	REG(a6, struct Library *SocketBase));

STATIC LONG ASM za_sendto
(	REG(d0, LONG s),
	REG(a0, const unsigned char *buf),
	REG(d1, LONG len),
	REG(d2, LONG flags),
	REG(a1, struct sockaddr *to),
	REG(d3, LONG tolen),
	REG(a6, struct Library *SocketBase))
{
	LONG rc = -1;
	SocketBasePatch * sbp;
	struct Library * DOSBase;
	
	TRACE_START;
	ENTER();
	MAGIC_CHECK(-1);
	
	Forbid();
	
	sbp = LookupSBP(SocketBase);
	DBG_ASSERT(sbp != NULL);
	if((sbp != NULL) && (DOSBase = OpenLibrary(__dosname,0)))
	{
		SendtoCB sys_sendto;
		
		sbp->sendto_pch->sf_Count++;
		sys_sendto = sbp->sendto_pch->sf_OriginalFunc;
		Permit();
		
		if(uSemaLock ( ))
		{
			BOOL allow = FALSE;
			LONG a_rc;
			TaskReg * tr;
			UWORD Flags = (AF_SOCKET|AF_SENDTO);
			
			a_rc = TaskRegRequest(tr,sbp->zat,Flags,0);
			
			if(a_rc == -1)
			{
				ALERT_DATA( Flags, sbp, tr );
				sbp->udata = (APTR) to;
				
				/**
				 * alert() returns -1 if the ReAction window
				 * cannot be opened.
				 */
				a_rc = alert( &ad );
				
				DBG_ASSERT(a_rc != -1);
				if( a_rc != -1 )
				{
					allow = (a_rc == 1);
					
					TaskRegUpdate(tr,sbp->zat,allow,&ad,0);
				}
			}
			else
			{
				DBG_ASSERT(a_rc==1||a_rc==0);
				
				allow = (a_rc ? TRUE : FALSE);
			}
			
			if(allow == TRUE)
			{
				#ifdef DEBUG
				struct sockaddr_in * sa = (struct sockaddr_in *) to;
				char * ip = (char *) &(sa->sin_addr);
				LONG port = (LONG) sa->sin_port;
				#define IC(ch) (((int)ch)&0xFF)
				
				DBG("Allowing access from task $%lx (%s) to host %ld.%ld.%ld.%ld:%ld\n",
					sbp->zat->task, sbp->zat->executable, IC(ip[0]),IC(ip[1]),IC(ip[2]),IC(ip[3]), port );
				
				#endif /* DEBUG */
				
				uSemaUnLock ( ) ;
				
				rc = sys_sendto( s, buf, len, flags, to, tolen, SocketBase );
			}
			else
			{
				uSemaUnLock ( ) ;
				
				SetNoAccessError( SocketBase );
			}
		}
		
		CloseLibrary( DOSBase );
		
		Forbid();
		sbp->sendto_pch->sf_Count--;
	}
	
	Permit();
	
	RETURN(rc);
	TRACE_END;
	return(rc);
}

//------------------------------------------------------------------------------

