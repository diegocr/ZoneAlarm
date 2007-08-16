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

;/* execute me to compile :ts=5
gcc -noixemul -m68020 -O -W -Wall -Winline xvs.c -o xvs -s -DDEBUG -DTEST
quit 0
*/

#include "global.h"
#include "xvs.h"
#include <proto/xvs.h>
#include <proto/xfdmaster.h>

//------------------------------------------------------------------------------

xvs_t xvs_analyzer( APTR data, ULONG data_length, STRPTR *virus )
{
	struct Library * xvsBase;
	ULONG r;
	UBYTE rc = XVS_NOTAVAIL;
	
	ENTER();
	
	if((xvsBase = OpenLibrary("xvs.library", 33)))
	{
		struct xvsFileInfo *xvs;
		
		rc = XVS_INTERR;
		if((xvs=(struct xvsFileInfo *)xvsAllocObject(XVSOBJ_FILEINFO)))
		{
			struct xfdMasterBase *xfdMasterBase;
			
			if((xfdMasterBase=(struct xfdMasterBase *)OpenLibrary("xfdmaster.library", 39)))
			{
				struct xfdLinkerInfo *xfd;
				
				if((xfd = (struct xfdLinkerInfo *)xfdAllocObject(XFDOBJ_LINKERINFO)))
				{
				/**
				 * XFD WARNING:
				 * After a call to xfdUnlink(), the contents of xfdli_Buffer are
				 * under no circumstances valid any longer. The only way to make use
				 * of the buffer contents is via xfdli_Save1 and xfdli_Save2.
				 * But if you have finished work with the splitted files, you have
				 * to release exactly the memory you have allocated for xfdli_Buffer.
				 */
					xfd->xfdli_Buffer=data;
					xfd->xfdli_BufLen=data_length;
					
					if(xfdRecogLinker( xfd ))
					{
						// at this point MAY we should use xfdStripHunks() ??
						
						DBG_STRING(xfd->xfdli_LinkerName);
						
						if(xfdUnlink( xfd ))
						{
							DBG("Data have been unlinked, checking...\n");
							
							xvs->xvsfi_FileLen = xfd->xfdli_SaveLen1;
							xvs->xvsfi_File = xfd->xfdli_Save1;
							
							r = xvsCheckFile(xvs);
							
							if(!(r==XVSFT_DATAVIRUS||r==XVSFT_LINKVIRUS))
							{
								/**
								 * there is no VIRUS on the first chunk,
								 * analize the next one.
								 */
								
								DBG("Checking segment #2...\n");
								
								xvs->xvsfi_FileLen = xfd->xfdli_SaveLen2;
								xvs->xvsfi_File = xfd->xfdli_Save2;
								
								r = xvsCheckFile(xvs);
							}
							
							if(r==XVSFT_DATAVIRUS||r==XVSFT_LINKVIRUS)
								rc = XVS_VIRUSDETECTED;
						}
					}
					
					xfdFreeObject(xfd);
				}
				
				CloseLibrary((struct Library *)xfdMasterBase);
			}
			
			if( rc == XVS_INTERR )
			{
				/**
				 * If we are here, data is unknown to XFD of
				 * we was unable to use xfd at all, check what
				 * happens passing the data directly to XVS.
				 */
				
				xvs->xvsfi_FileLen = data_length;
				xvs->xvsfi_File = data;
				
				DBG("Unable to use XFD..(or not a linked file)\n");
				
				r = xvsCheckFile(xvs);
				
				if(r==XVSFT_DATAVIRUS||r==XVSFT_LINKVIRUS)
					rc = XVS_VIRUSDETECTED;
			}
			
			if( rc == XVS_VIRUSDETECTED )
			{
				/* get the virus name */
				
				DBG_STRING(xvs->xvsfi_Name);
				
				if( virus != NULL )
				{
					ULONG len = StrLen(xvs->xvsfi_Name);
					
					if(((*virus) = Malloc( len+1 )))
					{
						CopyMem( xvs->xvsfi_Name, (*virus), len );
					}
				}
			}
			else rc = XVS_FILEOK;
			
			xvsFreeObject((APTR) xvs );
		}
		
		CloseLibrary( xvsBase );
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------

#ifdef TEST
#include "debug.c"

static const unsigned char Xdata[504] = { /* filetime program */
0x00,0x00,0x03,0xF3,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x68,0x00,0x00,0x00,0x02,0x00,0x00,
0x03,0xE9,0x00,0x00,0x00,0x68,0x61,0x5C,0x4E,0x75,0x2F,0x03,0x2F,0x02,0x20,
0x6F,0x00,0x0C,0x20,0x10,0x4C,0x3C,0x08,0x00,0x00,0x01,0x51,0x80,0x72,0x3C,
0x4C,0x28,0x18,0x00,0x00,0x04,0xD0,0x81,0x22,0x28,0x00,0x08,0x76,0x32,0x4C,
0x43,0x18,0x01,0xD0,0x81,0x24,0x1F,0x26,0x1F,0x4E,0x75,0x64,0x6F,0x73,0x2E,
0x6C,0x69,0x62,0x72,0x61,0x72,0x79,0x00,0x46,0x49,0x4C,0x45,0x4E,0x41,0x4D,
0x45,0x2F,0x41,0x2C,0x44,0x49,0x46,0x46,0x2F,0x53,0x00,0x25,0x6C,0x75,0x0A,
0x00,0x66,0x69,0x6C,0x65,0x74,0x69,0x6D,0x65,0x00,0xDE,0xFC,0xFE,0xDC,0x48,
0xE7,0x3D,0x36,0x78,0x0A,0x47,0xEC,0x80,0x02,0x26,0xB8,0x00,0x04,0x2C,0x53,
0x43,0xF8,0x00,0x00,0x4E,0xAE,0xFE,0xDA,0x24,0x40,0x4A,0xAA,0x00,0xAC,0x67,
0x00,0x00,0xDC,0x45,0xEC,0x80,0x06,0x2C,0x53,0x43,0xFA,0xFF,0xA6,0x42,0x80,
0x4E,0xAE,0xFD,0xD8,0x24,0x80,0x67,0x00,0x00,0xF6,0x42,0xAF,0x00,0x28,0x42,
0xAF,0x00,0x2C,0x4B,0xEF,0x00,0x28,0x2C,0x40,0x22,0x3C,0x00,0x00,0x00,0x3E,
0x24,0x0D,0x42,0x83,0x4E,0xAE,0xFC,0xE2,0x2A,0x00,0x67,0x7A,0x2C,0x52,0x22,
0x2F,0x00,0x28,0x74,0xFE,0x4E,0xAE,0xFF,0xAC,0x2E,0x00,0x67,0x60,0x76,0x3F,
0xD6,0x8F,0x70,0xFC,0xC6,0x80,0x2C,0x52,0x22,0x07,0x24,0x03,0x4E,0xAE,0xFF,
0x9A,0x4A,0x80,0x67,0x42,0x20,0x43,0x48,0x68,0x00,0x84,0x61,0x00,0xFF,0x1C,
0x58,0x8F,0x28,0x00,0x4A,0xAD,0x00,0x04,0x67,0x18,0x74,0x30,0xD4,0x8F,0x2C,
0x52,0x22,0x02,0x4E,0xAE,0xFF,0x40,0x2F,0x02,0x61,0x00,0xFF,0x00,0x58,0x8F,
0x90,0x84,0x28,0x00,0x2F,0x44,0x00,0x24,0x2C,0x52,0x22,0x3C,0x00,0x00,0x00,
0x50,0x74,0x24,0xD4,0x8F,0x4E,0xAE,0xFC,0x46,0x42,0x84,0x2C,0x52,0x22,0x07,
0x4E,0xAE,0xFF,0xA6,0x2C,0x6C,0x80,0x06,0x22,0x05,0x4E,0xAE,0xFC,0xA6,0x70,
0x0A,0xB0,0x84,0x66,0x18,0x45,0xEC,0x80,0x06,0x2C,0x52,0x4E,0xAE,0xFF,0x7C,
0x2C,0x52,0x22,0x00,0x24,0x3C,0x00,0x00,0x00,0x55,0x4E,0xAE,0xFE,0x26,0x2C,
0x53,0x22,0x6C,0x80,0x06,0x4E,0xAE,0xFE,0x62,0x60,0x30,0xD4,0xFC,0x00,0x5C,
0x2C,0x53,0x20,0x4A,0x4E,0xAE,0xFE,0x80,0x2C,0x53,0x20,0x4A,0x4E,0xAE,0xFE,
0x8C,0x24,0x00,0x2C,0x53,0x2E,0x3C,0x00,0x08,0x80,0x31,0x4E,0xAE,0xFF,0x94,
0x2C,0x53,0x4E,0xAE,0xFF,0x7C,0x2C,0x53,0x22,0x42,0x4E,0xAE,0xFE,0x86,0x20,
0x04,0x4C,0xDF,0x6C,0xBC,0xDE,0xFC,0x01,0x24,0x4E,0x75,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x03,0xEC,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0xAA,0x00,0x00,0x01,0x14,0x00,0x00,0x01,0x4A,0x00,0x00,0x00,0x00,
0x00,0x00,0x03,0xF2,0x00,0x00,0x03,0xEA,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xF2 };

const unsigned char data[1024] = { /* "Crackright" BootBlock VIRUS */
0x44,0x4F,0x53,0x00,0x40,0xB3,0x15,0x60,0x00,0x00,0x00,0x00,0x2C,0x79,0x00,
0x00,0x00,0x04,0x4E,0xAE,0xFF,0x88,0x42,0xB9,0x00,0xDF,0xF0,0x36,0x13,0xFC,
0x00,0xE0,0x00,0xDF,0xF0,0x34,0x30,0x3C,0x00,0x0F,0x51,0xC8,0xFF,0xFE,0x0C,
0x79,0x01,0x01,0x00,0xDF,0xF0,0x0C,0x66,0x14,0x20,0x3C,0x00,0x05,0xFF,0xFF,
0x53,0x80,0x66,0xFC,0x13,0xFC,0x00,0xA0,0x00,0xDF,0xF0,0x34,0x60,0x78,0x4E,
0xAE,0xFF,0x82,0x20,0x6E,0x00,0x2A,0x0C,0x68,0x42,0x4D,0x00,0x14,0x67,0x68,
0x2C,0x79,0x00,0x00,0x00,0x04,0x20,0x3C,0x00,0x00,0x07,0xD0,0x22,0x3C,0x00,
0x00,0x00,0x02,0x4E,0xAE,0xFF,0x3A,0x41,0xFA,0x03,0x5E,0x20,0x80,0x67,0x4A,
0x22,0x50,0x24,0x49,0x41,0xFA,0xFF,0x82,0x30,0x3C,0x01,0x00,0x22,0xD8,0x51,
0xC8,0xFF,0xFC,0x4E,0xEA,0x00,0x8E,0x41,0xFA,0x01,0xCC,0x43,0xFA,0x02,0xFC,
0x22,0xC8,0x41,0xFA,0x01,0xD8,0x22,0xC8,0x41,0xFA,0x00,0xB0,0x22,0xC8,0x41,
0xFA,0x00,0x34,0x22,0xC8,0x41,0xFA,0x02,0x00,0x22,0xC8,0x41,0xFA,0x03,0x50,
0x42,0x98,0x42,0x90,0x61,0x00,0x02,0x34,0x41,0xFA,0x03,0x16,0x52,0x90,0x43,
0xFA,0x02,0xEC,0x2C,0x79,0x00,0x00,0x00,0x04,0x4E,0xAE,0xFF,0xA0,0x20,0x40,
0x20,0x68,0x00,0x16,0x70,0x00,0x4E,0x75,0x48,0xE7,0xFF,0xFC,0x2C,0x79,0x00,
0x00,0x00,0x04,0x61,0x00,0x01,0xFC,0x6D,0x5C,0x41,0xFA,0x03,0x1A,0x52,0x90,
0x20,0x3A,0x03,0x10,0x02,0x80,0x00,0x00,0x00,0x03,0x67,0x0E,0x0C,0x90,0x00,
0x01,0x5F,0x90,0x66,0x42,0x4E,0xF9,0x00,0xFC,0x00,0xD2,0x0C,0x90,0x00,0x00,
0xEA,0x60,0x6D,0x34,0x66,0x08,0x13,0xFC,0x00,0x01,0x00,0xBF,0xE9,0x01,0x0C,
0x39,0x00,0x75,0x00,0xBF,0xE9,0x01,0x66,0x06,0x4E,0xF9,0x00,0xFC,0x00,0xD2,
0x20,0x3A,0x02,0xD8,0x42,0x81,0x12,0x39,0x00,0xBF,0xE9,0x01,0x80,0xC1,0x48,
0x40,0x4A,0x40,0x67,0x06,0x4C,0xDF,0x3F,0xFF,0x4E,0x75,0x4C,0xDF,0x3F,0xFF,
0x4E,0xF9,0x00,0xFC,0x12,0xFC,0x4E,0xB9,0x00,0xFC,0x06,0xDC,0x48,0xE7,0xFF,
0xFC,0x61,0x00,0x01,0x50,0x24,0x69,0x00,0x14,0x24,0x6A,0x00,0x0A,0x0C,0x92,
0x74,0x72,0x61,0x63,0x66,0x00,0x00,0x80,0x4A,0xA9,0x00,0x2C,0x66,0x00,0x00,
0x78,0x0C,0x69,0x00,0x02,0x00,0x1C,0x67,0x00,0x00,0xA4,0x0C,0x69,0x00,0x03,
0x00,0x1C,0x67,0x0C,0x0C,0x69,0x00,0x0B,0x00,0x1C,0x67,0x04,0x60,0x00,0x00,
0x5A,0x2C,0x79,0x00,0x00,0x00,0x04,0x33,0x7C,0x00,0x0F,0x00,0x1C,0x4E,0xB9,
0x00,0xFC,0x06,0xDC,0x4A,0xA9,0x00,0x20,0x66,0x40,0x41,0xFA,0x01,0xF8,0x52,
0x90,0x20,0x10,0x80,0xFC,0x00,0x05,0x48,0x40,0x4A,0x40,0x66,0x02,0x61,0x32,
0x61,0x6E,0x33,0x7C,0x00,0x03,0x00,0x1C,0x41,0xFA,0xFE,0x34,0x23,0x48,0x00,
0x28,0x23,0x7C,0x00,0x00,0x04,0x00,0x00,0x24,0x23,0x7C,0x00,0x00,0x00,0x00,
0x00,0x2C,0x2C,0x79,0x00,0x00,0x00,0x04,0x4E,0xB9,0x00,0xFC,0x06,0xDC,0x4C,
0xDF,0x3F,0xFF,0x4E,0x75,0x61,0x00,0x00,0xEC,0x6D,0x60,0x33,0x7C,0x00,0x0B,
0x00,0x1C,0x23,0x7C,0x00,0x06,0xE0,0x00,0x00,0x24,0x23,0x7C,0x00,0x06,0xE0,
0x00,0x00,0x2C,0x23,0x7C,0x00,0x00,0x00,0x00,0x00,0x28,0x2C,0x79,0x00,0x00,
0x00,0x04,0x4E,0xF9,0x00,0xFC,0x06,0xDC,0x0C,0xA9,0x00,0x00,0x04,0x00,0x00,
0x24,0x66,0x00,0xFF,0x68,0x60,0xBC,0x41,0xFA,0xFD,0xD0,0x20,0xBC,0x00,0x00,
0x00,0x00,0x45,0xFA,0xFD,0xC2,0x20,0x3C,0x00,0x00,0x01,0x00,0x22,0x3C,0xFF,
0xFF,0xFF,0xFF,0x44,0xFC,0x00,0x00,0x24,0x1A,0x93,0x82,0x51,0xC8,0xFF,0xFA,
0x20,0x81,0x4E,0x75,0x2C,0x79,0x00,0x00,0x00,0x04,0x2D,0x7A,0x01,0x2C,0x00,
0x2A,0x2D,0x7A,0x01,0x2A,0x00,0x2E,0x4E,0xD5,0x42,0x4D,0x2C,0x79,0x00,0x00,
0x00,0x04,0x20,0x3C,0x00,0x00,0x07,0xD0,0x22,0x7A,0x01,0x50,0x4E,0xAE,0xFF,
0x34,0x41,0xFA,0x01,0x7A,0x52,0x90,0x0C,0x90,0x00,0x00,0x00,0x05,0x6D,0x10,
0x20,0x10,0xC0,0xFC,0x28,0x00,0x22,0x3C,0x00,0x00,0x00,0x02,0x4E,0xAE,0xFF,
0x3A,0x41,0xFA,0x01,0x60,0x42,0x98,0x60,0x42,0x2D,0x7A,0x00,0xE2,0x00,0x2A,
0x2D,0x7A,0x00,0xE0,0x00,0x2E,0x1D,0x7C,0x00,0xFF,0x00,0x32,0x2D,0x7A,0x00,
0xD8,0xFE,0x3A,0x61,0x1C,0x6D,0x06,0x2D,0x7A,0x00,0xD2,0x00,0x94,0x41,0xEE,
0x00,0x22,0x42,0x40,0x72,0x17,0xD0,0x58,0x51,0xC9,0xFF,0xFC,0x46,0x40,0x30,
0x80,0x4E,0x75,0x41,0xFA,0x00,0xC4,0x0C,0x90,0x00,0x00,0x07,0xD0,0x4E,0x75,
0x2C,0x79,0x00,0x00,0x00,0x04,0x43,0xFA,0x01,0x14,0x45,0xFA,0x01,0x74,0x23,
0x4A,0x00,0x3A,0x45,0xFA,0x03,0x60,0x23,0x4A,0x00,0x3E,0x23,0x4A,0x00,0x36,
0x13,0x7C,0x00,0x01,0x00,0x08,0x13,0x7C,0xFF,0x88,0x00,0x09,0x45,0xFA,0x00,
0xA4,0x23,0x4A,0x00,0x0A,0x45,0xFA,0x00,0x10,0x47,0xF9,0x00,0x00,0x00,0x00,
0x4E,0xAE,0xFE,0xE6,0x60,0x00,0xFF,0x7C,0x2C,0x79,0x00,0x00,0x00,0x04,0x20,
0x7A,0x00,0x66,0x4E,0x90,0x20,0x3C,0x00,0x00,0x00,0x7E,0x22,0x3C,0x00,0x00,
0x00,0x02,0x4E,0xAE,0xFF,0x3A,0x4A,0x80,0x67,0xE0,0x41,0xFA,0x00,0x50,0x20,
0x80,0x20,0x40,0x43,0xFA,0xFF,0xD6,0x22,0x3C,0x00,0x00,0x00,0x7E,0x20,0x01,
0x10,0xD9,0x51,0xC9,0xFF,0xFC,0x43,0xFA,0xFF,0xC4,0x20,0x7A,0x00,0x32,0x4E,
0xE8,0x00,0x46,0x47,0xFA,0x00,0x32,0x4A,0x13,0x67,0x0A,0x4E,0xAE,0xFF,0x2E,
0x20,0x7A,0x00,0x1E,0x4E,0xD0,0x16,0xBC,0x00,0xFF,0x60,0xF4,0x00,0x01,0x06,
0x5C,0x00,0x01,0x06,0x72,0x00,0x01,0x05,0x50,0x00,0x01,0x04,0xDA,0x00,0x01,
0x06,0xAC,0x00,0x00,0x5B,0x00,0x00,0x00,0x02,0x0F,0x00,0x00,0x00,0x00,0x64,
0x6F,0x73,0x2E,0x6C,0x69,0x62,0x72,0x61,0x72,0x79,0x00,0x00,0x00,0x63,0x6C,
0x69,0x70,0x62,0x6F,0x61,0x72,0x64,0x2E,0x64,0x65,0x76,0x69,0x63,0x65,0x00,
0x00,0x00,0x01,0x04,0x00,0x00,0x00,0x00,0x7A,0x20,0x28,0x43,0x29,0x72,0x61,
0x63,0x6B,0x72,0x69,0x67,0x68,0x74,0x20,0x62,0x79,0x20,0x44,0x69,0x73,0x6B,
0x2D,0x44,0x6F,0x6B,0x74,0x6F,0x72,0x73,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
0x00,0x00,0x00,0x00 };

int main()
{
	switch(xvs_analyzer( data, sizeof(data)))
	{
		case XVS_NOTAVAIL:
			printf("no xvs.library available\n");
			break;
		
		case XVS_INTERR:
			printf("internal error\n");
			break;
		
		case XVS_VIRUSDETECTED:
			printf("\n\n VIRUS FOUND !!!!!\n\n\a");
			break;
		
		case XVS_FILEOK:
			printf(" *** File is OK! *** \n");
			break;
		
		default:
			printf("...Sorry ?\n");
			break;
	}
	
	return 0;
}
#endif /* TEST */
