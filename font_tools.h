
#ifndef FONT_TOOLS_H
#define FONT_TOOLS_H

#include "stdafx.h"
#include "interpreter.h"

/** prototypes **/
WORD	calcLenOfString(char *string);
WORD	calcLenOfChar(char c);
WORD	getCurrFontSize();
WORD 	getFontSize(BYTE idFont);
BYTE	getAscent();
BYTE	getSpaceWidth();
BYTE	getHeaderSize();
WORD 	getCharDataBytes(WORD posChar);
WORD 	getCharPos(BYTE c);

BOOL	selectFont(BYTE idFont);
int 	searchFont(BYTE idFont);
BOOL 	writeCharacter(BYTE c, WORD *posY, WORD *posX, BOOL inverseMode);

/** caractère de substitution **/
#define	STUB_CHAR	'_'

#endif	