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

;/* execute me to compile :ts=4
gcc -noixemul -m68020 -O -W -Wall -Winline alert.c -o alert -s -DDEBUG -DTEST
quit 0
*/

#if defined(TEST)
# define HEAVY_DEBUG
#endif
#ifndef HEAVY_DEBUG
# undef DEBUG
#endif
#define __NOLIBBASE__
#include "pch_socket.h"

#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/space.h>
#include <images/label.h>
#include <reaction/reaction_macros.h>

#include <proto/diskfont.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/space.h>
#include <proto/checkbox.h>

#include <graphics/gfxmacros.h>
#include <datatypes/soundclass.h>

#include <proto/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <SDI_hook.h>
#include "alert.h"
#include "alert_pri.h"
#include "taskreg.h"

#define WIDTH	250
#define HEIGHT	350

#define MAKECOLOR32(x)	(((x)<<24)|((x)<<16)|((x)<<8)|(x))
#define WND(wptr)		((struct Window *)(wptr))
#define wndcm(window)	WND(window)->WScreen->ViewPort.ColorMap
#define OBP_FLAGS	OBP_Precision,PRECISION_GUI,OBP_FailIfBad,FALSE,TAG_DONE

#define mObtainBestPen( window, R, G, B ) \
({ \
	ULONG r=MAKECOLOR32(R),g=MAKECOLOR32(G),b=MAKECOLOR32(B); \
	ObtainBestPen(wndcm(window),r,g,b,OBP_FLAGS); \
})

STATIC struct Library * IntuitionBase = NULL;
STATIC struct GfxBase * GfxBase = NULL;
STATIC struct Library * DiskfontBase = NULL;
STATIC struct Library * DOSBase = NULL;
STATIC struct Library * DataTypesBase = NULL;

STATIC struct Library * WindowBase = NULL;
STATIC struct Library * LayoutBase = NULL;
STATIC struct Library * LabelBase = NULL;
STATIC struct Library * SpaceBase = NULL;
STATIC struct Library * CheckBoxBase = NULL;

enum Gadgets_IDs
{
	GAID_BASE = 0xDF09,
	GAID_VIEWPROP,
	GAID_REMEMBER,
	GAID_ALLOW,
	GAID_DENY,
};

STATIC struct TextAttr helvetica15b = {
	(STRPTR)"helvetica.font", 15, FSF_BOLD, FPF_DISKFONT
};
STATIC struct TextAttr helvetica12 = {
	(STRPTR)"helvetica.font", 12, 0, FPF_DISKFONT
};
STATIC struct TextAttr times15b = {
	(STRPTR)"times.font", 15, FSF_BOLD, FPF_DISKFONT
};
STATIC struct TextAttr times13b = {
	(STRPTR)"times.font", 13, FSF_BOLD, FPF_DISKFONT
};
STATIC struct TextAttr times13 = {
	(STRPTR)"times.font", 13, 0, FPF_DISKFONT
};

/*
   -----------------------------------   -\    -\
     /\         <TITLE_FONT>               \     ) <TOP_TITLE_SIZE>
     \/  /¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯      \  -/
   _____/       <detection type>             \
                                             -) <TITLE>
*/
#define TITLE_FONT      ((struct TextAttr *)(&helvetica15b))
#define DECTYPE_FONT    ((struct TextAttr *)(&helvetica12))
#define ATEMPT_FONT     ((struct TextAttr *)(&times15b))
#define DEFWND_FONT     ((struct TextAttr *)(&times13))
#define DEFWND_FONT_B   ((struct TextAttr *)(&times13b))

#define TOP_TITLE_SIZE  (TITLE_FONT->ta_YSize + 8)
#define DECTYPE_SIZE    (DECTYPE_FONT->ta_YSize + 4)
#define TITLE           (TOP_TITLE_SIZE + DECTYPE_SIZE + 4)

// The follow is TOO bad, I'm hardcoding space for 3 lines! :-/
#define ATEMPT_LINES	3
#define ATEMPT_YSIZE    (ATEMPT_FONT->ta_YSize*ATEMPT_LINES)

#define TITICON_BORDER  4
#define TITICON_WIDTH   (/*WarnLogoImageWide*/32+(TITICON_BORDER*2))

#define ALERT_SOUND     "T:ZoneAlarm.snd"

// this is in pixels, even used when the window uses "virtual pixels" ...
#define FixedInnerSpacing  6
#define FixedTopSpacing    20

#define WINDOW_BACKGROUND	mObtainBestPen(wnd, 190, 190, 190 )


GLOBAL STRPTR GetProtoByNumber(UWORD port, STRPTR *desc);

#ifdef TEST
# define SNPrintf snprintf
#endif
//------------------------------------------------------------------------------

INLINE BOOL OpenLibraries( VOID )
{
	return((
		(IntuitionBase = OpenLibrary(__intuitionname,0)) &&
		(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0)) &&
		(DiskfontBase = OpenLibrary("diskfont.library", 0)) &&
		(DOSBase = OpenLibrary(__dosname, 0)) &&
		(DataTypesBase = OpenLibrary("datatypes.library", 0L)) &&
		(WindowBase = OpenLibrary("window.class", 0L)) && 
		(LayoutBase = OpenLibrary("gadgets/layout.gadget", 0L)) &&
		(LabelBase = OpenLibrary("images/label.image", 0L)) &&
		(SpaceBase = OpenLibrary("gadgets/space.gadget", 0L)) &&
		(CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 0L))
	) ? TRUE : FALSE);
}

