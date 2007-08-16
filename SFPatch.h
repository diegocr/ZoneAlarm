#ifndef SFPATCH_H
#define SFPATCH_H
/*
 * SFPatch.h 1.0
 *
 * Lee Kindness
 *
 * Public Domain
 *
 */

/* Includes */

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <SDI_compiler.h>
//#include <libfuncoset.h>

/* Structures */

enum {
	JMPINSTR = 0x4ef9
};

typedef struct JmpEntry {
	UWORD Instr;
	APTR Func;
} JmpEntry;

typedef struct SetFunc {
	APTR            sf_Func;
	APTR            sf_OriginalFunc;
	JmpEntry       *sf_Jmp;
	struct Library *sf_Library;
	LONG            sf_Offset;
	LONG            sf_QuitMethod;
	LONG            sf_Count;
} SetFunc;

BOOL SFReplace(SetFunc *sf);
void SFRestore(SetFunc *sf);

/* Constants for SetFunc->sf_QuitMethod */
#define SFQ_WAIT 0 /* wait sf_Count seconds (defaults to 3) */
#define SFQ_COUNT 1 /* only quit if count == 0 (retry every 3 secs...) */

#define SetSetFunc( sf, func, newfunc)		\
	(sf)->sf_Func       = newfunc;		\
	(sf)->sf_Library    = func ## _BASE;	\
	(sf)->sf_Offset     = func ## _OSET;	\
	(sf)->sf_QuitMethod = SFQ_COUNT

#endif /* SFPATCH_H */

