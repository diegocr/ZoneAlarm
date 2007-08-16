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

/** 
 * $Id: debug.c,v 0.1 2006/03/14 11:28:57 diegocr Exp $
 */

#ifdef DEBUG
# include <proto/exec.h>
# include <stdarg.h>
# include <SDI_compiler.h> /* REG() macro */

#if defined(__NOLIBBASE__)
GLOBAL struct Library * SysBase;
#endif

#ifndef RawPutChar
# define RawPutChar(CH)	LP1NR( 0x204, RawPutChar, ULONG, CH, d0,,SysBase)
#endif

VOID KPutC(UBYTE ch)
{
	RawPutChar(ch);
}

VOID KPutStr(CONST_STRPTR string)
{
	UBYTE ch;
	
	while((ch = *string++))
		KPutC( ch );
}

STATIC VOID ASM RawPutC(REG(d0,UBYTE ch))
{
	KPutC(ch); 
}

VOID KPrintF(CONST_STRPTR fmt, ...)
{
	va_list args;
	
	va_start(args,fmt);
	RawDoFmt((STRPTR)fmt, args,(VOID (*)())RawPutC, NULL );
	va_end(args);
}

#endif /* DEBUG */