INLINE VOID CloseLibraries( VOID )
{
	if(CheckBoxBase) CloseLibrary(CheckBoxBase);
	if(SpaceBase) CloseLibrary(SpaceBase);
	if(LabelBase) CloseLibrary(LabelBase);
	if(LayoutBase) CloseLibrary(LayoutBase);
	if(WindowBase) CloseLibrary(WindowBase);
	if(DataTypesBase) CloseLibrary(DataTypesBase);
	if(DOSBase) CloseLibrary(DOSBase);
	if(DiskfontBase) CloseLibrary(DiskfontBase);
	if(GfxBase) CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase) CloseLibrary(IntuitionBase);
}

//------------------------------------------------------------------------------

INLINE VOID PrepareSoundFile( VOID )
{
	BPTR lock;
	
	if((lock = Lock( ALERT_SOUND, SHARED_LOCK)))
		UnLock( lock );
	else {
		if((lock = Open( ALERT_SOUND, MODE_NEWFILE )))
		{
			Write( lock, (STRPTR)AlertSound, sizeof(AlertSound));
			Close( lock );
		}
	}
}

//------------------------------------------------------------------------------

STATIC void RectFillOutline(struct RastPort *rp, WORD xmin, WORD ymin, WORD xmax, WORD ymax, LONG p1, LONG p2)
{
	REGISTER ULONG orig_apen;
	
	// backup A-pen (this is needed?..)
	orig_apen = GetAPen( rp );
	
	// draw box
	SetAPen(rp,p1);
	Move(rp, xmin, ymax);
	Draw(rp, xmin, ymin);
	Draw(rp, xmax, ymin);
	SetAPen(rp,p2);
	Draw(rp, xmax, ymax);
	Draw(rp, xmin, ymax);
	
	// restore a-pen
	SetAPen( rp, orig_apen );
}

//------------------------------------------------------------------------------

STATIC VOID TextShadow
(	struct RastPort * rp,
	WORD x, WORD y,
	STRPTR text, UWORD textLen,
	LONG APen, LONG ShadowColor, LONG BPen)
{
	REGISTER LONG orig_apen, orig_bpen;
	
	// backup pens
	orig_apen = GetAPen( rp );
	orig_bpen = GetBPen( rp );
	
	// JAM1 is required...
	SetDrMd( rp, JAM1);
	
	// write shadowed text
	SetBPen( rp, BPen );
	SetAPen( rp, ShadowColor );
	Move( rp, x+1, y+1 );
	Text( rp, text, textLen );
	SetAPen( rp, APen );
	Move( rp, x, y );
	Text( rp, text, textLen );
	
	// restore pens
	SetAPen( rp, orig_apen );
	SetBPen( rp, orig_bpen );
}

STATIC LONG TextWordWrap
(	struct RastPort *rp,
	WORD xmin, WORD ymin,
	WORD xmax, WORD ymax,
	LONG APen, LONG ShadowColor, LONG BPen,
	BYTE MaxLines,
	struct TextAttr * ta,
	STRPTR string )
{
	struct TextFont * font = NULL, *rp_font = NULL;
	REGISTER STRPTR ptr=string;
	REGISTER WORD x1=0, y1=ymin, width=(xmax-xmin);
	REGISTER LONG word_count, /*AverageWidth,*/ SpaceWidth;
	LONG rc = -1, lines = 0;
	
	ENTER();
	
	if(ta != NULL)
	{
		DBG("Wanted to change font...\n");
		
		if((font = (struct TextFont *) OpenDiskFont( ta )))
		{
			rp_font = rp->Font;
			
			SetFont( rp, font );
			if(ta->ta_Style != 0)
				SetSoftStyle( rp, ta->ta_Style, 7 );
			
			DBG("...and font was changed OK.\n");
		}
	}
	
	SpaceWidth   = TextLength( rp, " ", 1 );
	//AverageWidth = TextLength( rp, "M", 1 ) + SpaceWidth / 2;
	
	SetAPen( rp, APen );
	SetBPen( rp, BPen );
	SetDrMd( rp, JAM1);
	Move( rp, xmin, rc=ymin );
	
	while(*ptr)
	{
		UBYTE *start=ptr;
		long tlen;
		
		while(*ptr && *ptr != 0x20)
		{
			if(*++ptr == '\n')
				*ptr = 0x20;
		}
		
		word_count = ptr - start;
		
		tlen = TextLength( rp, start, word_count );
		
		if((x1+tlen/*+AverageWidth*/) > width)
		{
			/* no space to draw this word on the current line */
			
			DBG("Out of space from position \"%s\"\n", start );
			
			if(width < tlen)
			{
				// uh-oh !! ...no space to fill this word on the WHOLE window!
				
				DBG("damn!, the next word DO NOT fit into the window!\a\n");
				
				#warning fixme
				break;
			}
			else
			{
				y1 += (rp->Font->tf_YSize /*+ rp->Font->tf_Baseline*/ + 1);
				
				if( y1 >= ymax )
				{
					// ouch!, no space to draw more text, we reached the HEIGHT!
					
					DBG("damn!, no more HEIGHT!\n");
					
					break;
				}
				else if( MaxLines > 0 )
				{
					if(++lines >= MaxLines) break;
				}
				
				Move( rp, xmin, rc=y1 );
				x1 = 0;
				
				DBG("Moved to next line at y-axis %ld\n", y1 );
			}
		}
		
		if(ShadowColor != -1)
			TextShadow(rp, xmin+x1, y1, start, word_count, APen, ShadowColor, BPen);
		else
			Text( rp, start, word_count );
		x1 += tlen;
		
		if((x1+SpaceWidth) < width)
		{
			Text( rp, " ", 1 );
			x1 += SpaceWidth;
		}
		
		// skip the space(s)
		while(*ptr && *ptr == 0x20)
			ptr++;
	}
	
	rc += (rp->Font->tf_YSize /*+ rp->Font->tf_Baseline*/ + 1);
	
	if( ta != NULL )
	{
		DBG("Restoring rastport font... (%lx -> %lx)\n", font, rp_font);
		
		if(font != NULL)
		{
			if( rp_font != NULL ) // this could be NULL !? I think NO, but...
			{
				SetFont( rp, rp_font );
				SetSoftStyle( rp, rp_font->tf_Style, 7 );
			}
			
			CloseFont(font);
		}
	}
	
	RETURN(rc);
	return(rc);
}

