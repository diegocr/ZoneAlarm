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

#ifndef ZONEALARM_ZA_TASK_H
#define ZONEALARM_ZA_TASK_H

#include "xvs.h"

typedef struct ZoneAlarmTask
{
	struct MinNode node;		/* node to link to G->ZoneAlarmTasks */
	
	struct Task *task;		/* task where the patch was installed */
	struct Task *parent;		/* if it is a subprocess, there is 
					 * the parent */
	STRPTR task_name;		/* name of the task (not the parent,
					 * if it's a subprocess) */
	STRPTR executable;		/* full path pointing to the executable
					 * program. (if it's a subprocess, thats
					 * the parent program, ofcourse) */
	
	UWORD TaskNameLen;		/* This is here JUST to avoid using
					 * StrLen() from TaskRegUpdate() */
	
	ULONG execrc;			/* CRC32 checksum of the executable */
	STRPTR exever;			/* $VER: string of the executable */
	
	xvs_t xvs;			/* return code for the virus check */
	STRPTR virus;			/* if a virus found, its name */
	
} ZoneAlarmTask;

GLOBAL ZoneAlarmTask * zat_malloc( VOID );
GLOBAL VOID zat_free(ZoneAlarmTask * zat);
GLOBAL ZoneAlarmTask * zat_copy(ZoneAlarmTask *zat);

// do NOT use THIS function! (see note at the .c)
GLOBAL ZoneAlarmTask * zat_malloc_SAFE(struct Task *task);

//------------------------------------------------------------------------------

/* compatible struct to obtain the zat data */

struct xyz_magic_zat
{
	struct MinNode node;
	ZoneAlarmTask * zat;
	/* ........ */
};

#define ZAT(x)	(((struct xyz_magic_zat *)(x))->zat)




#endif /* ZONEALARM_ZA_TASK_H */
