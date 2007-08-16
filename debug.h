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
 * $Id: debug.h,v 0.1 2006/03/14 11:28:57 diegocr Exp $
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
# include <clib/debug_protos.h>
GLOBAL VOID KPutC(UBYTE ch);

static __inline void __dEbuGpRInTF( char * func_name, long line )
{
	KPrintF("[%s] %ld:%s: ",FindTask(NULL)->tc_Node.ln_Name,line,func_name);
}
# define DBG( fmt... ) \
({ \
	__dEbuGpRInTF(__PRETTY_FUNCTION__, __LINE__); \
	KPrintF( fmt ); \
})
# define DBG_POINTER( ptr )	DBG("%s = 0x%08lx\n", #ptr, (long) ptr )
# define DBG_VALUE( val )	DBG("%s = 0x%08lx,%ld\n", #val,val,val )
# define DBG_STRING( str )	DBG("%s = 0x%08lx,\"%s\"\n", #str,str,str)
# define DBG_ASSERT( expr )	if(!( expr )) DBG(" **** FAILED ASSERTION '%s' ****\a\n", #expr )
# define TRACE_LINE(ch)					\
({	int i = 0; KPutC('\n'); KPutC('\n');		\
	__dEbuGpRInTF(__PRETTY_FUNCTION__, __LINE__);	\
	while(i++<40)KPutC(ch);KPutC('\n');KPutC('\n');	\
})
# define TRACE_START	TRACE_LINE('>')
# define TRACE_END	TRACE_LINE('<')
#else
# define DBG(fmt...)		((void)0)
# define DBG_POINTER( ptr )	((void)0)
# define DBG_VALUE( ptr )	((void)0)
# define DBG_STRING( str )	((void)0)
# define DBG_ASSERT( expr )	((void)0)
# define TRACE_START		((void)0)
# define TRACE_END		((void)0)
#endif

#define ENTER()		DBG("--- ENTERING %s:%ld\n", __func__, __LINE__)
#define LEAVE()		DBG("--- LEAVING %s:%ld\n", __func__, __LINE__)
#define RETURN(Rc)	\
	DBG("--- LEAVING %s:%ld RC=%ld,0x%08lx\n", __func__, __LINE__, (LONG)(Rc),(LONG)(Rc))

#endif /* DEBUG_H */