//------------------------------------------------------------------------------
#define StrLen	__i_StrLen
INLINE ULONG StrLen(const char *string)
{ const char *s;
#if 1
  if(!(string && *string))
  	return 0;
#endif
  s=string;
  do;while(*s++); return ~(string-s);
}

//------------------------------------------------------------------------------

struct BackFillData
{
	struct Layer *layer;
	struct Rectangle bounds;
	WORD offsetx;
	WORD offsety;
};

HOOKPROTO( BackFillHookFunc, VOID, struct RastPort *rp_orig, struct BackFillData * bfd )
{
	REGISTER LONG xmin, ymin, xmax, ymax, bg_pen;
	struct RastPort rptmp, *rp = &rptmp;
	struct Window * wnd = (struct Window *) bfd->layer->Window;
//	AlertData * ad = (AlertData *) hook->h_Data;
	
	CopyMem( rp_orig, rp, sizeof(struct RastPort));
	rp->Layer = NULL;
	
	xmin = bfd->bounds.MinX;
	ymin = bfd->bounds.MinY;
	xmax = bfd->bounds.MaxX;
	ymax = bfd->bounds.MaxY;
	
	DBG("xmin=%ld xmax=%ld ymin=%ld ymax=%ld width=%ld height=%ld rp=%lx wnd=%lx\n", xmin, xmax, ymin, ymax, xmax-xmin, ymax-ymin, rp_orig, wnd );
	
	bg_pen   = WINDOW_BACKGROUND;
	
	SetAPen( rp, bg_pen );
	SetBPen( rp, bg_pen );
	SetDrMd( rp, JAM2);
	SetDrPt( rp,(UWORD) ~0); // <- draw solid lines
	
	// draw the background
	RectFill( rp, xmin+1, ymin+1, xmax-1, ymax-1 );
	
	if(xmax-xmin == wnd->Width-1 && ymax-ymin == wnd->Height-1)
	{
		// draw the window border
		RectFillOutline( rp, xmin, ymin, xmax, ymax, 1,1);
	}
	
	if( bg_pen != -1 ) ReleasePen(wndcm(wnd), bg_pen );
}
STATIC MakeHook( BackFillHook, BackFillHookFunc );

//------------------------------------------------------------------------------

HOOKPROTO( AtemptMsgHookFunc, VOID, struct RastPort *rp_orig, struct BackFillData * bfd )
{
	REGISTER LONG xmin, ymin, xmax, ymax, bg_pen;
	struct RastPort rptmp, *rp = &rptmp;
	struct Window * wnd = (struct Window *) bfd->layer->Window;
	AlertData * ad = (AlertData *) hook->h_Data;
	STRPTR top_message;
	
	ENTER();
	
	CopyMem( rp_orig, rp, sizeof(struct RastPort));
	rp->Layer = NULL;
	
	xmin = bfd->bounds.MinX;
	ymin = bfd->bounds.MinY;
	xmax = bfd->bounds.MaxX;
	ymax = bfd->bounds.MaxY;
	
	DBG("xmin=%ld xmax=%ld ymin=%ld ymax=%ld width=%ld height=%ld rp=%lx wnd=%lx\n", xmin, xmax, ymin, ymax, xmax-xmin, ymax-ymin, rp_orig, wnd );
	
	bg_pen   = WINDOW_BACKGROUND;
	
	SetAPen( rp, bg_pen );
	SetBPen( rp, bg_pen );
	SetDrMd( rp, JAM2);
	SetDrPt( rp,(UWORD) ~0); // <- draw solid lines
	
	// draw the background
	RectFill( rp, xmin, ymin, xmax, ymax );
	
	if((top_message = Malloc(257)))
	{
		STRPTR filePart = FilePart(ZAT(ad->data)->task_name);
		
		if(ad->flags & AF_SOCKET)
		{
			if(ad->flags & AF_BIND)
			{
				SNPrintf( top_message, 256,
					"%s is trying to act as a server listening port %ld.",
					filePart, SOCKADDR(SBP(ad))->sin_port );
			}
			else /*if(ad->flags & AF_CONNECT)*/
			{
				SNPrintf( top_message, 256,
					"%s is trying to access the internet.", filePart );
			}
		}
		else if(ad->flags & AF_EXEC)
		{
			if(ad->flags & AF_COLDREBOOT)
			{
				SNPrintf( top_message, 256,
					"%s is trying to REBOOT your system!", filePart );
			}
		}
		
		TextWordWrap( rp, xmin, ymin, xmax, ymax, 1,-1,bg_pen, ATEMPT_LINES, ATEMPT_FONT, top_message );
		
		Free(top_message);
	}
	
	if( bg_pen != -1 ) ReleasePen(wndcm(wnd), bg_pen );
	
	LEAVE();
}
STATIC MakeHook( AtemptMsgHook, AtemptMsgHookFunc );

