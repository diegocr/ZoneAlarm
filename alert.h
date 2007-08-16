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

#ifndef ZONEALARM_ALERT_H
#define ZONEALARM_ALERT_H

// manage which type of data is passed to alert()

#include "taskreg.h"

enum AlertFlag
{
	// patch type
	AF_SOCKET	= (1<< 1),
	AF_EXEC		= (1<< 2),
	
	// functions
	AF_CONNECT	= (1<< 5),
	AF_BIND		= (1<< 6),
	AF_SENDTO	= (1<< 7),
	AF_COLDREBOOT	= (1<<12),
};

enum AlertReturnCode
{
	ARC_FATAL = -1,
	ARC_DENY  =  0,
	ARC_ALLOW =  1,
};

typedef struct AlertData
{
	APTR data;
	TaskReg * tr;
	UWORD flags;
	BOOL remember;
	
} AlertData;

GLOBAL LONG alert(AlertData *ad);

#define ALERT_DATA( Flags, Data, Tr )	\
	struct AlertData ad;		\
	ad.flags = Flags;		\
	ad.data = Data;			\
	ad.tr = Tr;			\
	ad.remember = FALSE

#define SBP(ad) ((SocketBasePatch *)(((AlertData *)(ad))->data))

#endif /* ZONEALARM_ALERT_H */
