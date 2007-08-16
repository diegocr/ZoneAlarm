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

#ifndef ZONEALARM_PCHS_H
#define ZONEALARM_PCHS_H

#include "SFPatch.h"

typedef struct SefFuncWrapper
{
	SetFunc ** pch;
	LONG offset;
	APTR func;
	BOOL patched;
	
} SefFuncWrapper;

GLOBAL BOOL sf_install(SefFuncWrapper *pchs, struct Library * base);
GLOBAL VOID sf_remove(ULONG ** pchs);



#endif /* ZONEALARM_PCHS_H */