//------------------------------------------------------------------------------

HOOKPROTO( TitleHookFunc, VOID, struct RastPort *rp_orig, struct BackFillData * bfd )
{
	struct RastPort rptmp, *rp = &rptmp;
	REGISTER LONG xmin, ymin, xmax, ymax, ymax_p;
	REGISTER WORD x, y, z, a;
	struct Window * wnd = (struct Window *) bfd->layer->Window;
	AlertData * ad = (AlertData *) hook->h_Data;
	struct TextFont * font;
	TaskReg * tr = ad->tr;
	LONG bg_pen = -1, red_pen = -1, blue_pen = -1, 
		orange_pen = -1, cur_pen = -1, yellow_pen = -1, tlen;
	
	ENTER();
	
	CopyMem( rp_orig, rp, sizeof(struct RastPort));
	rp->Layer = NULL;
	
	// BackFillHook will (or have) drawn a window border's line, so skip it
	xmin = bfd->bounds.MinX+1;
	ymin = bfd->bounds.MinY+1;
	xmax = bfd->bounds.MaxX-1;
	ymax = bfd->bounds.MaxY-1;
	
	DBG("xmin=%ld xmax=%ld ymin=%ld ymax=%ld width=%ld height=%ld rp=%lx wnd=%lx\n", xmin, xmax, ymin, ymax, xmax-xmin, ymax-ymin, rp_orig, wnd );
	
	DBG_ASSERT(ymax-ymin < TITLE); // too bad if happens...(needs re-layout)
	
	bg_pen   = WINDOW_BACKGROUND;
	
	SetAPen( rp, bg_pen );
	SetBPen( rp, bg_pen );
	SetDrMd( rp, JAM2);
	SetDrPt( rp,(UWORD) ~0); // <- draw solid lines
	
	// draw the background
	RectFill( rp, xmin, ymin, xmax, ymax );
	
	// select top box color
	if(ZAT(ad->data)->xvs == XVS_VIRUSDETECTED)
	{
		// DAMN!, a VIRUS have been DETECTED !
		cur_pen = yellow_pen = mObtainBestPen(wnd, 255, 255, 0 );
	}
	else if( tr == NULL )
	{
		// first atempt from program
		cur_pen = red_pen = mObtainBestPen(wnd, 255,   0,   0 );
	}
	else if(ZAT(ad->data)->execrc != tr->FileCRC)
	{
		// this program has changed since the last run!
		cur_pen = orange_pen = mObtainBestPen(wnd, 210, 110,  0 );
	}
	else if(ad->flags & AF_BIND)
	{
		// server program
		cur_pen = cyan_pen = mObtainBestPen(wnd,  19, 170, 194 );
	}
	else
	{
		// this program was seen already
		cur_pen = blue_pen = mObtainBestPen(wnd,  19,  92, 194 );
	}
	
	// adjust nominal ymax
	ymax_p = ymin+TOP_TITLE_SIZE;
	
	// draw the top box
	SetAPen( rp, cur_pen );
	SetBPen( rp, cur_pen );
	RectFill( rp, xmin, ymin, xmax, ymax_p );
	RectFill( rp, xmin, ymax_p, xmin+TITICON_WIDTH, ymax_p+DECTYPE_SIZE);
	
	x = 0;
	y = DECTYPE_SIZE;
	z = xmin+TITICON_WIDTH;
	a = ymax_p+y;
	
	while( x < y )
	{
		Move( rp, z, ymax_p-x );
		Draw( rp, z, a-x );
		x++; z++;
	}
	
	// draw a black line as border to the top box
	SetAPen( rp, 1 );
	SetBPen( rp, 1 );
	
	Move( rp, xmin, ymax_p+DECTYPE_SIZE );
	Draw( rp, xmin+TITICON_WIDTH, ymax_p+DECTYPE_SIZE );
	
	Move( rp, xmin+TITICON_WIDTH+1, ymax_p+DECTYPE_SIZE-1 );
	Draw( rp, xmin+TITICON_WIDTH+x, a-x );
	
	Move( rp, xmin+TITICON_WIDTH+x, a-x );
	Draw( rp, xmax, a-x );
	
	
	// draw the top title
	if((font = (struct TextFont *) OpenDiskFont(TITLE_FONT)))
	{
		struct TextFont * OldFont = rp->Font;
		
		SetFont( rp, font );
		if(TITLE_FONT->ta_Style != 0)
			SetSoftStyle( rp, TITLE_FONT->ta_Style, 7 );
		
		tlen = TextLength( rp, "ZoneAlarm Security Alert", 24);
		TextShadow( rp,
			xmin+(((xmax-xmin)/2)-(tlen/2))+(TITICON_WIDTH/2),
			ymin+((TOP_TITLE_SIZE/2)-(rp->Font->tf_YSize/2))+rp->Font->tf_Baseline,
			"ZoneAlarm Security Alert", 24,		2, 1, cur_pen );
		
		SetFont( rp, OldFont );
		SetSoftStyle( rp, OldFont->tf_Style, 7 );
		
		CloseFont( font );
	}
	
	
	// draw the detection type
	if((font = (struct TextFont *) OpenDiskFont(DECTYPE_FONT)))
	{
		struct TextFont * OldFont = rp->Font;
		STRPTR msg;
		UWORD msg_len;
		
		SetFont( rp, font );
		if(DECTYPE_FONT->ta_Style != 0)
			SetSoftStyle( rp, DECTYPE_FONT->ta_Style, 7 );
		
		if(ZAT(ad->data)->xvs == XVS_VIRUSDETECTED)
		{
			msg = "VIRUS DETECTED";
		}
		else if(ZAT(ad->data)->execrc != tr->FileCRC)
		{
			msg = "CHANGED PROGRAM";
		}
		else if(ad->flags & AF_BIND)
		{
			msg = "SERVER PROGRAM";
		}
		else if( tr == NULL )
		{
			msg = "NEW PROGRAM";
		}
		else
		{
			msg = "REPEATED PROGRAM";
		}
		
		msg_len = StrLen(msg);
		tlen = TextLength( rp, msg, msg_len );
		Move( rp, xmin+(((xmax-xmin)/2)-(tlen/2))+(TITICON_WIDTH/2), ymin+TOP_TITLE_SIZE+(DECTYPE_SIZE/2)-(rp->Font->tf_YSize/2)+rp->Font->tf_Baseline+2 );
		Text( rp, msg, msg_len );
		
		SetFont( rp, OldFont );
		SetSoftStyle( rp, OldFont->tf_Style, 7 );
		
		CloseFont(font);
	}
	
	#if 0
	// draw the warning image
	DBG_ASSERT(WarnLogoImageHigh < TITLE);
	SetAPen( rp, red_pen );
	SetBPen( rp, red_pen );
	SetDrMd( rp, JAM1);
	DrawImage( rp, &WarnLogoImage, xmin+TITICON_BORDER, ymin+TITICON_BORDER );
	#endif
	
	if( yellow_pen  != -1 ) ReleasePen(wndcm(wnd), red_pen  );
	if( orange_pen  != -1 ) ReleasePen(wndcm(wnd), red_pen  );
	if( red_pen  != -1 ) ReleasePen(wndcm(wnd), red_pen  );
	if( blue_pen != -1 ) ReleasePen(wndcm(wnd), blue_pen );
	if( bg_pen   != -1 ) ReleasePen(wndcm(wnd), bg_pen   );
	
	LEAVE();
}
STATIC MakeHook( TitleHook, TitleHookFunc );

