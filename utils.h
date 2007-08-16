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

#ifndef ZONEALARM_UTILS_H
#define ZONEALARM_UTILS_H

#include <stdarg.h>
#include <exec/types.h>

GLOBAL LONG RA_Requester( STRPTR GadgetText, STRPTR BodyText );
GLOBAL LONG RA_RequesterFMT(STRPTR GadgetText, STRPTR BodyTextFmt, ... );

GLOBAL LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args);
GLOBAL LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... );

GLOBAL STRPTR FileToMem( STRPTR FileName, struct FileInfoBlock * fib, struct Library * DOSBase);

GLOBAL ULONG CRC32(REG(d0, APTR buffer), REG(a0, ULONG buffer_length), REG(a1, ULONG crc));
GLOBAL ULONG CRC32OfFile( STRPTR FileName, struct Library * DOSBase );
GLOBAL STRPTR RandomString( UBYTE *buf, LONG buflen );
GLOBAL VOID transcode( STRPTR data, LONG len );

GLOBAL STRPTR GetProgramVersion(STRPTR full_path_to_program);

GLOBAL BOOL IsTaskRunning(struct Task * qTask);

//GLOBAL STRPTR AslFile( STRPTR TitleText, STRPTR InitialPattern );


GLOBAL int StrLen(const char *string);
GLOBAL int StrCmp(const char *s1, const char * s2);
GLOBAL int StrnCmp(const char *s1,const char *s2,ULONG n);

GLOBAL char *ioerrstr( char *prompt, struct Library * DOSBase );
GLOBAL VOID OutOfMemory(STRPTR where);
GLOBAL VOID CannotOpenMsg(STRPTR what, ULONG version);
GLOBAL VOID ShowIoErrMessage( STRPTR prompt );

GLOBAL UBYTE __intuitionname[],__requestername[];
GLOBAL UBYTE ask_gadgets[], tell_gadgets[];

#define tell( fmt... )	\
	RA_RequesterFMT(tell_gadgets, fmt )

#define ask( fmt... )	\
	RA_RequesterFMT(ask_gadgets, fmt )


#endif /* ZONEALARM_UTILS_H */
