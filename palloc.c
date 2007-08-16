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


#include "global.h"

STATIC struct SignalSemaphore gSemaphore;
STATIC APTR __private_memory_pool = NULL;

#define MEMORY_MAGICID	0xef1cb0a3
#define MEMORY_FREEDID	0xffff8520

BOOL InitMemoryPool( VOID )
{
	InitSemaphore(&gSemaphore);
	
	__private_memory_pool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR, 4096, 512);
	
	return((__private_memory_pool != NULL) ? TRUE : FALSE);
}

VOID ClearMemoryPool( VOID )
{
	if(__private_memory_pool != NULL)
		DeletePool( __private_memory_pool );
}

APTR Malloc( ULONG size )
{
	ULONG *mem;
	
	if(((long)size) <= 0)
		return NULL;
	
	ObtainSemaphore(&gSemaphore);
	
	size += sizeof(ULONG) * 2 + MEM_BLOCKMASK;
	size &= ~MEM_BLOCKMASK;
	
	if((mem = AllocPooled(__private_memory_pool, size)))
	{
		*mem++ = MEMORY_MAGICID;
		*mem++ = size;
	}
	ReleaseSemaphore(&gSemaphore);
	
	return mem;
}

VOID Free(APTR mem)
{
	ULONG size,*omem=mem;
	
	if( ! mem ) return;
	
	ObtainSemaphore(&gSemaphore);
	
	// get the allocation size
	size = *(--omem);
	
	// be sure we allocated this memory...
	if(*(--omem) == MEMORY_MAGICID)
	{
		*omem = MEMORY_FREEDID;
		FreePooled(__private_memory_pool, omem, size );
	}
	else {
		DBG("Memory area 0x%08lx cannot be freed! (%s)\n\a", mem,
		   (*omem == MEMORY_FREEDID) ? "already freed":"isn't mine");
	}
	
	ReleaseSemaphore(&gSemaphore);
}

APTR Realloc( APTR old_mem, ULONG new_size )
{
	APTR new_mem;
	ULONG old_size,*optr = old_mem;
	
	if(!old_mem) return Malloc( new_size );
	
	if(*(optr-2) != MEMORY_MAGICID)
	{
		DBG("Memory area 0x%08lx isn't known\n", old_mem);
		return NULL;
	}
	
	old_size = (*(optr-1)) - (sizeof(ULONG)*2);
	if(new_size <= old_size)
		return old_mem;
	
	if((new_mem = Malloc( new_size )))
	{
		CopyMem( old_mem, new_mem, old_size);
		Free( old_mem );
	}
	
	return new_mem;
}

VOID bZero( APTR data, ULONG size )
{
	register unsigned long * uptr = (unsigned long *) data;
	register unsigned char * sptr;
	
	// first blocks of 32 bits
	while(size >= sizeof(ULONG))
	{
		*uptr++ = 0;
		size -= sizeof(ULONG);
	}
	
	sptr = (unsigned char *) uptr;
	
	// now any pending bytes
	while(size-- > 0)
		*sptr++ = 0;
}

#ifdef TEST
int main()
{
	if(InitMemoryPool())
	{
		APTR data;
		
		if((data = Malloc(1025)))
		{
			Free(data);
			Free(data);
		}
		
		ClearMemoryPool();
	}
}
#endif