//------------------------------------------------------------------------------

HOOKPROTO( MoreInfoHookFunc, VOID, struct RastPort *rp_orig, struct BackFillData * bfd )
{
	REGISTER LONG xmin, ymin, xmax, ymax, bg_pen;
	struct RastPort rptmp, *rp = &rptmp;
	struct Window * wnd = (struct Window *) bfd->layer->Window;
	AlertData * ad = (AlertData *) hook->h_Data;
	TaskReg * tr = ad->tr;
	STRPTR message;
	
	ENTER();
	
	CopyMem( rp_orig, rp, sizeof(struct RastPort));
	rp->Layer = NULL;
	
	xmin = bfd->bounds.MinX;
	ymin = bfd->bounds.MinY;
	xmax = bfd->bounds.MaxX;
	ymax = bfd->bounds.MaxY;
	
	DBG("xmin=%ld xmax=%ld ymin=%ld ymax=%ld width=%ld height=%ld rp=%lx wnd=%lx\n", xmin, xmax, ymin, ymax, xmax-xmin, ymax-ymin, rp_orig, wnd );
	
	bg_pen   = WINDOW_BACKGROUND;
	
	SetAPen( rp, bg_pen );
	SetBPen( rp, bg_pen );
	SetDrMd( rp, JAM2);
	SetDrPt( rp,(UWORD) ~0); // <- draw solid lines
	
	// draw the background
	RectFill( rp, xmin, ymin, xmax, ymax );
	
	if((message = Malloc(257)))
	{
		if(ZAT(ad->data)->xvs == XVS_VIRUSDETECTED)
		{
			SNPrintf( message, 256,
				"WARNING: The VIRUS \"%s\" have been DETECTED on this program!!, "
				"We suggest you to terminate this program right now, and check "
				"your system with an Anti-Virus program NOW!", ZAT(ad->data)->virus );
		}
		else if( tr == NULL )
		{
			if(ad->flags & AF_SOCKET)
			{
				if(ad->flags & AF_BIND)
				{
					SNPrintf( message, 256,
						"This is the program's first attempt to act as a server at this port.");
				}
				else
				{
					SNPrintf( message, 256,
						"This is the program's first attempt to access the internet.");
				}
			}
			else if(ad->flags & AF_EXEC)
			{
				if(ad->flags & AF_COLDREBOOT)
				{
					SNPrintf( message, 256,
						"If you click \"Allow\", your system will be rebooted.");
				}
			}
		}
		else if(ZAT(ad->data)->execrc != tr->FileCRC)
		{
			SNPrintf( message, 256,
				"This program has changed since the last time it was run.");
		}
		else if(ad->flags & AF_SOCKET)
		{
			if(ad->flags & AF_BIND)
			{
				SNPrintf( message, 256,
					"This program has previously asked to be a server, "
					"your last action was to %s access", tr->allow ? "allow":"deny");
			}
			else
			{
				SNPrintf( message, 256,
					"This program has previously asked for internet access, "
					"your last action was to %s access", tr->allow ? "allow":"deny");
			}
		}
		else if(ad->flags & AF_EXEC)
		{
			if(ad->flags & AF_COLDREBOOT)
			{
				SNPrintf( message, 256,
					"If you click \"Allow\", your system will be rebooted.");
			}
		}
		
		TextWordWrap( rp, xmin, ymin, xmax, ymax, 1,-1,bg_pen, -1, DEFWND_FONT, message );
		
		Free(message);
	}
	
	if( bg_pen != -1 ) ReleasePen(wndcm(wnd), bg_pen );
	
	LEAVE();
}
STATIC MakeHook( MoreInfoHook, MoreInfoHookFunc );

