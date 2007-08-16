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

#ifndef ZONEALARM_TASKREG_H
#define ZONEALARM_TASKREG_H

/**
 * This "class" is used to register which tasks are allowed/denyed to access
 * 'privilegied resources' (ie: allowed access to the internet)
 */

typedef struct TaskReg
{
	struct MinNode node;
	
	struct Task * task;	/* This (ofcourse) is temporal to the current
				 * running time.
				 * NOTES:
				 * 1) It is NULL when the database is loaded at
				 *    startup, when a task matching 'TaskName'
				 *    is detected, this var contains it's 
				 *    address.
				 * 2) see below for more notes ..
				 */
	
	UWORD TaskNameLength;
	STRPTR TaskName;
	
	BOOL allow;			// I should agre to allow access?
	BOOL remember;			// did the user selected remember?
	
	struct DateStamp RegTime;	// when this task was first seen
	struct DateStamp ModTime;	// when this task was last seen
	ULONG accesses;			// times this task was seen
	
	ULONG FileCRC;			// This is the CRC32 of the executable
	UWORD CRCMods;			// times the crc value changed..
	
	UWORD AlertFlags;		// flags passed to alert() ..
	UWORD ServerPort;		// if AlertFlags has the AF_BIND flag
					// set, this contains at which port
	
} TaskReg;

//------------------------------------------------------------------------------

GLOBAL BOOL InitTaskReg( VOID );
GLOBAL VOID RemTaskReg( VOID );
GLOBAL BOOL TaskRegSave( VOID );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#define EqualString( str1, str2 )			\
({							\
	register char *s1 = (str1), *s2 = (str2);	\
	BOOL rc = FALSE;				\
							\
	while(*s1 == *s2++)				\
		if (*s1++ == 0) {			\
			rc = TRUE; break;		\
		}					\
	rc;						\
})

#define TASK_MATCH( tr, zat )	\
	(/*tr->task==zat->task||*/EqualString(tr->TaskName,zat->task_name))

#define ADD_TASKREG(tr)	\
	AddTail((struct List *)G->TaskRegList, (struct Node *)tr)

#define DBG_RECORD(tr,zat) \
	DBG(" +-+-+-+-+-+++ Found record for task \"%s\", allow is %s and " \
		"remember %s, %s, %s\n", \
		tr->TaskName, \
		tr->allow ? "ON":"OFF", \
		tr->remember ? "ON":"OFF", \
		!tr->task ? "tr->task is NULL":"this is a new attempt", \
		zat->execrc==tr->FileCRC ? "CRC is OK":"Program has changed")

#define DBG_RECORD_UPD(zat,allow,remember)	\
	DBG("....UPDATING record for task \"%s\", allow %s remember %s\n", \
		zat->task_name, allow ? "ON":"OFF", remember ? "ON":"OFF")
#define DBG_RECORD_NEW(zat,allow,remember)	\
	DBG("....ADDING record for task \"%s\", allow %s remember %s\n", \
		zat->task_name, allow ? "ON":"OFF", remember ? "ON":"OFF")

//------------------------------------------------------------------------------
/**
 * TaskRegRequest() is used when a task is requesting access.
 * 
 * RETURN:
 *	-1 if it is a new task an alert() must be used
 *	 0 if the task is NOT allowed to have access
 * 	 1 if the task IS allowed
 * 
 * NOTE: returning either 0 or 1 means the user selected the 
 *       'remember' checkbox previously (loaded from our database), OR if
 *       tr->task is != NULL means the task asked for access from the same
 *       running process already, therefore we accept to give access or no
 *       even if the user did not selected 'remember' checkbox.
 * 
 * NOTE: There could be the case where two tasks has the exact same name,
 *       hence if the taskname match, and tr->task (being != NULL) do not 
 *       match, we'll continue looking for a next entry matching, fallbacking
 *       to interpret it as a new task is no task address match.
 * 
 * *** this is executed uSemaLock()'ed ***
 */
