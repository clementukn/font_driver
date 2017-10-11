
#ifndef INTRPRETER_H
#define INTRPRETER_H

#include "stdafx.h"

// d�finition des caract�res sp�ciaux
#define CTAG 		'#'  // codage d'une instruction (police, centrage, ...)
#define CFONT		'p'	 // changement de police
#define CNORMAL		'w'	 // mode d'�criture simple (bleu sur fond blanc)
#define CINVERSE	'm'	 // mode d'�criture invers� (blanc sur fond bleu)
#define CINVISIBLE	'i'	 // mode d'�criture invisible
#define CVISIBLE	'!'  // mode d'�criture visible
#define CNEWLINE	'n'	 // saut de ligne
#define C1PIXEL		'1'	 // espacement d'1 pixel
#define CCENTERH	'c'	 // centrage horizontal
#define CALIGNL		'l'	 // alignement � gauche

#define CAUTO		'a'	 // mode affichage automatique



#define BORDERX		0	// bordure en pixel sur axe X
#define BORDERY		0	// bordure en pixel sur axe Y


typedef enum {
	AUTO
} DisplayMode;


/* D�finition du mode/style d'�criture utilis� 
 * � un instant T
 */
typedef struct {
	// police utilis�e
	BYTE	fontId;		

	// mode d'affichage utilis� : 2 lignes, plein �cran, ...
	DisplayMode	mode;	

	// �criture en mode invers�e activ�e
	BOOL	inverseWriting;	
	// mode invisible
	BOOL	invisible;
	// centrage activ�. Si non, alignement � gauche	
	BOOL	centerText;

	// position du curseur sur l'�cran � un instant T
	WORD	cursorX;
	WORD	cursorY;
	// position du curseur pr�c�dent (seulement X n�cessaire)
	WORD	prevCursorX;

	// d�limitation verticale
	WORD areaYStart;
	WORD areaYEnd;

 } Brush;



// d�claration des prototypes
int parseHeader(const char *string);
int parseData();
int decodeTag(const char *string, Brush *myBrush);
int writeString(const char *string, BYTE heightLine);

#endif	