//------------------------------------------------------------------------------

STATIC CONST_STRPTR Ident(AlertData * ad)
{
	switch( ZAT(ad->data)->task->tc_Node.ln_Type )
	{
		case NT_PROCESS:
			if(ZAT(ad->data)->parent != NULL)
				return "subprocess";
			return &"subprocess"[3];
		
		case NT_TASK:
			return "task"; // <- could a task use me?....
		
		default:
			break;
	}
	return "(unknown)";
}

//------------------------------------------------------------------------------

STATIC CONST_STRPTR DestIP(SocketBasePatch * sbp)
{
	struct sockaddr_in * sa = SOCKADDR(sbp);
	char * ip;
	STRPTR proto;
	
	/**
	 * this function IS thread-safe SINCE alert() is called uSemaLock'ed
	 */
	STATIC UBYTE buf[32];
	
	if(!sa) return "(unknown)";
	ip =(char *)&(sa->sin_addr);
	
	#define IC(ch) (((long)ch)&0xFF)
	
	SNPrintf( buf, sizeof(buf)-10, "%ld.%ld.%ld.%ld:",
		IC(ip[0]),IC(ip[1]),IC(ip[2]),IC(ip[3]) );
	
	if((proto = GetProtoByNumber((UWORD)sa->sin_port,NULL)) != NULL)
	{
		SNPrintf( &buf[StrLen(buf)], 10, proto );
	}
	else
	{
		SNPrintf( &buf[StrLen(buf)], 10, "%ld", sa->sin_port );
	}
	
	return((CONST_STRPTR) buf );
}

//------------------------------------------------------------------------------

/**
 * ::alert return codes
 *  
 *  -1 fatal error initiliazing/opening/... the window
 *   0 deny access for this task
 *   1 allow access
 */