#define TaskRegRequest( tr, zat, flags, port )		\
({							\
	TaskReg *entry;					\
	LONG rc = -1;					\
	tr = NULL;					\
	ITERATE_LIST( G->TaskRegList, TaskReg *, entry)	\
	{						\
		if(flags!=entry->AlertFlags)		\
			continue;			\
		if(TASK_MATCH( entry, zat ))		\
		{					\
			if(entry->task != NULL)		\
			{				\
			  if(entry->task != zat->task)	\
			  	continue;		\
			}				\
			if(entry->ServerPort != port)	\
				continue;		\
			tr = entry;			\
			break;				\
		}					\
	}						\
							\
	if( tr != NULL )				\
	{						\
		BOOL running = FALSE;			\
		DBG_RECORD(tr,zat);			\
		if(tr->task == NULL)			\
			tr->task = zat->task;		\
		else running = TRUE;			\
		if(tr->remember == TRUE||running)	\
		{					\
			if(zat->execrc==tr->FileCRC)	\
				rc = tr->allow;		\
							\
			DateStamp(&(tr->ModTime));	\
			tr->accesses++;			\
		}					\
	}						\
	rc;						\
})

//------------------------------------------------------------------------------
/**
 * This macro was created mainly to DeleteSocketBasePatch()
 * see pch_socket.c for further info
 */
#define ClearTaskRegTaskByFlags( zat, flags )		\
{							\
	TaskReg *entry;					\
	ITERATE_LIST( G->TaskRegList, TaskReg *, entry)	\
	{						\
		if((flags) != entry->AlertFlags)	\
			continue;			\
		if(entry->task == NULL)			\
			continue;			\
		if(TASK_MATCH( entry, zat ))		\
		{					\
		DBG("TaskReg addr $%lx (%s) cleared\n",	\
			entry->task, entry->TaskName);	\
			entry->task = NULL;		\
		}					\
	}						\
}

//------------------------------------------------------------------------------
/**
 * TaskRegUpdate is used after calling alert() to update OR add
 * a new register for the calling task.
 * 
 * *** this is executed uSemaLock()'ed ***
 */
#define TaskRegUpdate( tr, zat, IfAllow, ad, port )	\
({							\
	if( tr != NULL )				\
	{						\
	DBG_RECORD_UPD(zat,IfAllow,(ad)->remember);	\
		if(tr->task == NULL)			\
			tr->task = zat->task;		\
		DBG_ASSERT(tr->task == zat->task);	\
		DBG_ASSERT((ad)->flags==tr->AlertFlags);\
		DBG_ASSERT(tr->ServerPort == port);	\
		tr->remember = (ad)->remember;		\
		tr->allow = IfAllow;			\
		DateStamp(&(tr->ModTime));		\
		tr->accesses++;				\
		if(zat->execrc != tr->FileCRC)		\
		{					\
			tr->FileCRC = zat->execrc;	\
			tr->CRCMods++;			\
		}					\
	}						\
	else if((tr = Malloc(sizeof(TaskReg))))		\
	{						\
		UWORD tlen = zat->TaskNameLen;		\
	DBG_RECORD_NEW(zat,IfAllow,(ad)->remember);	\
							\
		if(tlen > 0)				\
			tr->TaskName = Malloc(tlen+1);	\
		DBG_ASSERT(tr->TaskName != NULL);	\
		if(!(tr->TaskName)) {			\
			Free(tr);			\
		} else {				\
			CopyMem(zat->task_name,	\
				tr->TaskName,tlen);	\
			tr->task = zat->task;		\
			tr->TaskNameLength = tlen;	\
			DateStamp(&(tr->RegTime));	\
			CopyMem( &(tr->RegTime),	\
				&(tr->ModTime),		\
			   sizeof(struct DateStamp));	\
			tr->remember = (ad)->remember;	\
			tr->allow = IfAllow;		\
			tr->accesses = 1;		\
			tr->FileCRC = zat->execrc;	\
			tr->CRCMods = 0;		\
			tr->AlertFlags = (ad)->flags;	\
			tr->ServerPort = port;		\
							\
			ADD_TASKREG(tr);		\
			DBG_POINTER(tr);		\
		}					\
	} else { DBG(" **** OUT OF MEMORY!!\a\n"); }	\
	tr;						\
})

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


#endif /* ZONEALARM_TASKREG_H */
