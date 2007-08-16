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
gcc -noixemul -m68020 -O -W -Wall -Winline window.c -o window -s -DDEBUG -DTEST
quit 0
*/

#define __NOLIBBASE__
#include "global.h"

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/space.h>
#include <images/label.h>
#include <reaction/reaction_macros.h>

#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/space.h>
#include <proto/checkbox.h>

STATIC struct Library * IntuitionBase = NULL;
STATIC struct Library * DOSBase = NULL;

STATIC struct Library * WindowBase = NULL;
STATIC struct Library * LayoutBase = NULL;
/*STATIC struct Library * LabelBase = NULL;
STATIC struct Library * SpaceBase = NULL;
STATIC struct Library * CheckBoxBase = NULL;*/

//------------------------------------------------------------------------------

INLINE BOOL OpenLibraries( VOID )
{
	return((
		(IntuitionBase = OpenLibrary(__intuitionname,0)) &&
	//	(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0)) &&
	//	(DiskfontBase = OpenLibrary("diskfont.library", 0)) &&
		(DOSBase = OpenLibrary(__dosname, 0)) &&
	//	(DataTypesBase = OpenLibrary("datatypes.library", 0L)) &&
		(WindowBase = OpenLibrary("window.class", 0L)) && 
		(LayoutBase = OpenLibrary("gadgets/layout.gadget", 0L)) //&&
	//	(LabelBase = OpenLibrary("images/label.image", 0L)) &&
	//	(SpaceBase = OpenLibrary("gadgets/space.gadget", 0L)) &&
	//	(CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 0L))
	) ? TRUE : FALSE);
}

INLINE VOID CloseLibraries( VOID )
{
//	if(CheckBoxBase) CloseLibrary(CheckBoxBase);
//	if(SpaceBase) CloseLibrary(SpaceBase);
//	if(LabelBase) CloseLibrary(LabelBase);
	if(LayoutBase) CloseLibrary(LayoutBase);
	if(WindowBase) CloseLibrary(WindowBase);
//	if(DataTypesBase) CloseLibrary(DataTypesBase);
	if(DOSBase) CloseLibrary(DOSBase);
//	if(DiskfontBase) CloseLibrary(DiskfontBase);
//	if(GfxBase) CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase) CloseLibrary(IntuitionBase);
}

//------------------------------------------------------------------------------

VOID Window( VOID )
{
	
	if(OpenLibraries())
	{
		struct Screen * scr = NULL;
		
		if((scr = LockPubScreen(NULL)))
		{
			struct DrawInfo *drinfo = GetScreenDrawInfo(scr);
			Object * window_obj;
			
			window_obj = NewObject( WINDOW_GetClass(), NULL,
			//	WA_ScreenTitle,		(ULONG) ProgramName(),
				WA_SmartRefresh,	TRUE,
				WA_Activate,		TRUE,
				WA_PubScreen,		(ULONG) scr,
				WA_PubScreenFallBack,	TRUE,
				//WA_BackFill,		(ULONG) &BackFillHook,
				WINDOW_Activate,	TRUE,
				WINDOW_FrontBack,	WT_FRONT,
				WINDOW_Position,	WPOS_CENTERSCREEN,
				WINDOW_Layout,(ULONG)NewObject(LAYOUT_GetClass(),NULL,
					GA_DrawInfo,  (ULONG) drinfo,
					LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
					LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
						GA_DrawInfo,  (ULONG) drinfo,
						
					TAG_DONE),
				TAG_DONE),
			TAG_DONE);
			
			if( window_obj )
			{
				struct Window * wnd = NULL;
				
				if((wnd = (struct Window *)DoMethod(window_obj, WM_OPEN, NULL)))
				{
					ULONG signals=0, HI_Result, RA_Code;
					BOOL running = TRUE;
					
					GetAttr( WINDOW_SigMask, window_obj, &signals );
					
					while(running)
					{
						ULONG sigs = Wait(signals|SIGBREAKF_CTRL_C);
						
						if( sigs & SIGBREAKF_CTRL_C ) break;
						
						while((HI_Result=DoMethod(window_obj,WM_HANDLEINPUT,&RA_Code)))
						{
							switch(HI_Result & WMHI_CLASSMASK)
							{
								case WMHI_GADGETUP:
									switch(HI_Result & WMHI_GADGETMASK)
									{
								/*		case GAID_VIEWPROP:
											DBG("View Properties\n");
											break;
									*/	default:
											DBG("Unknown gadget!\n");
											break;
									}
									break;
								default:
									break;
							}
						}
					}
					
					// this isn't really required, DisposeObject() makes it
					DoMethod(window_obj, WM_CLOSE, NULL);
				}
				
				DisposeObject(window_obj);
			}
			
			if (drinfo)
				FreeScreenDrawInfo(scr, drinfo);
			
			UnlockPubScreen(NULL, scr );
		}
	}
	else if(IntuitionBase) DisplayBeep(NULL);
	
	CloseLibraries();
}

//------------------------------------------------------------------------------

#ifdef TEST
#include "debug.c"

int main()
{
	Window();
	
	return 0;
}

#endif /* TEST */