LONG alert( AlertData * ad )
{
	LONG rc = ARC_FATAL;
	
	ENTER();
	
	/*BackFillHook.h_Data  = */
	AtemptMsgHook.h_Data = 
	TitleHook.h_Data     = 
	MoreInfoHook.h_Data  = (APTR) ad;
	
	if(OpenLibraries())
	{
		struct Screen * scr = NULL;
		ULONG IBaseLock;
		STRPTR ScreenName = NULL;
		
		IBaseLock = LockIBase(0);
		ScreenName = ((struct IntuitionBase *)IntuitionBase)->ActiveScreen->Title;
		scr = LockPubScreen(ScreenName);
		UnlockIBase(IBaseLock);
		
		DBG_STRING(ScreenName);
		if(scr == NULL)
			scr = LockPubScreen(NULL);
		
		if(scr != NULL)
		{
			struct DrawInfo *drinfo = GetScreenDrawInfo(scr);
			Object * window_obj,*id_left=NULL,*id_right=NULL;
			BOOL allow = FALSE;
			
			if(ad->flags & AF_SOCKET)
			{
				id_left = NewObject(LABEL_GetClass(),NULL,
									LABEL_DrawInfo, (ULONG) drinfo,
									IA_Font,        (ULONG) DEFWND_FONT,
									LABEL_Text,     (ULONG) " Identification:  \n",
									LABEL_Text,     (ULONG) " Aplication:",
									LABEL_Text,     (ULONG) "\n Destination IP:",
								TAG_DONE);
				id_right = NewObject(LABEL_GetClass(),NULL,
								LABEL_DrawInfo, (ULONG) drinfo,
								IA_Font,        (ULONG) DEFWND_FONT,
								LABEL_Text,     (ULONG) Ident(ad),
								LABEL_Text,     (ULONG) "\n",
								LABEL_Text,     (ULONG) FilePart(ZAT(ad->data)->executable),
								LABEL_Text,     (ULONG) "\n",
								LABEL_Text,     (ULONG) DestIP(SBP(ad)),
							TAG_DONE);
			}
			else if(ad->flags & AF_EXEC)
			{
				id_left = NewObject(LABEL_GetClass(),NULL,
									LABEL_DrawInfo, (ULONG) drinfo,
									IA_Font,        (ULONG) DEFWND_FONT,
									LABEL_Text,     (ULONG) " Identification:  \n",
									LABEL_Text,     (ULONG) " Aplication:",
								TAG_DONE);
				id_right = NewObject(LABEL_GetClass(),NULL,
								LABEL_DrawInfo, (ULONG) drinfo,
								IA_Font,        (ULONG) DEFWND_FONT,
								LABEL_Text,     (ULONG) Ident(ad),
								LABEL_Text,     (ULONG) "\n",
								LABEL_Text,     (ULONG) FilePart(ZAT(ad->data)->executable),
							TAG_DONE);
			}
			
			window_obj = NewObject( WINDOW_GetClass(), NULL,
				WA_ScreenTitle,		(ULONG) ProgramName(),
				WA_Width,			WIDTH,
				WA_Height,			HEIGHT,
				WA_DragBar,			FALSE,
				WA_CloseGadget,		FALSE,
				WA_SizeGadget,		FALSE,
				WA_DepthGadget,		FALSE,
				WA_Borderless,		TRUE,
				WA_SmartRefresh,	TRUE,
				//WA_NoCareRefresh,	TRUE,
				WA_Activate,        TRUE,
				WA_PubScreen,		(ULONG) scr,
				WA_PubScreenFallBack,	TRUE,
				WA_BackFill,		(ULONG) &BackFillHook,
				WINDOW_Activate,	TRUE,
				WINDOW_FrontBack,	WT_FRONT,
				WINDOW_Position,	WPOS_CENTERSCREEN,
				WINDOW_Layout,(ULONG)NewObject(LAYOUT_GetClass(),NULL,
					GA_DrawInfo,  (ULONG) drinfo,
					LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
					LAYOUT_VertAlignment, LAYOUT_ALIGN_TOP,
					LAYOUT_TopSpacing,    0,
					LAYOUT_BottomSpacing, 0,
					LAYOUT_LeftSpacing,   0,
					LAYOUT_RightSpacing,  0,
					LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
						GA_BackFill, (ULONG) &TitleHook,
						LAYOUT_ShrinkWrap, TRUE,
						LAYOUT_FixedVert, FALSE,
						LAYOUT_AddChild,(ULONG)NewObject(SPACE_GetClass(),NULL,TAG_DONE),
						CHILD_MinHeight, TITLE,
						CHILD_MaxHeight, TITLE,
					TAG_DONE),
					LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
						GA_DrawInfo,  (ULONG) drinfo,
						LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
						LAYOUT_VertAlignment, LAYOUT_ALIGN_BOTTOM,
						LAYOUT_TopSpacing,    FixedTopSpacing,
						LAYOUT_BottomSpacing, FixedInnerSpacing,
						LAYOUT_LeftSpacing,   FixedInnerSpacing,
						LAYOUT_RightSpacing,  FixedInnerSpacing,
						LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
							GA_BackFill, (ULONG) &AtemptMsgHook,
							LAYOUT_AddChild,(ULONG)NewObject(SPACE_GetClass(),NULL,TAG_DONE),
							CHILD_MinHeight, ATEMPT_YSIZE,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
							LAYOUT_ShrinkWrap, TRUE,
							LAYOUT_FixedHoriz, FALSE,
							LAYOUT_SpaceInner, FALSE,
							LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
								LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
								LAYOUT_ShrinkWrap,   TRUE,
								LAYOUT_FixedHoriz,   FALSE,
								LAYOUT_AddImage,(ULONG) id_left,
								LAYOUT_AddChild,(ULONG)NewObject(NULL,"button.gadget",
									GA_ID,                GAID_VIEWPROP,
									GA_TextAttr,          (ULONG) DEFWND_FONT,
									LAYOUT_FixedHoriz,   FALSE,
									BUTTON_TextPen,       3,
									BUTTON_FillTextPen,   2,
									BUTTON_BevelStyle,    BVS_NONE,
									BUTTON_Transparent,   TRUE,
									BUTTON_SoftStyle,     FSF_UNDERLINED,
									BUTTON_Justification, BCJ_LEFT,
									GA_Text,              (ULONG) "View Properties",
									GA_RelVerify,         TRUE,
								TAG_DONE),
							TAG_DONE),
							LAYOUT_AddImage,(ULONG) id_right,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(SPACE_GetClass(),NULL,
							SPACE_MinHeight, 6,
						TAG_DONE),
						LAYOUT_AddImage,(ULONG)NewObject(LABEL_GetClass(),NULL,
							LABEL_DrawInfo, (ULONG) drinfo,
							IA_Font,        (ULONG) DEFWND_FONT_B,
							LABEL_Text,     (ULONG) "More Information Available:\n",
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
							GA_BackFill, (ULONG) &MoreInfoHook,
							LAYOUT_AddChild, (ULONG)NewObject(SPACE_GetClass(),NULL,TAG_DONE),
							CHILD_MinHeight, 40,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(SPACE_GetClass(),NULL,
							SPACE_MinHeight, 8,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(CHECKBOX_GetClass(),NULL,
							GA_DrawInfo,  (ULONG) drinfo,
							GA_ID, GAID_REMEMBER,
							GA_Text, (ULONG)"_Remember this setting",
							GA_RelVerify, TRUE,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(SPACE_GetClass(),NULL,
							SPACE_MinHeight, 2,
						TAG_DONE),
						LAYOUT_AddChild, (ULONG)NewObject(LAYOUT_GetClass(),NULL,
							LAYOUT_HorizAlignment, LAYOUT_ALIGN_CENTER,
							LAYOUT_AddChild,(ULONG)NewObject(NULL,"button.gadget",
								GA_ID,          GAID_ALLOW,
								BUTTON_BevelStyle, BVS_THIN,
								GA_Text,        (ULONG) "_Allow",
								GA_RelVerify,   TRUE,
							TAG_DONE),
							CHILD_MaxHeight, scr->Font->ta_YSize + 6,
							CHILD_MaxWidth, (WIDTH/3),
							LAYOUT_AddChild,(ULONG)NewObject(NULL,"button.gadget",
								GA_ID,          GAID_DENY,
								BUTTON_BevelStyle, BVS_THIN,
								GA_Text,        (ULONG) "_Deny",
								GA_RelVerify,   TRUE,
							TAG_DONE),
							CHILD_MaxHeight, scr->Font->ta_YSize + 6,
							CHILD_MaxWidth, (WIDTH/3),
						TAG_DONE),
					TAG_DONE),
				TAG_DONE),
			TAG_DONE);
			
			DBG("Screen=0x%08lx DrawInfo=0x%08lx\n", scr, drinfo );
			
			DBG_POINTER(window_obj);
			
			if( window_obj )
			{
				struct dtTrigger trigger;
				struct Window * wnd=NULL;
				Object *sample = NULL;
				//LONG SampleSignal;
				
				//if((SampleSignal = AllocSignal(-1)) != -1)
				{
					PrepareSoundFile ( ) ;
					sample = (Object*)NewDTObject(ALERT_SOUND,DTA_GroupID,GID_SOUND,TAG_DONE);
					
					if( sample != NULL )
					{
		/*				SetDTAttrs(sample,NULL,NULL,
							SDTA_SignalBit,(1L<<SampleSignal),
							SDTA_SignalTask,(ULONG)FindTask(0),
						TAG_DONE);
		*/				
						trigger.MethodID = DTM_TRIGGER;
						trigger.dtt_GInfo = NULL;
						trigger.dtt_Function = STM_PLAY;
						trigger.dtt_Data = NULL;
						
						DoDTMethodA(sample, 0L, 0L, (Msg)((ULONG)&trigger));
					}
				}
				
				if((wnd = (struct Window *)DoMethod(window_obj, WM_OPEN, NULL)))
				{
					ULONG signals=0, HI_Result, RA_Code;
					BOOL running = TRUE;
					
					DBG("window=$%lx CM=%lx\n", wnd, wndcm(wnd));
					
					GetAttr(WINDOW_SigMask, window_obj, &signals );
					
		/*			if( sample != NULL )
						signals |= (1L<<SampleSignal);
		*/			
					do {
						ULONG sigs = Wait(signals|SIGBREAKF_CTRL_C);
						
						if( sigs & SIGBREAKF_CTRL_C ) break;
		/*				if( sigs & (1L<<SampleSignal))
							DoDTMethodA(sample, 0L, 0L, (Msg)&trigger);
		*/				
						while((HI_Result=DoMethod(window_obj,WM_HANDLEINPUT,&RA_Code)))
						{
							switch(HI_Result & WMHI_CLASSMASK)
							{
								case WMHI_GADGETUP:
									switch(HI_Result & WMHI_GADGETMASK)
									{
										case GAID_VIEWPROP:
											DBG("View Properties\n");
											break;
										case GAID_REMEMBER:
											ad->remember = !(ad->remember);
											DBG("remember set to %ld\n", (LONG) ad->remember);
											break;
										case GAID_ALLOW:
											DBG("GAID_ALLOW\n");
											allow = TRUE;
											running = FALSE;
											break;
										case GAID_DENY:
											DBG("GAUD_DENY\n");
											running = FALSE;
											break;
										default:
											DBG("Unknown gadget!\n");
											break;
									}
									break;
								default:
									break;
							}
						}
					} while(running);
					
					rc = allow;
					DBG_ASSERT(rc==ARC_ALLOW||rc==ARC_DENY);
					
					DBG("Closing window...\n");
					
					// this isn't really required, DisposeObject() makes it
					DoMethod(window_obj, WM_CLOSE, NULL);
				}
				else DisplayBeep(NULL);
				
				DisposeObject(window_obj);
				
				if(sample != NULL )
					DisposeDTObject(sample);
				
				//if( SampleSignal != -1 )
				//	FreeSignal(SampleSignal);
			}
			else DisplayBeep(NULL);
			
			if (drinfo)
				FreeScreenDrawInfo(scr, drinfo);
			
			UnlockPubScreen(NULL, scr );
		}
		else DisplayBeep(NULL);
	}
	else if(IntuitionBase) DisplayBeep(NULL);
	
	CloseLibraries();
	
	RETURN(rc);
	return(rc);
}


#ifdef TEST
#include "debug.c"
APTR Malloc(ULONG Size) { return AllocVec(Size,0);}
VOID Free(APTR mem) { if(mem) FreeVec(mem);}
STRPTR ProgramName( VOID ) { return "ZoneAlarm";}
int main( int argc, char *argv[] )
{
	UBYTE task_name[] = "Network Handler Process";
	SocketBasePatch sbp;
	BOOL rr=0;
	
	bzero( &sbp, sizeof(SocketBasePatch));
	
	sbp.task_name  = task_name;
	sbp.executable = "task_name";
	sbp.task = FindTask(NULL);
	
	return alert( &sbp, &rr );
}
#endif
