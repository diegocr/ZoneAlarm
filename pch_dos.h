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

#ifndef ZONEALARM_PCH_DOS_H
#define ZONEALARM_PCH_DOS_H

typedef struct CreateNewProcPatch
{
	struct MinNode node;
	
	struct Task * parent;
	struct Process * newproc;
	
} CreateNewProcPatch;

GLOBAL BOOL InstallDosPatches( VOID );
GLOBAL VOID RemoveDosPatches( VOID );

GLOBAL struct Task * NewProcSearch(struct Task * task);
GLOBAL VOID NewProcRemove(struct Task *task);

#endif /* ZONEALARM_PCH_DOS_H */
